#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2026 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
Adapt a python-build-standalone (PBS) distribution for use as RV's embedded
Python.

PBS ships a standard-ABI CPython as a self-contained relocatable tree. This
script massages a freshly extracted PBS "install_only" distribution so it
matches the layout and runtime expectations of the rest of the RV build:

1. Windows layout fix
   PBS places ``python311.dll`` / ``python.exe`` at the install root, whereas
   RV's CMake (python3.cmake) expects them under ``bin/``. We move the runtime
   binaries into ``bin/`` and leave import libraries in ``libs/``.

2. sitecustomize.py injection
   RV relies on a ``sitecustomize.py`` that points OpenSSL at certifi's CA
   bundle and reorders ``sys.path``. We copy RV's canonical sitecustomize.py
   into the distribution's site-packages.

3. macOS libpython install-name relocation
   PBS ships ``libpython3.11.dylib`` with an absolute build-time install id
   (``/install/lib/libpython3.11.dylib``). Anything RV links against it inherits
   that dead path and fails at load (dyld: Library not loaded). We rewrite the
   id to ``@rpath/libpython3.11.dylib`` to match RV's from-source convention
   (executables carry rpath ``@executable_path/../lib``).

4. PySide6 Qt repoint (--qt)
   PySide6 wheels bundle their own copy of Qt. RV shares Qt objects between its
   C++ application and PySide6 (see rv/qtutils.py wrapInstance), so a single Qt
   must serve both. We strip the wheel's bundled Qt runtime libraries and point
   PySide6 at RV's Qt (which MUST be the same Qt version as the wheel was built
   against). On macOS/Linux this is done with symlinks (no binary patching,
   using PySide6's existing ``@loader_path/Qt/lib`` / ``$ORIGIN/Qt/lib`` search
   path); on Windows the Qt DLLs are copied next to the PySide6 package.

This script is intentionally idempotent: re-running it on an already-adapted
tree is a no-op for each completed step.
"""

import argparse
import glob
import os
import platform
import shutil
import subprocess
import sys


ROOT_DIR = os.path.dirname(os.path.abspath(__file__))


def _log(msg: str) -> None:
    print(f"[adapt_pbs_python] {msg}", flush=True)


def get_site_packages(install_dir: str) -> str:
    """Locate the site-packages directory in a PBS install tree."""
    candidates = glob.glob(os.path.join(install_dir, "**", "site-packages"), recursive=True)
    if not candidates:
        raise FileNotFoundError(f"No site-packages directory found under {install_dir}")
    # Shortest path wins (the real stdlib site-packages, not a nested venv).
    return sorted(candidates, key=len)[0]


def fix_windows_layout(install_dir: str) -> None:
    """Move python*.dll / python*.exe from the install root into bin/.

    PBS Windows install_only places these at the install root. RV's CMake
    (python3.cmake, Windows branch) resolves the interpreter and shared library
    from ``bin/``.
    """
    if platform.system() != "Windows":
        return

    bin_dir = os.path.join(install_dir, "bin")
    os.makedirs(bin_dir, exist_ok=True)

    # Runtime binaries RV expects under bin/. We copy (not move) the DLLs so
    # that anything still resolving them from the root keeps working, but move
    # the executables to avoid duplicate interpreters on PATH.
    patterns_copy = ["python*.dll", "vcruntime*.dll"]
    patterns_move = ["python.exe", "pythonw.exe", "python3.dll"]

    for pattern in patterns_copy:
        for src in glob.glob(os.path.join(install_dir, pattern)):
            dst = os.path.join(bin_dir, os.path.basename(src))
            if not os.path.exists(dst):
                _log(f"copy {os.path.basename(src)} -> bin/")
                shutil.copy2(src, dst)

    for pattern in patterns_move:
        for src in glob.glob(os.path.join(install_dir, pattern)):
            dst = os.path.join(bin_dir, os.path.basename(src))
            if not os.path.exists(dst):
                _log(f"move {os.path.basename(src)} -> bin/")
                shutil.copy2(src, dst)


def inject_sitecustomize(install_dir: str) -> None:
    """Install RV's canonical sitecustomize.py into site-packages."""
    template = os.path.join(ROOT_DIR, "sitecustomize.py")
    if not os.path.exists(template):
        raise FileNotFoundError(f"sitecustomize.py template not found: {template}")

    site_packages = get_site_packages(install_dir)
    dst = os.path.join(site_packages, "sitecustomize.py")
    _log(f"install sitecustomize.py -> {dst}")
    shutil.copyfile(template, dst)


