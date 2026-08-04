#!/usr/bin/env python3
"""Runtime error detection for golden-test scenarios.

At Mu capture time, normalized error *signatures* are committed as
``runtime_errors.txt`` beside ``session.rv``. Gate 0 passes when the Python port
introduces no signatures beyond that Mu baseline (same pre-existing RV noise is
fine; new regressions are not).

Also used on every ``run_scenario.py`` invocation when ``--runtime-golden-dir`` is set.
"""

from __future__ import annotations

import argparse
import os
import re
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))

GOLDEN_RUNTIME_FILE = "runtime_errors.txt"

_ERROR_LINE_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"Traceback \(most recent call last\)", re.I),
    re.compile(r"Exception thrown while calling", re.I),
    re.compile(r"runtime\.eval,\s*line\s+\d+", re.I),
    re.compile(r"Unresolved reference to", re.I),
    re.compile(r"Unable to reference", re.I),
    re.compile(r"Cannot use default constructor", re.I),
    re.compile(r"^Exception\s*:", re.I),
    re.compile(r"^TypeError\s*:", re.I),
    re.compile(r"^SyntaxError\s*:", re.I),
    re.compile(r"^ValueError\s*:", re.I),
    re.compile(r"^AttributeError\s*:", re.I),
    re.compile(r"^RuntimeError\s*:", re.I),
    re.compile(r"^Mu\s+exception\s*:", re.I),
)

_EXCEPTION_LINE = re.compile(
    r"^(\w+(?:Error|Exception)|Exception|SyntaxError|TypeError|ValueError|"
    r"AttributeError|RuntimeError|Mu exception)\b",
    re.I,
)


def _is_error_line(line: str) -> bool:
    return any(p.search(line) for p in _ERROR_LINE_PATTERNS)


def _extract_blocks(text: str) -> list[str]:
    blocks: list[str] = []
    current: list[str] = []
    in_traceback = False

    for line in text.splitlines():
        if re.search(r"Traceback \(most recent call last\)", line, re.I):
            if current:
                blocks.append("\n".join(current))
            current = [line]
            in_traceback = True
            continue
        if in_traceback:
            current.append(line)
            if line.strip() == "" and len(current) > 3:
                blocks.append("\n".join(current))
                current = []
                in_traceback = False
            continue
        if _is_error_line(line):
            blocks.append(line)
    if current:
        blocks.append("\n".join(current))
    return blocks


def _normalize_line(line: str) -> str:
    line = re.sub(r"\bline \d+", "line N", line)
    line = re.sub(r":\d+:", ":N:", line)
    line = re.sub(r"0x[0-9a-fA-F]+", "0xADDR", line)
    return line.strip()


def signature_from_block(block: str) -> str:
    """Stable signature for one traceback or standalone error line."""
    lines = [ln.strip() for ln in block.splitlines() if ln.strip()]
    if not lines:
        return ""

    exc = ""
    for ln in reversed(lines):
        if _EXCEPTION_LINE.match(ln):
            exc = _normalize_line(ln)
            break
    if not exc:
        exc = _normalize_line(lines[-1])

    frames: list[str] = []
    for ln in lines:
        if 'File "' in ln or "runtime.eval" in ln.lower():
            frames.append(_normalize_line(ln))

    if frames:
        return f"{exc} @ {' | '.join(frames[-2:])}"
    return exc


def collect_signatures(
    rv_log_text: str,
    *,
    traceback_text: str | None = None,
) -> set[str]:
    sigs: set[str] = set()
    if traceback_text and traceback_text.strip():
        sig = signature_from_block(traceback_text.strip())
        if sig:
            sigs.add(sig)
    for block in _extract_blocks(rv_log_text):
        sig = signature_from_block(block)
        if sig:
            sigs.add(sig)
    return sigs


def _read_out_logs(out_dir: str) -> tuple[str, str | None]:
    rv_log = os.path.join(out_dir, "rv.log")
    tb_path = os.path.join(out_dir, "traceback.txt")
    rv_text = ""
    if os.path.isfile(rv_log):
        with open(rv_log, encoding="utf-8", errors="replace") as f:
            rv_text = f.read()
    tb_text = None
    if os.path.isfile(tb_path):
        with open(tb_path, encoding="utf-8", errors="replace") as f:
            tb_text = f.read()
    return rv_text, tb_text


