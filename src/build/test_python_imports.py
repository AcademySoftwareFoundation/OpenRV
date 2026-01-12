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
configuration problems (like OpenSSL legacy provider) before runtime.

Package list is automatically generated from requirements.txt.in to prevent
manual synchronization errors.
"""

import os
import platform
import re
import sys
import traceback

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


def try_import(package_name):
    """Try importing package, with fallback to stripped 'Py' prefix.

    Handles cases where pip package name differs from import name:
    - PyOpenGL -> OpenGL
    - PyOpenGL_accelerate -> OpenGL_accelerate

    Raises ImportError if both attempts fail.
    """
    try:
        return __import__(package_name)
    except ImportError:
        if package_name.startswith("Py"):
            # Try without "Py" prefix
            return __import__(package_name[2:])
        raise


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

            # For opentimelineio failures, provide diagnostics
            if "opentimelineio" in module_name.lower():
                print("  Diagnostic info for OpenTimelineIO:")
                try:
                    import site

                    site_packages = site.getsitepackages()
                    print(f"  Site-packages: {site_packages}")

                    # Check if opentimelineio is installed
                    for sp in site_packages:
                        otio_path = os.path.join(sp, "opentimelineio")
                        if os.path.exists(otio_path):
                            print(f"  Found opentimelineio at: {otio_path}")
                            # List contents
                            contents = os.listdir(otio_path)
                            print(f"  Contents: {', '.join(contents[:10])}")
                            # Check for _opentime
                            opentime_files = [f for f in contents if "_opentime" in f]
                            if opentime_files:
                                print(f"  _opentime files: {opentime_files}")
                            else:
                                print("  WARNING: No _opentime extension found!")
                except Exception as diag_e:
                    print(f"  Could not get diagnostic info: {diag_e}")

            # Print full traceback for debugging
            if "--verbose" in sys.argv:
                print("  Traceback:")
                traceback.print_exc(file=sys.stdout)

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
            print("  - On Windows, check that the C++ extension (_opentime.pyd) was built")
            print("  - Try rebuilding: ninja -t clean RV_DEPS_PYTHON3 && ninja RV_DEPS_PYTHON3")
            print()
        return 1
    else:
        print()
        print("All Python package imports successful!")
        return 0


if __name__ == "__main__":
    sys.exit(test_imports())
