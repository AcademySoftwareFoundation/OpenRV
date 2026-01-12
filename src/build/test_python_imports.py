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
    print(f"Source: {requirements_file}")
    if skipped_packages:
        print(f"Skipping: {', '.join(skipped_packages)}")
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
        return 1
    else:
        print()
        print("All Python package imports successful!")
        return 0


if __name__ == "__main__":
    sys.exit(test_imports())