def relocate_macos_libpython(install_dir: str) -> None:
    """Rewrite libpython's install id from PBS's absolute build path to @rpath.

    PBS's ``lib/libpython3.11.dylib`` has ``LC_ID_DYLIB`` set to
    ``/install/lib/libpython3.11.dylib``. When RV links against this dylib the
    linker records that dead absolute path, so RV's executables fail at load
    with "dyld: Library not loaded: /install/lib/libpython3.11.dylib". RV's
    from-source build uses ``@rpath/libpython<ver>.dylib`` with an rpath of
    ``@executable_path/../lib`` on its executables; we match that here.

    macOS only. On Linux the ELF SONAME (libpython3.11.so.1.0) is already a
    relative name resolved via RUNPATH, and on Windows the DLL is referenced by
    name, so neither needs relocation. PBS's own bin/python3 references
    libpython via @executable_path/../lib, so it is unaffected by the id change.
    """
    if platform.system() != "Darwin":
        return

    candidates = glob.glob(os.path.join(install_dir, "lib", "libpython*.dylib"))
    # Skip symlinks; operate on the real dylib only.
    reals = [c for c in candidates if not os.path.islink(c)]
    if not reals:
        _log("macOS: no libpython dylib found to relocate")
        return

    for dylib in reals:
        basename = os.path.basename(dylib)
        new_id = f"@rpath/{basename}"

        current_id = subprocess.run(
            ["otool", "-D", dylib], capture_output=True, text=True, check=True
        ).stdout.splitlines()
        # otool -D prints the path on line 1 and the id on line 2.
        already = len(current_id) >= 2 and current_id[1].strip() == new_id
        if already:
            _log(f"macOS: {basename} id already {new_id}")
            continue

        subprocess.run(["install_name_tool", "-id", new_id, dylib], check=True)
        _log(f"macOS: relocated {basename} install id -> {new_id}")


def _pyside_dir(install_dir: str) -> str:
    site_packages = get_site_packages(install_dir)
    return os.path.join(site_packages, "PySide6")


def repoint_pyside_qt(install_dir: str, qt_dir: str) -> None:
    """Repoint PySide6's bundled Qt to RV's external Qt.

    :param qt_dir: RV's Qt root. The directory that (directly or via lib/)
        contains the Qt runtime libraries/frameworks. Must be the SAME Qt
        version PySide6 was built against.
    """
    pyside_dir = _pyside_dir(install_dir)
    if not os.path.isdir(pyside_dir):
        _log("PySide6 not present; skipping Qt repoint")
        return

    system = platform.system()
    if system == "Darwin":
        _repoint_qt_macos(pyside_dir, qt_dir)
    elif system == "Linux":
        _repoint_qt_linux(pyside_dir, qt_dir)
    elif system == "Windows":
        _repoint_qt_windows(pyside_dir, qt_dir)
    else:
        raise RuntimeError(f"Unsupported platform for Qt repoint: {system}")


def _resolve_qt_lib_dir(qt_dir: str, kind: str) -> str:
    """Find the directory holding Qt runtime libs within an RV Qt install.

    :param kind: one of "framework" (macOS), "so" (Linux), "dll" (Windows).
    """
    # Common Qt layouts: <qt>/lib (unix), <qt>/bin (windows dlls).
    search = {
        "framework": ["lib"],
        "so": ["lib"],
        "dll": ["bin"],
    }[kind]

    globs = {
        "framework": "QtCore.framework",
        "so": "libQt6Core.so*",
        "dll": "Qt6Core.dll",
    }[kind]

    for sub in [""] + search:
        candidate = os.path.join(qt_dir, sub) if sub else qt_dir
        if glob.glob(os.path.join(candidate, globs)):
            return candidate
    raise FileNotFoundError(f"Could not locate Qt {kind} libraries under {qt_dir} (looked for {globs})")


def _symlink_replace(src: str, dst: str) -> None:
    """Create/replace dst as a symlink to src."""
    if os.path.islink(dst) or os.path.exists(dst):
        if os.path.islink(dst):
            os.unlink(dst)
        elif os.path.isdir(dst):
            shutil.rmtree(dst)
        else:
            os.unlink(dst)
    os.symlink(src, dst)


