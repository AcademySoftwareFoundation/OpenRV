#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
Test script to validate all Python package imports at build time.
This catches issues like missing dependencies, ABI incompatibilities, and
configuration problems (like OpenSSL legacy provider) before running the build.

Package list is automatically generated from requirements.txt.in to prevent
manual synchronization errors.
"""

import glob
import os
import platform
import re
import shutil
import sys

# Packages to skip from requirements.txt.in (e.g., build dependencies that don't need import testing).
SKIP_IMPORTS = [
    "pip",
    "setuptools",
    "wheel",
]

# Additional imports to test beyond what's in requirements.txt.in.
# These test deeper functionality (e.g., sub-modules that verify OpenSSL legacy provider support).
ADDITIONAL_IMPORTS = [
    "cryptography.fernet",
    "cryptography.hazmat.bindings._rust",
]


def parse_requirements(file_path):
    """Parse requirements.txt.in and return list of package names.

    Extracts package names from lines like:
    - numpy==@_numpy_version@
    - pip==24.0

    Skips comments and empty lines.
    """
    packages = []
    with open(file_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith("#"):
                continue
            # Extract package name (before ==, <, >, etc.)
            match = re.match(r"^([A-Za-z0-9_-]+)", line)
            if match:
                packages.append(match.group(1))
    return packages


def fix_opentimelineio_debug_windows():
    """Fix OpenTimelineIO Debug extension naming and DLL dependencies on Windows.

    In Debug builds, OTIO creates _opentimed.pyd but Python expects _opentime_d.pyd.
    This copies the file to the correct name if needed.

    Also ensures OTIO's DLL dependencies are accessible by adding lib directories to PATH.
    """
    if platform.system() != "Windows" or "_d.exe" not in sys.executable.lower():
        return  # Only needed for Windows Debug builds

    try:
        import site

        site_packages = site.getsitepackages()
        paths_to_add = []

        for sp in site_packages:
            otio_path = os.path.join(sp, "opentimelineio")
            if not os.path.exists(otio_path):
                continue

            # Add any OTIO lib directories to PATH for DLL loading
            # Check common locations for OTIO DLLs
            potential_lib_dirs = [
                otio_path,  # The package itself
                os.path.join(otio_path, "lib"),
                os.path.join(otio_path, ".libs"),
            ]

            for lib_dir in potential_lib_dirs:
                if os.path.exists(lib_dir) and lib_dir not in os.environ.get("PATH", ""):
                    # Check if there are DLL files in this directory
                    dll_files = glob.glob(os.path.join(lib_dir, "*.dll"))
                    if dll_files:
                        paths_to_add.append(lib_dir)

            # Look for ALL misnamed Debug extensions that end with 'd' before the Python tag
            # Examples: _opentimed.*.pyd, _otiod.*.pyd
            # These should be: _opentime_d.*.pyd, _otio_d.*.pyd
            all_pyd_files = glob.glob(os.path.join(otio_path, "*.pyd"))

            for pyd_file in all_pyd_files:
                basename = os.path.basename(pyd_file)

                # Check if it matches pattern: _<name>d.cp<version>-win_amd64.pyd
                # where <name> ends with 'd' but should be <name>_d
                if basename.startswith("_") and "d.cp" in basename and "_d.cp" not in basename:
                    # Extract the module name (between '_' and 'd.cp')
                    # Example: _opentimed.cp311 -> _opentime
                    parts = basename.split("d.cp")
                    if len(parts) == 2:
                        module_base = parts[0]  # e.g., "_opentime"

                        # Create correct name with _d suffix
                        correct_name = f"{module_base}_d.cp{parts[1]}"
                        correct_path = os.path.join(otio_path, correct_name)

                        if not os.path.exists(correct_path):
                            shutil.copy2(pyd_file, correct_path)
                            print(f"Fixed OTIO Debug extension: {basename} -> {correct_name}")

        # Add any discovered lib paths to PATH
        if paths_to_add:
            current_path = os.environ.get("PATH", "")
            new_path = os.pathsep.join(paths_to_add) + os.pathsep + current_path
            os.environ["PATH"] = new_path
            print(f"Added OTIO lib directories to PATH: {', '.join(paths_to_add)}")

    except Exception as e:
        # Don't fail if fix doesn't work, let import fail naturally
        print(f"Warning: Could not fix OTIO Debug naming: {e}")


def try_import(package_name):
    """Try importing package, with fallback to stripped 'Py' prefix.

    Handles cases where pip package name differs from import name:
    - PyOpenGL -> OpenGL
    - PyOpenGL_accelerate -> OpenGL_accelerate

    Raises ImportError if both attempts fail.
    """
    # Fix OpenTimelineIO Debug naming issue before importing
    if "opentimelineio" in package_name.lower():
        fix_opentimelineio_debug_windows()

        # On Windows Debug, add current directory to DLL search path
        # This helps Windows find any DLLs that might be in the package directory
        if platform.system() == "Windows" and "_d.exe" in sys.executable.lower():
            try:
                import site

                site_packages = site.getsitepackages()
                for sp in site_packages:
                    otio_path = os.path.join(sp, "opentimelineio")
                    if os.path.exists(otio_path):
                        # Add DLL search directory (Python 3.8+)
                        if hasattr(os, "add_dll_directory"):
                            try:
                                os.add_dll_directory(otio_path)
                            except (OSError, FileNotFoundError):
                                pass
            except Exception:
                pass  # Silently continue if this fails

    try:
        return __import__(package_name)
    except ImportError:
        if package_name.startswith("Py"):
            # Try without "Py" prefix
            return __import__(package_name[2:])
        raise


def find_msvc_runtime_dlls():
    """Find MSVC runtime DLL directories from Visual Studio installation.

    Returns list of directories containing MSVC runtime DLLs (both Release and Debug).
    Searches in both VC\Tools (build tools) and VC\Redist (redistributable packages).
    """
    msvc_dll_dirs = []

    # Common Visual Studio installation paths
    vs_paths = [
        r"C:\Program Files\Microsoft Visual Studio",
        r"C:\Program Files (x86)\Microsoft Visual Studio",
    ]

    for vs_base in vs_paths:
        if not os.path.exists(vs_base):
            continue

        try:
            # Look for VS versions (2022, 2019, etc.)
            for vs_year in os.listdir(vs_base):
                vs_year_path = os.path.join(vs_base, vs_year)
                if not os.path.isdir(vs_year_path):
                    continue

                # Look for editions (Enterprise, Professional, Community, BuildTools)
                for edition in ["Enterprise", "Professional", "Community", "BuildTools"]:
                    edition_path = os.path.join(vs_year_path, edition)
                    if not os.path.exists(edition_path):
                        continue

                    # 1. Check VC\Redist for Debug runtime DLLs (for Debug builds)
                    vc_redist = os.path.join(edition_path, "VC", "Redist", "MSVC")
                    if os.path.exists(vc_redist):
                        try:
                            msvc_versions = sorted(os.listdir(vc_redist), reverse=True)
                            for msvc_ver in msvc_versions[:3]:  # Check up to 3 latest versions
                                # Check Debug redistributables
                                debug_redist_paths = [
                                    os.path.join(
                                        vc_redist, msvc_ver, "debug_nonredist", "x64", "Microsoft.VC143.DebugCRT"
                                    ),
                                    os.path.join(
                                        vc_redist,
                                        msvc_ver,
                                        "onecore",
                                        "debug_nonredist",
                                        "x64",
                                        "Microsoft.VC143.DebugCRT",
                                    ),
                                ]
                                for debug_path in debug_redist_paths:
                                    if os.path.exists(debug_path):
                                        # Check for Debug runtime DLLs
                                        if glob.glob(os.path.join(debug_path, "*140d.dll")):
                                            if debug_path not in msvc_dll_dirs:
                                                msvc_dll_dirs.append(debug_path)
                                            break
                                if msvc_dll_dirs:
                                    break
                        except (OSError, PermissionError):
                            pass

                    # 2. Check VC\Tools for runtime DLLs (fallback for Release or if Redist not found)
                    vc_tools = os.path.join(edition_path, "VC", "Tools", "MSVC")
                    if os.path.exists(vc_tools):
                        try:
                            # Get the latest MSVC version
                            msvc_versions = sorted(os.listdir(vc_tools), reverse=True)
                            for msvc_ver in msvc_versions[:3]:  # Check up to 3 latest versions
                                # Check bin directories for runtime DLLs
                                bin_paths = [
                                    os.path.join(vc_tools, msvc_ver, "bin", "Hostx64", "x64"),
                                    os.path.join(vc_tools, msvc_ver, "bin", "Hostx86", "x64"),
                                ]
                                for bin_path in bin_paths:
                                    if os.path.exists(bin_path):
                                        # Check for MSVC runtime DLLs
                                        if glob.glob(os.path.join(bin_path, "msvcp140*.dll")) or glob.glob(
                                            os.path.join(bin_path, "vcruntime140*.dll")
                                        ):
                                            if bin_path not in msvc_dll_dirs:
                                                msvc_dll_dirs.append(bin_path)
                                            break
                                if msvc_dll_dirs and len(msvc_dll_dirs) >= 2:
                                    # Found both Redist and Tools, that's enough
                                    break
                        except (OSError, PermissionError):
                            pass

                    if msvc_dll_dirs:
                        break
                if msvc_dll_dirs:
                    break
        except (OSError, PermissionError):
            pass

    return msvc_dll_dirs


def test_imports():
    """Test that all required packages can be imported."""
    # On Windows, ensure Python bin directory is on PATH for DLL loading.
    # This is required for .pyd extensions (like _opentime.pyd) to find Python DLL and dependencies.
    if platform.system() == "Windows":
        python_bin = os.path.dirname(sys.executable)
        path_env = os.environ.get("PATH", "")
        paths_to_add = []

        if python_bin not in path_env:
            paths_to_add.append(python_bin)

        # For Python 3.11+, also check for DLLs directory.
        python_root = os.path.dirname(python_bin)
        dlls_dir = os.path.join(python_root, "DLLs")
        if os.path.exists(dlls_dir) and dlls_dir not in path_env:
            paths_to_add.append(dlls_dir)

        # For Debug builds, add additional directories for runtime DLLs
        if "_d.exe" in sys.executable.lower():
            # Add Python lib/libs directories
            for dir_name in ["lib", "libs"]:
                potential_dir = os.path.join(python_root, dir_name)
                if os.path.exists(potential_dir) and potential_dir not in path_env:
                    paths_to_add.append(potential_dir)

            # Look for MSVC runtime DLLs from Visual Studio installation
            # This is needed because OTIO Debug extensions link against MSVC runtime
            msvc_dirs = find_msvc_runtime_dlls()
            for msvc_dir in msvc_dirs:
                if msvc_dir not in path_env:
                    paths_to_add.append(msvc_dir)

            # Look for RV_DEPS directories that might contain MSVC DLLs
            # Check in RV_DEPS directory structure (for CI builds)
            build_root = python_root
            for _ in range(5):  # Go up max 5 levels to find _build directory
                build_root = os.path.dirname(build_root)
                if not build_root or build_root == os.path.dirname(build_root):
                    break

                # Check for RV_DEPS directories that might contain MSVC DLLs
                if os.path.exists(build_root):
                    # Look for any RV_DEPS_* directories that might have bin folders with DLLs
                    try:
                        for item in os.listdir(build_root):
                            if item.startswith("RV_DEPS_") and os.path.isdir(os.path.join(build_root, item)):
                                bin_dir = os.path.join(build_root, item, "install", "bin")
                                if os.path.exists(bin_dir) and bin_dir not in path_env:
                                    # Check if it has DLL files
                                    if glob.glob(os.path.join(bin_dir, "*.dll")):
                                        paths_to_add.append(bin_dir)
                    except (OSError, PermissionError):
                        pass

        if paths_to_add:
            # Add paths to the front of PATH.
            new_path = os.pathsep.join(paths_to_add) + os.pathsep + path_env
            os.environ["PATH"] = new_path
            print(f"Added to PATH for DLL loading: {', '.join(paths_to_add)}")

    # Get requirements.txt.in from same directory as this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    requirements_file = os.path.join(script_dir, "requirements.txt.in")

    if not os.path.exists(requirements_file):
        print(f"ERROR: Could not find {requirements_file}")
        return 1

    # Parse package list from requirements.txt.in
    packages = parse_requirements(requirements_file)

    # Build list of imports to test (packages from requirements.txt + additional imports)
    imports_to_test = []
    skipped_packages = []
    for package in packages:
        # Skip build dependencies and other packages that don't need import testing
        if package not in SKIP_IMPORTS:
            imports_to_test.append((package, "from requirements.txt"))
        else:
            skipped_packages.append(package)

    # Add any additional imports (e.g., sub-modules for deeper testing)
    for additional_import in ADDITIONAL_IMPORTS:
        imports_to_test.append((additional_import, "additional import"))

    failed_imports = []
    successful_imports = []

    print("=" * 80)
    print("Testing Python package imports at build time")
    print(f"Python: {sys.version}")
    print(f"Platform: {sys.platform}")
    print(f"Executable: {sys.executable}")
    print(f"Source: {requirements_file}")
    if skipped_packages:
        print(f"Skipping: {', '.join(skipped_packages)}")

    # On Windows, show PATH info for DLL debugging
    if platform.system() == "Windows":
        python_bin = os.path.dirname(sys.executable)
        print(f"Python bin directory: {python_bin}")
        path_env = os.environ.get("PATH", "")
        if python_bin in path_env:
            print("Python bin is on PATH: Yes")
        else:
            print("Python bin is on PATH: No (this should not happen - PATH was modified at startup)")

    print("=" * 80)
    print()

    for module_name, description in imports_to_test:
        try:
            print(f"Testing {module_name:45} ({description})...", end=" ")
            try_import(module_name)
            print("OK")
            successful_imports.append(module_name)
        except Exception as e:
            print("FAILED")
            print(f"  Error: {type(e).__name__}: {e}")
            failed_imports.append((module_name, e))

            # Keep failure output minimal; detailed diagnostics removed.

    print()
    print("=" * 80)
    print(f"Results: {len(successful_imports)} passed, {len(failed_imports)} failed")
    print("=" * 80)

    if failed_imports:
        print()
        print("FAILED IMPORTS:")
        for module_name, error in failed_imports:
            print(f"  - {module_name}: {type(error).__name__}: {error}")
        print()
        print("Build-time import test FAILED!")
        print("One or more required Python packages could not be imported.")
        print()
        if any("OpenSSL" in str(e) or "legacy" in str(e).lower() for _, e in failed_imports):
            print("NOTE: If you see OpenSSL legacy provider errors:")
            print(
                "  - Rebuild OpenSSL to generate openssl.cnf: ninja -t clean RV_DEPS_OPENSSL && ninja RV_DEPS_OPENSSL"
            )
            print("  - Check that OPENSSL_CONF is set in sitecustomize.py")
            print()

        if any("opentimelineio" in str(m).lower() for m, _ in failed_imports):
            print("NOTE: If you see OpenTimelineIO import errors:")
            print("  - Check that opentimelineio was built from source (not from wheel)")
            print("  - Verify CMAKE_ARGS were passed correctly to pip install")
            print("  - On Windows Debug builds: OTIO may link against Release runtime (msvcp140.dll)")
            print("    instead of Debug runtime (msvcp140d.dll), causing initialization failures")
            print("  - Try rebuilding: ninja -t clean RV_DEPS_PYTHON3 && ninja RV_DEPS_PYTHON3")
            print()
            print("To diagnose DLL dependencies, run:")
            print(f"  python {os.path.join(script_dir, 'check_pyd_dependencies.py')}")
            print()
        return 1
    else:
        print()
        print("All Python package imports successful!")
        return 0


if __name__ == "__main__":
    sys.exit(test_imports())
