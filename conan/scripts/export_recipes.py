#!/usr/bin/env python3
"""Export every recipe declared in conan/packages.yml into the local Conan cache.

Each recipe is exported from the cloned conan-center-index (CCI_DIR env var)
under @openrv/<channel> so that replace_requires in conan/profiles/common can
resolve transitive deps locally during the build step.

Environment variables:
    CCI_DIR    - Path to the cloned conan-center-index repo (required).
    CONAN_EXEC - Full path to the conan binary (optional; defaults to "conan").
    IS_WINDOWS_JOB - Set to "1" on Windows runners to skip windows_only filtering.
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


def main() -> None:
    cci_dir = os.environ.get("CCI_DIR")
    if not cci_dir:
        print("error: CCI_DIR environment variable is not set.", file=sys.stderr)
        sys.exit(1)

    conan = os.environ.get("CONAN_EXEC", "conan")
    pkgs = yaml.safe_load(PACKAGES_YML.read_text())

    failures = []
    for dep in pkgs["deps"]:
        name = dep["name"]
        version = dep["version"]
        channel = dep["channel"]
        cci_folder = dep["cci_folder"]

        recipe_path = os.path.join(cci_dir, "recipes", name, cci_folder)
        if not os.path.isdir(recipe_path):
            print(f"::error::Recipe folder not found: {recipe_path}")
            failures.append(name)
            continue

        cmd = [
            conan, "export", recipe_path,
            "--version", version,
            "--user", "openrv",
            "--channel", channel,
        ]
        print(f"\n>>> {' '.join(cmd)}", flush=True)
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"::error::conan export failed for {name}/{version}@openrv/{channel}")
            failures.append(name)

    if failures:
        print(f"\n::error::{len(failures)} export(s) failed: {', '.join(failures)}", file=sys.stderr)
        sys.exit(1)

    print(f"\nAll {len(pkgs['deps'])} recipes exported successfully.")


if __name__ == "__main__":
    main()