def signatures_from_out_dir(out_dir: str) -> set[str]:
    rv_text, tb_text = _read_out_logs(out_dir)
    return collect_signatures(rv_text, traceback_text=tb_text)


def load_golden_runtime_signatures(golden_dir: str) -> set[str]:
    path = os.path.join(golden_dir, GOLDEN_RUNTIME_FILE)
    if not os.path.isfile(path):
        return set()
    sigs: set[str] = set()
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#"):
                sigs.add(line)
    return sigs


def find_new_runtime_errors(out_dir: str, golden_dir: str) -> list[str]:
    """Return sorted signatures in *out_dir* that are not in the Mu golden baseline."""
    actual = signatures_from_out_dir(out_dir)
    baseline = load_golden_runtime_signatures(golden_dir)
    return sorted(actual - baseline)


def write_runtime_baseline(out_dir: str, dest_path: str) -> set[str]:
    sigs = signatures_from_out_dir(out_dir)
    os.makedirs(os.path.dirname(dest_path) or ".", exist_ok=True)
    with open(dest_path, "w", encoding="utf-8") as f:
        f.write(
            "# Normalized runtime error signatures from Mu capture.\n"
            "# Python port must not introduce errors beyond this set.\n"
        )
        for sig in sorted(sigs):
            f.write(sig + "\n")
    return sigs


def write_runtime_report(
    out_dir: str,
    *,
    new_errors: list[str],
    actual: set[str],
    baseline: set[str],
) -> None:
    path = os.path.join(out_dir, "runtime_errors.txt")
    with open(path, "w", encoding="utf-8") as f:
        if new_errors:
            f.write("NEW runtime errors (not in Mu golden):\n")
            for err in new_errors:
                f.write(err + "\n")
            f.write("\n")
        f.write(f"Actual signatures ({len(actual)}):\n")
        for sig in sorted(actual):
            f.write(sig + "\n")
        f.write(f"\nGolden baseline ({len(baseline)}):\n")
        for sig in sorted(baseline):
            f.write(sig + "\n")


def check_runtime_delta(out_dir: str, golden_dir: str) -> list[str]:
    """Compare *out_dir* against golden baseline; return new error signatures."""
    actual = signatures_from_out_dir(out_dir)
    baseline = load_golden_runtime_signatures(golden_dir)
    new_errors = sorted(actual - baseline)
    write_runtime_report(
        out_dir,
        new_errors=new_errors,
        actual=actual,
        baseline=baseline,
    )
    return new_errors


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("out_dir", help="Scenario output dir (contains rv.log)")
    ap.add_argument(
        "--golden-dir",
        help="Golden baseline dir containing runtime_errors.txt (delta check)",
    )
    ap.add_argument(
        "--write-baseline",
        metavar="PATH",
        help="Write Mu runtime_errors.txt from OUT_DIR to PATH and exit",
    )
    args = ap.parse_args()

    out_dir = os.path.abspath(args.out_dir)

    if args.write_baseline:
        sigs = write_runtime_baseline(out_dir, os.path.abspath(args.write_baseline))
        print(f"runtime baseline: {len(sigs)} signature(s) -> {args.write_baseline}")
        return 0

    if args.golden_dir:
        new_errors = check_runtime_delta(out_dir, os.path.abspath(args.golden_dir))
        if not new_errors:
            print("runtime: PASS (no new errors vs Mu golden)")
            return 0
        print("runtime: FAIL (new errors vs Mu golden)")
        for err in new_errors:
            print("---")
            print(err)
        return 1

    sigs = signatures_from_out_dir(out_dir)
    if not sigs:
        print("runtime: CLEAN")
        return 0
    print("runtime: FAIL (errors present; pass --golden-dir for delta check)")
    for sig in sorted(sigs):
        print("---")
        print(sig)
    return 1


if __name__ == "__main__":
    sys.exit(main())
