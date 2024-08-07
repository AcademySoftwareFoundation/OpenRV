#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2020 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

import argparse
import glob
import os
import re
import pathlib
import shutil
import subprocess
import platform
import tempfile
import uuid

from utils import (
    download_file,
    extract_7z_archive,
    verify_7z_archive,
    source_widows_msvc_env,
    update_env_path,
)
from make_python import get_python_interpreter_args

SOURCE_DIR = ""
OUTPUT_DIR = ""
TEMP_DIR = ""
VARIANT = ""

QT_OUTPUT_DIR = ""
PYTHON_OUTPUT_DIR = ""
OPENSSL_OUTPUT_DIR = ""

LIBCLANG_URL_BASE = "https://mirrors.ocf.berkeley.edu/qt/development_releases/prebuilt/libclang/libclang-release_"


def test_python_distribution(python_home: str) -> None:
    """
    Test the Python distribution.

    :param python_home: Package root of an Python package
    """
    tmp_dir = os.path.join(tempfile.gettempdir(), str(uuid.uuid4()))
    os.makedirs(tmp_dir)

    tmp_python_home = os.path.join(tmp_dir, os.path.basename(python_home))
    try:
        print(f"Moving {python_home} to {tmp_python_home}")
        shutil.move(python_home, tmp_python_home)

        python_validation_args = get_python_interpreter_args(tmp_python_home) + [
            "-c",
            "\n".join(
                [
                    "from PySide2 import *",
                    "from PySide2 import QtWebEngineCore, QtWebEngine",
                ]
            ),
        ]
        print(f"Validating the PySide package with {python_validation_args}")
        subprocess.run(python_validation_args).check_returncode()
    finally:
        print(f"Moving {tmp_python_home} to {python_home}")
        shutil.move(tmp_python_home, python_home)


def prepare() -> None:
    """
    Run the clean step of the build. Removes everything.
    """
    if os.path.exists(TEMP_DIR) is False:
        os.makedirs(TEMP_DIR)

    # PySide2 5.15.x recommends building with clang version 8.
    # But clang 8 headers are not compatible with Mac SDK 13.3+ headers.
    # To workaround it, since Mac is clang-based, we'll detect the OS clang
    # version and download the matching headers to build PySide.
    clang_filename_suffix = ""

    system = platform.system()
    if system == "Darwin":
        clang_version_search = re.search(
            "version (\d+)\.(\d+)\.(\d+)",
            os.popen("clang --version").read(),
        )
        clang_version_str = ".".join(clang_version_search.groups())
        clang_filename_suffix = clang_version_str + "-based-macos-universal.7z"
    elif system == "Linux":
        clang_filename_suffix = "80-based-linux-Rhel7.2-gcc5.3-x86_64.7z"
    elif system == "Windows":
        clang_filename_suffix = "140-based-windows-vs2019_64.7z"

    download_url = LIBCLANG_URL_BASE + clang_filename_suffix
    libclang_zip = os.path.join(TEMP_DIR, "libclang.7z")
    if os.path.exists(libclang_zip) is False:
        download_file(download_url, libclang_zip)
    # if we have a failed download, clean it up and redownload.
    # checking for False since it can return None when archive doesn't have a CRC
    elif verify_7z_archive(libclang_zip) is False:
        os.remove(libclang_zip)
        download_file(download_url, libclang_zip)

    # clean up previous failed extraction
    libclang_tmp = os.path.join(TEMP_DIR, "libclang-tmp")
    if os.path.exists(libclang_tmp) is True:
        shutil.rmtree(libclang_tmp)

    # extract to a temp location and only move if successful
    libclang_extracted = os.path.join(TEMP_DIR, "libclang")
    if os.path.exists(libclang_extracted) is False:
        extract_7z_archive(libclang_zip, libclang_tmp)
        shutil.move(libclang_tmp, libclang_extracted)

    libclang_install_dir = os.path.join(libclang_extracted, "libclang")

    if OPENSSL_OUTPUT_DIR:
        os.environ["PATH"] = os.path.pathsep.join(
            [
                os.path.join(OPENSSL_OUTPUT_DIR, "bin"),
                os.environ.get("PATH", ""),
            ]
        )

    print(f"PATH={os.environ['PATH']}")

    os.environ["LLVM_INSTALL_DIR"] = libclang_install_dir
    os.environ["CLANG_INSTALL_DIR"] = libclang_install_dir

    # PySide2 build requires a version of numpy lower than 1.23
    install_numpy_args = get_python_interpreter_args(PYTHON_OUTPUT_DIR) + [
        "-m",
        "pip",
        "install",
        "numpy<1.23",
    ]
    print(f"Installing numpy with {install_numpy_args}")
    subprocess.run(install_numpy_args).check_returncode()

    cmakelist_path = os.path.join(
        SOURCE_DIR, "sources", "shiboken2", "ApiExtractor", "CMakeLists.txt"
    )
    old_cmakelist_path = os.path.join(
        SOURCE_DIR, "sources", "shiboken2", "ApiExtractor", "CMakeLists.txt.old"
    )
    if os.path.exists(old_cmakelist_path):
        os.remove(old_cmakelist_path)

    os.rename(cmakelist_path, old_cmakelist_path)
    with open(old_cmakelist_path) as old_cmakelist:
        with open(cmakelist_path, "w") as cmakelist:
            for line in old_cmakelist:
                new_line = line.replace(
                    " set(HAS_LIBXSLT 1)",
                    " #set(HAS_LIBXSLT 1)",
                )

                cmakelist.write(new_line)


