#!/usr/bin/env python3
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""End-to-end smoke test for RV's crash-dump capability.

Launches the RV main executable with the test-only ``crash()`` Mu command and
confirms the process crashes and that a Crashpad minidump is written to a
private crash-dump directory. When a ``minidump_dump`` tool is provided (macOS /
Linux), it also inspects the dump and asserts the expected annotations are
present. When it is not provided -- e.g. on Windows, where Breakpad's
``minidump_dump`` is not built -- the annotation check is skipped and the test
passes on a dump being produced (native PDB symbolication is used on Windows).

This validates the core live path: handler init (C4/C5), crash capture, and
dump delivery; plus, where ``minidump_dump`` is available, a static annotation
from the table (C3). See docs/crash-reporting.md.

The test runs headless (QT_QPA_PLATFORM=offscreen) so it works on CI runners
without a display. Note: the Mu execution-context annotation (mu_function, C2)
is NOT populated under the offscreen platform, so it is reported for information
only and not asserted here. mu_function IS captured in a real display session;
verify it manually or in a display-backed run. See docs/crash-reporting.md
(section 8).
"""

import argparse
import contextlib
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

# Written by the platform wrapper next to the dumps (see
# crashpad_handler_linux.sh.in / crashpad_handler_macos.sh.in).
HANDLER_LOG_NAME = "crashpad_handler.log"

# Start marker the Linux wrapper writes just before exec'ing crashpad_handler.
WRAPPER_MARKER_PATTERN = re.compile(r"^\[wrapper\].*\bpid=(\d+)", re.MULTILINE)

# Trailing handler-log lines echoed on the success path. Bounded so a chatty
# handler cannot flood the CI log; the failure path still dumps the last 8 KiB.
SUCCESS_HANDLER_LOG_LINES = 20

# Annotation keys that MUST appear (non-empty) in the dump. "platform" is set at
# init for every dump and is present even headless, so it is a reliable signal
# that capture + annotation delivery worked.
REQUIRED_ANNOTATIONS = ("platform",)

# Reported for information only (not asserted) -- empty under offscreen Qt.
INFO_ANNOTATIONS = ("mu_function", "mu_script_file", "py_function")

# crash() is invoked from a named Mu function so mu_function has a value in a
# display-backed run (informational here).
CRASH_EVAL = "function: rvCrashSmokeTest (void;) { crash(); } rvCrashSmokeTest();"


def log(msg):
    print(f"[crash-dump-smoke] {msg}", flush=True)


def find_dumps(crash_dir):
    # Crashpad writes new reports under <dir>/pending/<uuid>.dmp.
    return glob.glob(os.path.join(crash_dir, "**", "*.dmp"), recursive=True)


def resource_snapshot(path):
    """Return a free-disk / free-memory summary for path.

    Both kinds of starvation can silently cost us the dump: no space for the
    handler to write it, or not enough memory, so that the kernel kills the
    handler before the crash happens. The numbers are logged before the launch
    and again if no dump shows up, so the two can be compared.
    """
    parts = []
    try:
        usage = shutil.disk_usage(path)
        parts.append(f"disk free {usage.free // (1 << 20)} MiB of {usage.total // (1 << 20)} MiB")
    except OSError as exc:
        parts.append(f"disk usage unavailable ({exc})")

    try:
        with open("/proc/meminfo", "r", encoding="utf-8") as fh:
            match = re.search(r"^MemAvailable:\s+(\d+) kB", fh.read(), re.MULTILINE)
        if match:
            parts.append(f"memory available {int(match.group(1)) // 1024} MiB")
    except OSError:
        pass  # No /proc (macOS/Windows): disk numbers alone will have to do.

    return ", ".join(parts)


def read_handler_log(crash_dir):
    """Return (size, tail, handler_pid) for the handler log, or None if absent.

    The wrappers create the log through a shell redirection, so the file
    existing proves only that the wrapper ran. The "[wrapper] ... starting"
    marker is written just before the exec, so its absence means
    crashpad_handler itself never started; a failing exec reports its error
    into the same log, right after the marker. handler_pid is None when the
    marker is missing (that includes macOS and Windows, whose wrappers do not
    write one).
    """
    handler_log = os.path.join(crash_dir, HANDLER_LOG_NAME)
    if not os.path.isfile(handler_log):
        return None

    with open(handler_log, "rb") as fh:
        fh.seek(0, os.SEEK_END)
        size = fh.tell()
        fh.seek(max(0, size - 8192))
        tail = fh.read().decode("utf-8", errors="replace")

    match = WRAPPER_MARKER_PATTERN.search(tail)
    return size, tail, int(match.group(1)) if match else None


def log_handler_log_summary(crash_dir):
    """Log the handler-log summary, and a bounded tail, on the success path.

    The summary line is cheap continuous proof that the marker mechanism still
    works, so we find out it has broken here rather than on the one run where
    the dump is missing and the log is the only evidence we have.

    The tail is diagnostic and temporary: a healthy capture writes several KB
    of handler stderr and we do not yet know what it says, which matters
    because the run that flaked wrote none of it. Reduce this back to the
    summary line alone once that output is understood.
    """
    entry = read_handler_log(crash_dir)
    if entry is None:
        log(f"{HANDLER_LOG_NAME}: absent")
        return

    size, tail, handler_pid = entry
    marker = f"marker present (handler pid {handler_pid})" if handler_pid is not None else "no marker"
    log(f"{HANDLER_LOG_NAME}: {size} bytes, {marker}")

    lines = tail.splitlines()
    shown = lines[-SUCCESS_HANDLER_LOG_LINES:]
    if shown:
        log(f"  (last {len(shown)} of {len(lines)} lines):")
        for line in shown:
            log(f"  | {line}")


def report_handler_log(crash_dir):
    """Log the handler's own log in full, and return the pid it reported."""
    entry = read_handler_log(crash_dir)
    if entry is None:
        log(f"{HANDLER_LOG_NAME} does not exist: the handler wrapper never ran.")
        return None

    size, tail, handler_pid = entry
    log(f"{HANDLER_LOG_NAME} ({size} bytes, last 8 KiB):")
    for line in tail.splitlines():
        log(f"  | {line}")

    if handler_pid is None:
        log("  -> no wrapper start marker: crashpad_handler was never exec'd.")
        return None

    log(f"  -> wrapper reached exec with pid {handler_pid}; lines after it are the exec error or handler stderr.")
    return handler_pid


