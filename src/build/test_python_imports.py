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
"""

import sys
import traceback

# List of all packages that should be importable
REQUIRED_IMPORTS = [
    # Core packages
    ("pip", "Package installer"),
    ("setuptools", "Build system"),
    ("wheel", "Wheel format support"),
    # Scientific/Data packages
    ("numpy", "Numerical computing"),
    ("opentimelineio", "Editorial timeline"),
    # Graphics/OpenGL
    ("OpenGL", "PyOpenGL - OpenGL bindings"),
    ("OpenGL_accelerate", "PyOpenGL acceleration"),
    # SSL/Security
    ("certifi", "SSL certificate bundle"),
    ("cryptography", "Cryptographic primitives"),
    ("cryptography.fernet", "Fernet encryption (requires OpenSSL legacy provider)"),
    ("cryptography.hazmat.bindings._rust", "Rust bindings (tests OpenSSL provider loading)"),
    # HTTP/Network
    ("requests", "HTTP library"),
    # Utilities
    ("six", "Python 2/3 compatibility"),
    ("packaging", "Version parsing"),
    ("pydantic", "Data validation"),
]


def test_imports():
    """Test that all required packages can be imported."""
    failed_imports = []
    successful_imports = []

    print("=" * 80)
    print("Testing Python package imports at build time")
    print("=" * 80)
    print()

    for module_name, description in REQUIRED_IMPORTS:
        try:
            print(f"Testing {module_name:40} ({description})...", end=" ")
            __import__(module_name)
            print("  OK")
            successful_imports.append(module_name)
        except Exception as e:
            print("  FAILED")
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
        print("All Python package imports successful! ✓")
        return 0


if __name__ == "__main__":
    sys.exit(test_imports())
