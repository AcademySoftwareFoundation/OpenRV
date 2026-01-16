#!/usr/bin/env python3
"""
Patch OpenTimelineIO debug build to avoid pybind11 GIL assertions and ensure debug
module naming on Windows.

Actions:
- Download OTIO source for the requested version (sdist only).
- Patch otio_utils.cpp to lazily initialize _value_to_any (fixes GIL assert).
- Build & install OTIO from the patched source using the provided Python.
- On Windows Debug, copy _otio/_opentime pyds to *_d names if missing.
"""

from __future__ import annotations

import argparse
import glob
import os
import shutil
import subprocess
import tempfile
from pathlib import Path


def _run(cmd: list[str], env: dict[str, str]) -> None:
    subprocess.check_call(cmd, env=env)


def _patch_otio_utils(root: Path) -> None:
    target = next(
        root.glob("**/src/py-opentimelineio/opentimelineio-bindings/otio_utils.cpp")
    )
    text = target.read_text(encoding="utf-8")
    # Make _value_to_any lazy-initialized to avoid pybind11 GIL asserts at static init.
    if "static py::object _value_to_any = py::none();" not in text:
        raise RuntimeError("Expected marker not found in otio_utils.cpp")
    text = text.replace(
        "static py::object _value_to_any = py::none();",
        "// Initialized lazily after the interpreter is ready; constructing py::none()\n"
        "// at static init time triggers pybind11 GIL assertions in Debug builds.\n"
        "static py::object _value_to_any;",
        1,
    )
    # Relax the guard to handle default-constructed (null) py::object
    text = text.replace(
        "if (_value_to_any.is_none()) {",
        "if (!_value_to_any || _value_to_any.is_none()) {",
        1,
    )
    target.write_text(text, encoding="utf-8")


def _maybe_copy_debug_names(site_packages: Path) -> None:
    otio_path = site_packages / "opentimelineio"
    if not otio_path.exists():
        return

    for base in ("_otio", "_opentime"):
        for pyd in otio_path.glob(f"{base}*.pyd"):
            name = pyd.name
            if "_d.cp" in name:
                continue
            if "d.cp" in name:
                # already has a trailing d before cp tag
                continue
            if ".cp" in name:
                corrected = name.replace(".cp", "_d.cp", 1)
            else:
                corrected = name.replace(".pyd", "_d.pyd")
            dest = pyd.with_name(corrected)
            if not dest.exists():
                shutil.copy2(pyd, dest)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--python-exe", required=True, help="Path to python_d.exe")
    ap.add_argument("--otio-version", required=True, help="OTIO version to patch")
    ap.add_argument("--site-packages", required=True, help="Target site-packages path")
    ap.add_argument(
        "--cmake-args",
        required=True,
        help="CMAKE_ARGS string to ensure Python debug libs are used",
    )
    args = ap.parse_args()

    py = args.python_exe
    version = args.otio_version
    sp = Path(args.site_packages).resolve()

    env = os.environ.copy()
    env["CMAKE_ARGS"] = args.cmake_args
    # Work around rare pip/pyproject_hooks KeyError on _PYPROJECT_HOOKS_BUILD_BACKEND
    # by providing a default backend name when unset.
    env.setdefault("_PYPROJECT_HOOKS_BUILD_BACKEND", "setuptools.build_meta")

    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        _run(
            [
                py,
                "-m",
                "pip",
                "download",
                f"opentimelineio=={version}",
                "--no-binary",
                ":all:",
                "-d",
                str(td_path),
            ],
            env,
        )
        sdist = next(td_path.glob("opentimelineio-*.tar.gz"))
        extract_dir = td_path / "src"
        extract_dir.mkdir()
        _run(["python", "-m", "tarfile", "-e", str(sdist), str(extract_dir)], env)

        # tarfile via python -m tarfile doesn't support -e; fall back to shutil
        if not any(extract_dir.iterdir()):
            shutil.unpack_archive(str(sdist), extract_dir)

        root = next(extract_dir.glob("opentimelineio-*"))
        _patch_otio_utils(root)

        _run(
            [
                py,
                "-m",
                "pip",
                "install",
                "--no-cache-dir",
                "--force-reinstall",
                "--no-build-isolation",
                str(root),
            ],
            env,
        )

    _maybe_copy_debug_names(sp)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