def remove_broken_shortcuts(python_home: str) -> None:
    """
    Remove broken Python shortcuts that depend on the absolute
    location of the Python executable.

    Note that this method will also remove scripts like
    pip, easy_install and wheel that were left around by
    previous steps of the build pipeline.

    :param str python_home: Path to the Python folder.
    :param int version: Version of the python executable.
    """
    if platform.system() == "Windows":
        # All executables inside Scripts have a hardcoded
        # absolute path to the python, which can't be relied
        # upon, so remove all scripts.
        shutil.rmtree(os.path.join(python_home, "Scripts"))
    else:
        # Aside from the python executables, every other file
        # in the build is a script that does not support
        bin_dir = os.path.join(python_home, "bin")
        for filename in os.listdir(bin_dir):
            filepath = os.path.join(bin_dir, filename)
            if filename not in [
                "python",
                "python3",
                f"python{PYTHON_VERSION}",
            ]:
                print(f"Removing {filepath}...")
                os.remove(filepath)
            else:
                print(f"Keeping {filepath}...")


def build() -> None:
    """
    Run the build step of the build. It compile every target of the project.
    """
    python_home = PYTHON_OUTPUT_DIR
    python_interpreter_args = get_python_interpreter_args(python_home)

    pyside_build_args = python_interpreter_args + [
        os.path.join(SOURCE_DIR, "setup.py"),
        "install",
        f"--qmake={os.path.join(QT_OUTPUT_DIR, 'bin', 'qmake' + ('.exe' if platform.system() == 'Windows' else ''))}",
        "--ignore-git",
        "--standalone",
        "--verbose-build",
        f"--parallel={os.cpu_count() or 1}",
        "--skip-docs",
    ]

    if OPENSSL_OUTPUT_DIR:
        pyside_build_args.append(f"--openssl={os.path.join(OPENSSL_OUTPUT_DIR, 'bin')}")

    # PySide2 v5.15.2.1 builds with errors on Windows using Visual Studio 2019.
    # We force Visual Studio 2017 here to make it build without errors.
    if platform.system() == "Windows":
        # Add Qt jom to the path to build in parallel
        jom_path = os.path.abspath(
            os.path.join(QT_OUTPUT_DIR, "..", "..", "Tools", "QtCreator", "bin", "jom")
        )
        if os.path.exists(os.path.join(jom_path, "jom.exe")):
            print(f"jom.exe was successfully located at: {jom_path}")
            update_env_path([jom_path])
        else:
            print(f"Could not find jom.exe at the expected location: {jom_path}")
            print(f"Build performance might be impacted")

        # Add the debug switch to match build type but only on Windows
        # (on other platforms, PySide2 is built in release)
        if VARIANT == "Debug":
            pyside_build_args.append("--debug")

    print(f"Executing {pyside_build_args}")
    subprocess.run(pyside_build_args).check_returncode()

    generator_cleanup_args = python_interpreter_args + [
        "-m",
        "pip",
        "uninstall",
        "-y",
        "shiboken2_generator",
    ]

    print(f"Executing {generator_cleanup_args}")
    subprocess.run(generator_cleanup_args).check_returncode()

    # Even if we remove shiboken2_generator from pip, the files stays... for some reasons
    generator_cleanup_args = python_interpreter_args + [
        "-c",
        "\n".join(
            [
                "import os, shutil",
                "try:",
                "  import shiboken2_generator",
                "except:",
                "  exit(0)",
                "shutil.rmtree(os.path.dirname(shiboken2_generator.__file__))",
            ]
        ),
    ]

    print(f"Executing {generator_cleanup_args}")
    subprocess.run(generator_cleanup_args).check_returncode()

    if OPENSSL_OUTPUT_DIR and platform.system() == "Windows":
        pyside_folder = glob.glob(
            os.path.join(python_home, "**", "site-packages", "PySide2"), recursive=True
        )[0]
        openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "bin", "lib*"))

        for lib in openssl_libs:
            print(f"Copying {lib} into {pyside_folder}")
            shutil.copy(lib, pyside_folder)

    remove_broken_shortcuts(python_home)
    test_python_distribution(python_home)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--prepare", dest="prepare", action="store_true")
    parser.add_argument("--build", dest="build", action="store_true")

    parser.add_argument("--source-dir", dest="source", type=pathlib.Path, required=True)
    parser.add_argument("--python-dir", dest="python", type=pathlib.Path, required=True)
    parser.add_argument("--qt-dir", dest="qt", type=pathlib.Path, required=True)
    parser.add_argument(
        "--openssl-dir", dest="openssl", type=pathlib.Path, required=False
    )
    parser.add_argument("--temp-dir", dest="temp", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", dest="output", type=pathlib.Path, required=True)

    parser.add_argument("--variant", dest="variant", type=str, required=True)


    # Major and minor version with dots.
    parser.add_argument("--python-version", dest="python_version", type=str, required=True, default="")

    parser.set_defaults(prepare=False, build=False)

    args = parser.parse_args()

    SOURCE_DIR = args.source
    OUTPUT_DIR = args.output
    TEMP_DIR = args.temp
    OPENSSL_OUTPUT_DIR = args.openssl
    PYTHON_OUTPUT_DIR = args.python
    QT_OUTPUT_DIR = args.qt
    VARIANT = args.variant
    PYTHON_VERSION = args.python_version
    
    print(args)

    if args.prepare:
        prepare()

    if args.build:
        build()