def report_oom_kills(handler_pid):
    """Log any kernel OOM activity, which would explain a handler that died."""
    if not sys.platform.startswith("linux"):
        return

    try:
        completed = subprocess.run(["dmesg"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=30)
    except (OSError, subprocess.TimeoutExpired) as exc:
        log(f"dmesg unavailable ({exc}); cannot check for OOM kills.")
        return

    text = completed.stdout.decode("utf-8", errors="replace")
    hits = [line for line in text.splitlines() if re.search(r"Out of memory|oom-kill|Killed process", line)]
    if not hits:
        log("dmesg reports no OOM kills.")
        return

    log("dmesg OOM activity (last 10 matching lines):")
    for line in hits[-10:]:
        log(f"  | {line}")
    if handler_pid is not None and any(str(handler_pid) in line for line in hits):
        log(f"  -> handler pid {handler_pid} appears in the OOM kill log.")


@contextlib.contextmanager
def macos_crash_dialog_suppressed():
    """Temporarily suppress the macOS "Report to Apple" crash dialog.

    The deliberate crash() would otherwise pop the system CrashReporter dialog
    (driven by ReportCrash, independent of the app's Qt platform). Set
    com.apple.CrashReporter DialogType=none for the duration of the launch and
    restore the previous value afterward. No-op off macOS.
    """
    if sys.platform != "darwin":
        yield
        return

    domain, key = "com.apple.CrashReporter", "DialogType"
    probe = subprocess.run(
        ["defaults", "read", domain, key],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    had_value = probe.returncode == 0
    prev_value = probe.stdout.strip() if had_value else None

    subprocess.run(["defaults", "write", domain, key, "none"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    log("Suppressed macOS crash dialog (com.apple.CrashReporter DialogType=none).")
    try:
        yield
    finally:
        if had_value:
            subprocess.run(
                ["defaults", "write", domain, key, prev_value], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )
        else:
            subprocess.run(["defaults", "delete", domain, key], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        log("Restored macOS crash dialog setting.")


def annotation_value(dump_text, key):
    """Return the annotation value for key from minidump_dump output, or None.

    Matches both the process-level (simple_annotations["k"] = v) and module-level
    (crashpad_annotations["k"] (type = N) = v) forms; returns the first non-empty
    value found, else "" if the key is present but empty, else None.
    """
    # Match the whole line, then take the text after the LAST "= " so the
    # module-level "(type = N) = value" form is parsed correctly (not the "="
    # inside "(type = N)").
    pattern = re.compile(r'^.*_annotations\["' + re.escape(key) + r'"\].*$', re.MULTILINE)
    found_empty = False
    for m in pattern.finditer(dump_text):
        line = m.group(0)
        value = line.rsplit("= ", 1)[-1].strip() if "= " in line else ""
        if value:
            return value
        found_empty = True
    return "" if found_empty else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rv-binary", required=True, help="Path to the RV main executable to drive.")
    parser.add_argument(
        "--minidump-dump",
        default=None,
        help="Path to the minidump_dump tool. If omitted or missing (e.g. Windows, "
        "where Breakpad is not built), annotation verification is skipped.",
    )
    parser.add_argument("--launch-timeout", type=int, default=180, help="Seconds to wait for RV to crash.")
    parser.add_argument("--dump-timeout", type=int, default=30, help="Seconds to wait for the dump to appear.")
    args = parser.parse_args()

    if not os.path.isfile(args.rv_binary):
        log(f"FAIL: RV binary not found: {args.rv_binary}")
        return 1

    # minidump_dump is optional: when absent (e.g. Windows) the annotation
    # verification is skipped and the test only checks that a dump was produced.
    have_dump_tool = bool(args.minidump_dump) and os.path.isfile(args.minidump_dump)
    if args.minidump_dump and not have_dump_tool:
        log(f"WARNING: minidump_dump not found at {args.minidump_dump}; skipping annotation verification.")

    with tempfile.TemporaryDirectory(prefix="rv-crash-smoke-") as crash_dir:
        env = dict(os.environ)
        env["RV_CRASH_DUMPS_ENABLED"] = "1"
        env["RV_CRASH_DUMPS_DIR"] = crash_dir
        # Avoid the macOS-only handler restart for log attachments during this
        # headless test; capture correctness is what we are validating here.
        env["RV_CRASH_DUMPS_ATTACH_LOGS"] = "0"
        # Run headless so the test works on CI runners without a display.
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        # GHA/RHEL container jobs often run as root; Qt WebEngine refuses to start
        # without disabling the Chromium sandbox in that case.
        if sys.platform.startswith("linux") and os.geteuid() == 0:
            env.setdefault("QTWEBENGINE_DISABLE_SANDBOX", "1")

        log(f"Launching: {args.rv_binary} -eval '{CRASH_EVAL}'")
        log(f"Crash dir: {crash_dir}")
        log(f"Resources before launch: {resource_snapshot(crash_dir)}")
        try:
            with macos_crash_dialog_suppressed():
                proc = subprocess.run(
                    [args.rv_binary, "-eval", CRASH_EVAL],
                    env=env,
                    timeout=args.launch_timeout,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                )
            rc = proc.returncode
        except subprocess.TimeoutExpired:
            log(f"FAIL: RV did not exit within {args.launch_timeout}s (no crash?).")
            return 1

        # A captured crash makes the process exit abnormally (non-zero).
        log(f"RV exited with code {rc}")
        if proc.stdout:
            log("RV output (last 8 KiB):")
            tail = proc.stdout[-8192:]
            if isinstance(tail, bytes):
                tail = tail.decode("utf-8", errors="replace")
            for line in tail.splitlines():
                log(f"  | {line}")
        if rc == 0:
            log("FAIL: RV exited cleanly; the crash was not triggered.")
            return 1

        # The dump is written by the out-of-process handler, so poll briefly.
        deadline = time.time() + args.dump_timeout
        dumps = []
        while time.time() < deadline:
            dumps = find_dumps(crash_dir)
            if dumps:
                break
            time.sleep(0.5)

        if not dumps:
            # A missing dump is almost never a bug in the capture code itself;
            # it is the handler process being absent, starved or killed. Report
            # enough about its state to tell those apart from the CI log alone.
            handler_pid = report_handler_log(crash_dir)
            log(f"Resources after failure: {resource_snapshot(crash_dir)}")
            report_oom_kills(handler_pid)
            log(f"FAIL: no .dmp produced in {crash_dir} within {args.dump_timeout}s.")
            return 1

        dump = max(dumps, key=os.path.getmtime)
        log(f"Found dump: {dump}")
        log_handler_log_summary(crash_dir)

        if not have_dump_tool:
            log("PASS: dump produced (minidump_dump not provided; annotation verification skipped).")
            return 0

        try:
            out = subprocess.run(
                [args.minidump_dump, dump],
                timeout=60,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
            ).stdout.decode("utf-8", errors="replace")
        except subprocess.TimeoutExpired:
            log("FAIL: minidump_dump timed out.")
            return 1

        missing = []
        for key in REQUIRED_ANNOTATIONS:
            value = annotation_value(out, key)
            if value:
                log(f"  annotation {key} = {value}")
            else:
                missing.append(key)

        for key in INFO_ANNOTATIONS:
            value = annotation_value(out, key)
            log(f"  (info) annotation {key} = {value!r}")

        if missing:
            log(f"FAIL: dump is missing required annotation(s): {', '.join(missing)}")
            return 1

        log(f"PASS: dump produced with required annotation(s): {', '.join(REQUIRED_ANNOTATIONS)}")
        return 0


if __name__ == "__main__":
    sys.exit(main())
