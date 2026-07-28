#!/usr/bin/env python3
"""Build a single target package from conan/packages.yml using conan install.

Usage:
    python3 conan/scripts/conan_install_target.py <name> <version> <profile>

Reads packages.yml to look up the dep's channel and options, then runs:
    conan install --requires=<name>/<version>@openrv/<channel>
        --build=missing --build=b2/* --build=meson/* --build=nasm/*
        -pr:a ./conan/profiles/<profile>
        -o <option>=<value> ...

Environment variables:
    CONAN_EXEC      - Full path to the conan binary (optional; defaults to "conan").
    IS_WINDOWS_JOB  - Set to "1" to apply windows_only options too.
"""

import argparse
import os
import subprocess
import sys

from _common import CONAN_USER, PROFILES_DIR, conan_exec, find_dep, is_active_dep, load_packages


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("name")
    parser.add_argument("version")
    parser.add_argument("profile")
    args = parser.parse_args()

    pkgs = load_packages()
    dep = find_dep(pkgs, args.name, args.version)
    if dep is None:
        print(f"::error::{args.name}/{args.version} not found in conan/packages.yml", file=sys.stderr)
        sys.exit(1)

    channel = dep["channel"]
    profile_path = str(PROFILES_DIR / args.profile)
    ref = f"{args.name}/{args.version}@{CONAN_USER}/{channel}"

    cmd = [
        conan_exec(),
        "install",
        f"--requires={ref}",
        "--build=missing",
        "--build=b2/*",
        "--build=meson/*",
        "--build=nasm/*",
        "-pr:a",
        profile_path,
    ]

    # Append options from ALL deps in packages.yml, not just the target.
    # The consumer (openrvcore-conanfile.py) sets options for every dep at
    # once, so transitive deps like zlib get shared=True. We must match
    # those options here so the built package_id is identical to what the
    # consumer will request.
    is_windows = os.environ.get("IS_WINDOWS_JOB") == "1"
    for d in pkgs["deps"]:
        if not is_active_dep(d, is_windows=is_windows):
            continue
        for k, v in (d.get("options") or {}).items():
            cmd.extend(["-o", f"{k}={v}"])

    print(f"\n>>> {' '.join(cmd)}", flush=True)
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(f"::error::conan install failed for {ref}", file=sys.stderr)
        sys.exit(result.returncode)

    print(f"\nBuild succeeded: {ref}")


if __name__ == "__main__":
    main()
