#!/usr/bin/env python3
"""
Build a single @openrv/vfx2024 target with `conan install`, passing the
union of options declared across conan/packages.yml so transitive deps
that get resolved via [replace_requires] (see conan/profiles/common) are
built with the shared/static settings OpenRV expects.

Usage: conan_install_target.py <name> <version> <conan-profile>

Called from .github/workflows/conan-upload-deps.yml per matrix cell. Reads
IS_WINDOWS_JOB from the environment to decide whether to include
windows_only entries in the options union.

Forces local rebuild of the *target itself* via `--build={name}/*`. The
Rocky 8 and Rocky 9 profiles share `os/compiler/compiler.version/libcxx`
and therefore produce the same Conan package_id; a Rocky 9 binary
uploaded earlier would otherwise be downloaded by `--build=missing` on
Rocky 8 (visible as "already in server, skipping upload" on the upload
step) and then break the Rocky 8 OpenRV consumer at link time on
GLIBC_2.29+/GLIBCXX_3.4.26+. Forcing a local rebuild produces a fresh
package_revision on the matching toolchain; Conan resolves the latest
revision by default, so the new upload supersedes any older one.

Also forces local rebuild of host tools (b2, meson, nasm) via extra
--build patterns. b2 reaches every cell that builds boost via
[replace_tool_requires] in conan/profiles/common; if the b2 binary on
the remote was built on a newer glibc host (same Rocky 8/9 package_id
collision as the target) it fails to even execute on Rocky 8 with
"GLIBC_2.32 not found". meson/nasm are CCI prebuilts that hit the same
class of host-mismatch issue. Forcing local rebuild on each cell makes
the consuming cell independent of whatever is currently on the remote.
"""
import os
import subprocess
import sys

import yaml


def main() -> int:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} <name> <version> <conan-profile>", file=sys.stderr)
        return 2
    name, version, profile = sys.argv[1], sys.argv[2], sys.argv[3]

    with open("conan/packages.yml", encoding="utf-8") as fh:
        cfg = yaml.safe_load(fh)

    is_windows = os.environ.get("IS_WINDOWS_JOB", "0") == "1"

    option_flags: list[str] = []
    for dep in cfg["deps"]:
        if dep.get("windows_only") and not is_windows:
            continue
        for key, value in (dep.get("options") or {}).items():
            option_flags += ["-o", f"{key}={value}"]

    conan_exec = os.environ.get("CONAN_EXEC", "conan")
    cmd = [
        conan_exec, "install",
        f"--requires={name}/{version}@openrv/vfx2024",
        "--build=missing",
        f"--build={name}/*",
        "--build=b2/*",
        "--build=meson/*",
        "--build=nasm/*",
        "-pr:a", f"./conan/profiles/{profile}",
        *option_flags,
    ]
    print(f"$ {' '.join(cmd)}", flush=True)
    subprocess.run(cmd, check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