def _repoint_qt_macos(pyside_dir: str, qt_dir: str) -> None:
    """macOS: symlink PySide6/Qt/lib/*.framework -> RV Qt frameworks.

    PySide6 abi3 modules reference Qt via ``@rpath/QtN.framework`` with an
    ``LC_RPATH`` of ``@loader_path/Qt/lib``. Replacing the framework entries in
    ``PySide6/Qt/lib`` with symlinks into RV's Qt keeps that search path intact
    with zero binary patching.
    """
    rv_qt_lib = _resolve_qt_lib_dir(qt_dir, "framework")
    wheel_qt_lib = os.path.join(pyside_dir, "Qt", "lib")
    os.makedirs(wheel_qt_lib, exist_ok=True)

    count = 0
    for fw in glob.glob(os.path.join(rv_qt_lib, "*.framework")):
        dst = os.path.join(wheel_qt_lib, os.path.basename(fw))
        _symlink_replace(os.path.abspath(fw), dst)
        count += 1
    _log(f"macOS: repointed {count} Qt frameworks -> {rv_qt_lib}")


def _repoint_qt_linux(pyside_dir: str, qt_dir: str) -> None:
    """Linux: symlink PySide6/Qt/lib/libQt6*.so* -> RV Qt shared objects.

    PySide6 abi3 modules use RUNPATH ``$ORIGIN/Qt/lib``. We replace the bundled
    Qt .so files with symlinks into RV's Qt lib directory.
    """
    rv_qt_lib = _resolve_qt_lib_dir(qt_dir, "so")
    wheel_qt_lib = os.path.join(pyside_dir, "Qt", "lib")
    os.makedirs(wheel_qt_lib, exist_ok=True)

    # Remove bundled Qt .so files, then symlink RV's.
    for existing in glob.glob(os.path.join(wheel_qt_lib, "libQt6*.so*")):
        if os.path.islink(existing) or os.path.isfile(existing):
            os.unlink(existing)

    count = 0
    for so in glob.glob(os.path.join(rv_qt_lib, "libQt6*.so*")):
        dst = os.path.join(wheel_qt_lib, os.path.basename(so))
        _symlink_replace(os.path.abspath(so), dst)
        count += 1
    _log(f"Linux: repointed {count} Qt shared objects -> {rv_qt_lib}")


def _repoint_qt_windows(pyside_dir: str, qt_dir: str) -> None:
    """Windows: replace PySide6's Qt6*.dll with RV's Qt DLLs.

    On Windows, PySide6 loads ``Qt6*.dll`` from the PySide6 package directory
    (added to the DLL search path by ``PySide6/__init__.py``). Symlinks are
    unreliable on Windows without privileges, so we copy RV's Qt DLLs over the
    bundled ones.
    """
    rv_qt_bin = _resolve_qt_lib_dir(qt_dir, "dll")

    # PySide6 Windows wheels put Qt DLLs directly in the PySide6 dir.
    count = 0
    for dll in glob.glob(os.path.join(rv_qt_bin, "Qt6*.dll")):
        dst = os.path.join(pyside_dir, os.path.basename(dll))
        # Only overwrite DLLs that the wheel actually shipped (keep addon DLLs
        # RV's Qt may not include from being deleted).
        if os.path.exists(dst):
            os.chmod(dst, 0o666)
            os.remove(dst)
        shutil.copy2(dll, dst)
        count += 1
    _log(f"Windows: repointed {count} Qt DLLs from {rv_qt_bin}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--install",
        required=True,
        help="Path to the extracted PBS install tree (contains bin/, lib/, ...)",
    )
    parser.add_argument(
        "--qt",
        required=False,
        help="Path to RV's Qt install to repoint PySide6 at (same Qt version). "
        "If omitted, the PySide6 Qt repoint step is skipped.",
    )
    parser.add_argument(
        "--skip-sitecustomize",
        action="store_true",
        help="Do not inject sitecustomize.py",
    )
    args = parser.parse_args()

    install_dir = os.path.abspath(args.install)
    if not os.path.isdir(install_dir):
        _log(f"ERROR: install dir does not exist: {install_dir}")
        return 1

    _log(f"adapting PBS distribution at {install_dir}")

    fix_windows_layout(install_dir)

    if not args.skip_sitecustomize:
        inject_sitecustomize(install_dir)

    relocate_macos_libpython(install_dir)

    if args.qt:
        repoint_pyside_qt(install_dir, os.path.abspath(args.qt))
    else:
        _log("no --qt provided; skipping PySide6 Qt repoint")

    _log("done")
    return 0


if __name__ == "__main__":
    sys.exit(main())
