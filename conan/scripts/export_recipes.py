#!/usr/bin/env python3
"""Export every recipe declared in conan/packages.yml into the local Conan cache.

Each recipe is exported from the cloned conan-center-index (CCI_DIR env var)
under @openrv/<channel> so that replace_requires in conan/profiles/common can
resolve transitive deps locally during the build step.

Environment variables:
    CCI_DIR    - Path to the cloned conan-center-index repo (required).
    CONAN_EXEC - Full path to the conan binary (optional; defaults to "conan").
"""

import os
import subprocess
import sys
from pathlib import Path

import yaml

from _common import CONAN_USER, conan_exec, load_packages


def patch_conandata(recipe_path: Path, dep: dict) -> None:
    """Inject a missing version entry into the CCI recipe's conandata.yml.

    When packages.yml provides source_url/source_sha256 for a version that
    CCI's conandata.yml does not list, this adds the entry so that the
    recipe's source() method can download the tarball.
    """
    source_url = dep.get("source_url")
    source_sha256 = dep.get("source_sha256")
    if not source_url:
        return

    conandata_path = recipe_path / "conandata.yml"
    if not conandata_path.is_file():
        return

    conandata = yaml.safe_load(conandata_path.read_text()) or {}

    version = dep["version"]
    sources = conandata.setdefault("sources", {})
    if version in sources:
        return

    entry = {"url": source_url}
    if source_sha256:
        entry["sha256"] = source_sha256
    sources[version] = entry

    with conandata_path.open("w") as f:
        yaml.dump(conandata, f, default_flow_style=False, sort_keys=False)
    print(f"  Patched conandata.yml: added sources entry for {dep['name']}/{version}")


def main() -> None:
    cci_dir = os.environ.get("CCI_DIR")
    if not cci_dir:
        print("error: CCI_DIR environment variable is not set.", file=sys.stderr)
        sys.exit(1)

    conan = conan_exec()
    pkgs = load_packages()

    # Intentionally exports ALL deps, including windows_only entries, on
    # every platform. Exporting is cheap (no compilation) and ensures that
    # replace_requires in profiles/common can resolve on all platforms.
    # Platform filtering happens at build time in conan_install_target.py.
    failures: list[str] = []
    for dep in pkgs["deps"]:
        name = dep["name"]
        version = dep["version"]
        channel = dep["channel"]
        cci_folder = dep["cci_folder"]
        ref = f"{name}/{version}@{CONAN_USER}/{channel}"

        recipe_path = Path(cci_dir) / "recipes" / name / cci_folder
        if not recipe_path.is_dir():
            print(f"::error::Recipe folder not found: {recipe_path}", file=sys.stderr)
            failures.append(f"{name}/{version}")
            continue

        patch_conandata(recipe_path, dep)

        cmd = [
            conan,
            "export",
            str(recipe_path),
            "--version",
            version,
            "--user",
            CONAN_USER,
            "--channel",
            channel,
        ]
        print(f"\n>>> {' '.join(cmd)}", flush=True)
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"::error::conan export failed for {ref}", file=sys.stderr)
            failures.append(f"{name}/{version}")

    if failures:
        print(f"\n::error::{len(failures)} export(s) failed: {', '.join(failures)}", file=sys.stderr)
        sys.exit(1)

    print(f"\nAll {len(pkgs['deps'])} recipes exported successfully.")


if __name__ == "__main__":
    main()
