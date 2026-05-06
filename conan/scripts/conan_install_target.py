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
    CONAN_EXEC - Full path to the conan binary (optional; defaults to "conan").
"""

import os
import subprocess
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("error: pyyaml is required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGES_YML = REPO_ROOT / "conan" / "packages.yml"


def find_dep(pkgs: dict, name: str, version: str) -> dict:
    """Find the matching dep entry in packages.yml."""
    for dep in pkgs["deps"]:
        if dep["name"] == name and dep["version"] == version:
            return dep
    return None


def main() -> None:
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <name> <version> <profile>", file=sys.stderr)
        sys.exit(1)

    name, version, profile = sys.argv[1], sys.argv[2], sys.argv[3]
    conan = os.environ.get("CONAN_EXEC", "conan")

    pkgs = yaml.safe_load(PACKAGES_YML.read_text())
    dep = find_dep(pkgs, name, version)
    if dep is None:
        print(f"::error::{name}/{version} not found in conan/packages.yml", file=sys.stderr)
        sys.exit(1)

    channel = dep["channel"]
    profile_path = str(REPO_ROOT / "conan" / "profiles" / profile)

    cmd = [
        conan, "install",
        f"--requires={name}/{version}@openrv/{channel}",
        "--build=missing",
        "--build=b2/*",
        "--build=meson/*",
        "--build=nasm/*",
        f"-pr:a", profile_path,
    ]

    # Append options from ALL deps in packages.yml, not just the target.
    # The consumer (openrvcore-conanfile.py) sets options for every dep at
    # once, so transitive deps like zlib get shared=True. We must match
    # those options here so the built package_id is identical to what the
    # consumer will request.
    is_windows = os.environ.get("IS_WINDOWS_JOB") == "1"
    for d in pkgs["deps"]:
        if d.get("windows_only") and not is_windows:
            continue
        for k, v in (d.get("options") or {}).items():
            cmd.extend(["-o", f"{k}={v}"])

    print(f"\n>>> {' '.join(cmd)}", flush=True)
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(f"::error::conan install failed for {name}/{version}@openrv/{channel}")
        sys.exit(result.returncode)

    print(f"\nBuild succeeded: {name}/{version}@openrv/{channel}")


if __name__ == "__main__":
    main()
