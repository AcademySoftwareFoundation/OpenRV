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
import pathlib
import shutil
import subprocess
import platform
import tempfile
import uuid
from typing import List

SOURCE_DIR = ""
OUTPUT_DIR = ""
ARCH = ""
PERL_ROOT = ""


def get_openssl_args(root) -> List[str]:
    """
    Return the path to the openssl binary given a package root.

    :param root: OpenSSL package root

    :return: Path to the openssl binary
    """
    openssl_bins = glob.glob(
        os.path.join(root, "**", "bin", "openssl*"), recursive=True
    )

    print(openssl_bins)

    if not openssl_bins or os.path.exists(openssl_bins[0]) is False:
        raise FileNotFoundError()

    print(f"Found openssl binaries {openssl_bins}")

    openssl_bin = sorted(openssl_bins)[0]

    return [openssl_bin]


def patch_openssl_distribution() -> None:
    """
    Patch the OpenSSL package so it can be used standalone.
    """

    if platform.system() == "Darwin":
        print("Making the shared libraries relocatable")

        openssl_libs = glob.glob(os.path.join(OUTPUT_DIR, "lib", "lib*.dylib*"))
        openssl_libs = [lib for lib in openssl_libs if os.path.islink(lib) is False]

        openssl_bin = get_openssl_args(OUTPUT_DIR)[0]

        for rpath in ["@executable_path/../lib", "@executable_path/."]:
            print(f"Adding {rpath} as an rpath to the openssl binary")
            install_name_tool_add_args = [
                "install_name_tool",
                "-add_rpath",
                rpath,
                openssl_bin,
            ]

            print(f"Executing {install_name_tool_add_args}")
            subprocess.run(install_name_tool_add_args).check_returncode()

        for lib_path in openssl_libs:
            lib_name = os.path.basename(lib_path)

            print(f"Setting the 'id' to {lib_name}")
            install_name_tool_id_args = [
                "install_name_tool",
                "-id",
                f"@rpath/{lib_name}",
                lib_path,
            ]

            print(f"Executing {install_name_tool_id_args}")
            subprocess.run(install_name_tool_id_args).check_returncode()

            print(f"Changing the library path of {lib_name} in the OpenSSL binary")
            install_name_tool_change_args = [
                "install_name_tool",
                "-change",
                lib_path,
                f"@rpath/{lib_name}",
                openssl_bin,
            ]

            print(f"Executing {install_name_tool_change_args}")
            subprocess.run(install_name_tool_change_args).check_returncode()

            for dependencies_path in openssl_libs:
                if dependencies_path == lib_path:
                    continue

                dependencies_name = os.path.basename(dependencies_path)

                print(f"Changing the library path of {lib_name} in {dependencies_name}")
                install_name_tool_change_args = [
                    "install_name_tool",
                    "-change",
                    dependencies_path,
                    f"@rpath/{dependencies_name}",
                    lib_path,
                ]

                print(f"Executing {install_name_tool_change_args}")
                subprocess.run(install_name_tool_change_args).check_returncode()


def test_openssl_distribution() -> None:
    """
    Test the OpenSSL distribution.
    """
    tmp_dir = os.path.join(tempfile.gettempdir(), str(uuid.uuid4()))
    os.makedirs(tmp_dir)

    tmp_openssl_dir = os.path.join(tmp_dir, "OpenSSL")

    try:
        print(f"Moving {OUTPUT_DIR} to {tmp_openssl_dir}")
        shutil.move(OUTPUT_DIR, tmp_openssl_dir)

        openssl_bin = get_openssl_args(tmp_openssl_dir)

        print(f"Executing {openssl_bin}")
        subprocess.run(
            openssl_bin,
            input=b"exit\n",
        ).check_returncode()
    finally:
        print(f"Moving {tmp_openssl_dir} to {OUTPUT_DIR}")
        shutil.move(tmp_openssl_dir, OUTPUT_DIR)


def clean() -> None:
    """
    Run the clean step of the build. Removes everything.
    """

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)

    git_clean_command = ["git", "clean", "-dxff"]

    print(f"Executing {git_clean_command} from {SOURCE_DIR}")
    subprocess.run(git_clean_command, cwd=SOURCE_DIR)


def configure() -> None:
    """
    Run the configure step of the build. It builds the makefile required to build OpenSSL with the path correctly set.
    """
    if os.path.exists(OUTPUT_DIR) is not True:
        os.makedirs(OUTPUT_DIR)

    configure_args = [
        os.path.join(PERL_ROOT, "perl"),
        os.path.join(SOURCE_DIR, "Configure"),
    ]

    if platform.system() == "Darwin":
        if ARCH:
            configure_args.append(f"darwin64{ARCH}-cc")
        else:
            configure_args.append("darwin64-x86_64-cc")
    elif platform.system() == "Windows":
        configure_args.append("VC-WIN64A")
        configure_args.append("no-asm")
    else:
        configure_args.append("linux-x86_64")

    configure_args.append(f"--prefix={OUTPUT_DIR}")
    configure_args.append(f"--openssldir={OUTPUT_DIR}")

    if platform.system() == "Linux":
        configure_args.append("-Wl,-rpath,'$$ORIGIN/../lib'")

    print(f"Executing {configure_args} from {SOURCE_DIR}")

    subprocess.run(configure_args, cwd=SOURCE_DIR).check_returncode()


def build() -> None:
    """
    Run the build step of the build. It compile every target of the project.
    """
    if platform.system() == "Windows":
        build_args = ["nmake"]
    else:
        build_args = ["make", f"-j{os.cpu_count() or 1}"]

    print(f"Executing {build_args} from {SOURCE_DIR}")
    subprocess.run(build_args, cwd=SOURCE_DIR).check_returncode()


def install() -> None:
    """
    Run the install step of the build. It puts all the files inside of the output directory and make them ready to be
    packaged.
    """
    if platform.system() == "Windows":
        install_args = ["nmake", "install_sw"]
    else:
        install_args = ["make", "install_sw", f"-j{os.cpu_count() or 1}"]

    print(f"Executing {install_args} from {SOURCE_DIR}")
    subprocess.run(install_args, cwd=SOURCE_DIR).check_returncode()

    patch_openssl_distribution()
    test_openssl_distribution()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--clean", dest="clean", action="store_true")
    parser.add_argument("--configure", dest="configure", action="store_true")
    parser.add_argument("--build", dest="build", action="store_true")
    parser.add_argument("--install", dest="install", action="store_true")

    parser.add_argument("--source-dir", dest="source", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", dest="output", type=pathlib.Path, required=True)

    parser.add_argument("--arch", dest="arch", type=str, required=False, default="")
    parser.add_argument(
        "--perlroot", dest="perlroot", type=str, required=False, default=""
    )

    parser.set_defaults(clean=False, configure=False, build=False, install=False)

    args = parser.parse_args()
    print(args)

    SOURCE_DIR = args.source
    OUTPUT_DIR = args.output
    ARCH = args.arch
    PERL_ROOT = args.perlroot

    if args.clean:
        clean()

    if args.configure:
        configure()

    if args.build:
        build()

    if args.install:
        install()
