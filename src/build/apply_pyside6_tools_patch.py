#!/usr/bin/env python3
# Copyright 2026 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Apply PySide6 patches that tolerate missing Qt Designer tooling."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def apply_patch(source_dir: Path, patch_file: Path) -> bool:
    result = subprocess.run(
        ["patch", "-p1", "--binary", "-N", "-i", str(patch_file)],
        cwd=source_dir,
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        return True

    combined_output = f"{result.stdout}\n{result.stderr}"
    if "Ignoring previously applied (or reversed) patch." in combined_output:
        return True

    print(combined_output, file=sys.stderr)
    return False


def main() -> int:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <pyside-source-dir> <patch-dir>", file=sys.stderr)
        return 2

    source_dir = Path(sys.argv[1]).resolve()
    patch_dir = Path(sys.argv[2]).resolve()

    platform_patch = patch_dir / "pyside6_tools_optional_qt_app_bundles.patch"
    cmake_patch = patch_dir / "pyside6_tools_optional_qt_app_bundles_cmake683.patch"

    if not apply_patch(source_dir, platform_patch):
        print(f"ERROR: Failed to apply {platform_patch}", file=sys.stderr)
        return 1

    if not apply_patch(source_dir, cmake_patch):
        print(f"ERROR: Failed to apply {cmake_patch}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
