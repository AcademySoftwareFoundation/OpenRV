#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2020 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
Test script for validating a Python distribution built by make_python.py.

This script validates that the Python distribution is correctly built,
relocatable, and has all necessary dependencies installed.
"""

import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import uuid

# Import the helper function from make_python.py (same directory)
from make_python import get_python_interpreter_args


def test_python_distribution(python_home: str, variant: str) -> None:
    """
    Test the Python distribution.

    This test validates:
    - The distribution is relocatable (works when moved to a temp directory)
    - Can install wheels (cryptography package)
    - All critical modules are available (ssl, tkinter, certifi, etc.)
    - SSL_CERT_FILE environment variable is properly set
    - Respects user-provided SSL_CERT_FILE
    - Respects DO_NOT_SET_SSL_CERT_FILE flag

    :param python_home: Package root of a Python package
    :param variant: Build variant (Debug or Release)
    """
    tmp_dir = os.path.join(tempfile.gettempdir(), str(uuid.uuid4()))
    os.makedirs(tmp_dir)

    tmp_python_home = os.path.join(tmp_dir, os.path.basename(python_home))
    try:
        print(f"Moving {python_home} to {tmp_python_home}")
        shutil.move(python_home, tmp_python_home)

        python_interpreter_args = get_python_interpreter_args(tmp_python_home, variant)

        # Note: OpenTimelineIO is installed via requirements.txt for all platforms and build types.
        # The git URL in requirements.txt ensures it builds from source with proper linkage.

        wheel_install_arg = python_interpreter_args + [
            "-m",
            "pip",
            "install",
            "cryptography",
        ]

        print(f"Validating that we can install a wheel with {wheel_install_arg}")
        subprocess.run(wheel_install_arg).check_returncode()

        python_validation_args = python_interpreter_args + [
            "-c",
            "\n".join(
                [
                    # Check for tkinter
                    "try:",
                    "    import tkinter",
                    "except:",
                    "    import Tkinter as tkinter",
                    # Make sure certifi is available
                    "import certifi",
                    # Make sure the SSL_CERT_FILE variable is set
                    "import os",
                    "assert certifi.where() == os.environ['SSL_CERT_FILE']",
                    # Make sure ssl is correctly built and linked
                    "import ssl",
                    # Misc
                    "import sqlite3",
                    "import ctypes",
                    "import ssl",
                    "import _ssl",
                    "import zlib",
                ]
            ),
        ]
        print(f"Validating the python package with {python_validation_args}")
        subprocess.run(python_validation_args).check_returncode()

        dummy_ssl_file = os.path.join("Path", "To", "Dummy", "File")
        python_validation2_args = python_interpreter_args + [
            "-c",
            "\n".join(
                [
                    "import os",
                    f"assert os.environ['SSL_CERT_FILE'] == '{dummy_ssl_file}'",
                ]
            ),
        ]
        print(f"Validating the python package with {python_validation2_args}")
        subprocess.run(python_validation2_args, env={**os.environ, "SSL_CERT_FILE": dummy_ssl_file}).check_returncode()

        python_validation3_args = python_interpreter_args + [
            "-c",
            "\n".join(
                [
                    "import os",
                    "assert 'SSL_CERT_FILE' not in os.environ",
                ]
            ),
        ]
        print(f"Validating the python package with {python_validation3_args}")
        subprocess.run(
            python_validation3_args,
            env={**os.environ, "DO_NOT_SET_SSL_CERT_FILE": "bleh"},
        ).check_returncode()

        print("All Python distribution tests passed successfully!")

    finally:
        print(f"Moving {tmp_python_home} back to {python_home}")
        shutil.move(tmp_python_home, python_home)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Test a Python distribution built by make_python.py"
    )

    parser.add_argument(
        "--python-home",
        dest="python_home",
        type=pathlib.Path,
        required=True,
        help="Path to the Python installation directory to test"
    )
    parser.add_argument(
        "--variant",
        dest="variant",
        type=str,
        required=True,
        choices=["Debug", "Release"],
        help="Build variant (Debug or Release)"
    )

    args = parser.parse_args()

    if not os.path.exists(args.python_home):
        print(f"Error: Python home directory does not exist: {args.python_home}", file=sys.stderr)
        sys.exit(1)

    try:
        test_python_distribution(args.python_home, args.variant)
        print("\n✓ Python distribution validation completed successfully")
        sys.exit(0)
    except Exception as e:
        print(f"\n✗ Python distribution validation failed: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)

