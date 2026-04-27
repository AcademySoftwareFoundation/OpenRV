#!/usr/bin/env python3
"""
Export every dep in conan/packages.yml into the local Conan cache under
@openrv/vfx2024 so that [replace_requires] entries in conan/profiles/common
can be resolved when building a single target.

Called from .github/workflows/conan-upload-deps.yml per matrix cell. Reads
IS_WINDOWS_JOB from the environment to decide whether to include
windows_only recipes (pcre2).

Reads CCI_DIR from the environment (defaults to /tmp/cci) to find the
conan-center-index checkout. The workflow sets it per-platform because
MSYS2 and native Windows Python disagree on where /tmp lives.
"""
import os
import subprocess
import sys

import yaml

CCI_ROOT = os.path.join(os.environ.get("CCI_DIR", "/tmp/cci"), "recipes")
CONAN_EXEC = os.environ.get("CONAN_EXEC", "conan")


def main() -> int:
    with open("conan/packages.yml", encoding="utf-8") as fh:
        cfg = yaml.safe_load(fh)

    is_windows = os.environ.get("IS_WINDOWS_JOB", "0") == "1"
    exported = 0

    for dep in cfg["deps"]:
        if dep.get("windows_only") and not is_windows:
            continue
        name = dep["name"]
        version = dep["version"]
        folder = dep["cci_folder"]
        recipe_dir = os.path.join(CCI_ROOT, name, folder)
        if not os.path.isdir(recipe_dir):
            sibling_root = os.path.join(CCI_ROOT, name)
            siblings = []
            if os.path.isdir(sibling_root):
                siblings = sorted(os.listdir(sibling_root))
            print(
                f"::error::Recipe folder not found: {recipe_dir}. "
                f"Siblings under {sibling_root}: {siblings}",
                file=sys.stderr,
            )
            return 1

        cmd = [
            CONAN_EXEC, "export", recipe_dir,
            "--version", version,
            "--user", "openrv",
            "--channel", "vfx2024",
        ]
        print(f"$ {' '.join(cmd)}", flush=True)
        subprocess.run(cmd, check=True)
        exported += 1

    print(f"Exported {exported} recipes under @openrv/vfx2024.", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
