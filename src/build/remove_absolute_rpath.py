#!/usr/bin/env python

# *****************************************************************************
# Copyright (c) 2020 Autodesk, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************
import argparse
import logging
import os
import pathlib
import subprocess

from typing import Iterable

# Configure logging.
logging.basicConfig(format='-- %(message)s')
logger = logging.getLogger().setLevel(logging.INFO)

def threadsafe_print(message: str):
    """
    Print a message in a thread safe manner.

    :param message: The message to print.

    :return: None
    """
    logging.info(message)

def is_mach_o_file(path: pathlib.Path) -> bool:
    file_type = subprocess.check_output(["file", "-bh", str(path)])
    return file_type.startswith(b"Mach-O") and b"dSYM" not in file_type

def get_object_files(target: pathlib.Path) -> Iterable[pathlib.Path]:
    if target.is_dir():
        for root, dirs, files in os.walk(target):
            for f in files:
                path = pathlib.Path(root) / f
                if is_mach_o_file(path):
                    yield path.absolute()
    elif is_mach_o_file(target):
        yield target.absolute()


def get_rpaths(object_file_path: pathlib.Path) -> Iterable[str]:
    otool_output = (
        subprocess.check_output(["otool", "-l", str(object_file_path)])
        .decode()
        .splitlines()
    )

    i = 0
    while i < len(otool_output):
        otool_line = otool_output[i].strip()
        i += 1

        if otool_line == "":
            continue

        if otool_line.split()[-1] == "LC_RPATH":
            for y in range(i, i + 2):
                rpath_line = otool_output[y].split()

                if rpath_line[0] == "path":
                    yield rpath_line[1]
                    break


def set_id(object_file_path: pathlib.Path) -> str:
    new_library_id = f"@rpath/{object_file_path.name}"

    set_id_command = [
        "install_name_tool",
        "-id",
        new_library_id,
        str(object_file_path),
    ]
    subprocess.run(set_id_command).check_returncode()

    return new_library_id


def get_shared_library_paths(object_file_path: pathlib.Path) -> Iterable[pathlib.Path]:
    otool_output = (
        subprocess.check_output(["otool", "-L", object_file_path]).decode().splitlines()
    )

    i = 0

    while i < len(otool_output):
        otool_line = otool_output[i].strip()
        i += 1
        if "(compatibility version" in otool_line:
            yield pathlib.Path(otool_line.split("(compatibility")[0].strip())


def delete_rpath(object_file_path: pathlib.Path, rpath: str):
    delete_rpath_command = [
        "install_name_tool",
        "-delete_rpath",
        str(rpath),
        object_file_path,
    ]
    subprocess.run(delete_rpath_command).check_returncode()


def change_shared_library_path(object_file_path, old_library_path):
    new_library_path = f"@rpath/{os.path.basename(old_library_path)}"

    change_shared_library_path_command = [
        "install_name_tool",
        "-change",
        old_library_path,
        new_library_path,
        object_file_path,
    ]
    subprocess.run(change_shared_library_path_command).check_returncode()

    return new_library_path


def fix_rpath(target: pathlib.Path, build_root: pathlib.Path):
    for file in get_object_files(target):
        try:
            logging.info(f"Fixing rpaths for {file}")

            for rpath in get_rpaths(file):
                output = f"\trpath: {rpath}"

                if rpath.startswith("@") is False and rpath.startswith(".") is False:
                    delete_rpath(file, rpath)
                    output += " (Deleted)"

                logging.debug(output)

            logging.debug(f"Setting shared library identification name for {file}")
            lib_id = set_id(file)

            logging.debug(f"\t{file} identification name: {lib_id}")

            logging.info(f"Fixing shared library paths for {file}")

            for library_path in get_shared_library_paths(file):
                output = f"\t{file} library path: {library_path}"

                # Fix entries containing only library name and paths to the build directory.
                # Do not change entries coming from system location or other paths.

                # e.g. 
                #       /usr/lib/abc.dylib (do no change this rpath)
                #          is_outside_of_build_folder   = True
                #          is_from_build_folder         = False
                #          is_looser_library            = False
                #
                #       /.../_build/lib/abc.dylib (change this rpath)
                #          is_outside_of_build_folder   = False
                #          is_from_build_folder         = True
                #          is_loose_library             = False
                #
                #       abc.dylib (change this rpath)
                #          is_outside_of_build_folder   = False
                #          is_from_build_folder         = False
                #          is_loose_library             = True
                #
                # With the current build system, other paths than system paths for libraries should not happen.
                is_from_build_folder = (
                    library_path.is_absolute() 
                    and library_path.exists() 
                    and library_path.is_relative_to(build_root)
                )

                is_loose_library = (
                    not str(library_path).startswith("/")
                    and not str(library_path).startswith("@")
                    and not str(library_path).startswith(".")
                )

                if library_path != file.name and (is_loose_library or is_from_build_folder):
                    new_library_path = change_shared_library_path(file, library_path)
                    output += f" (Changed to {new_library_path})"

                logging.debug(output)
        except subprocess.CalledProcessError as e:
            logging.error(f"Error fixing rpath for {file}: {e}")
            logging.error(f"Output: {e.output}")

            raise e


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--file", type=pathlib.Path, help="File to sanitize", required=True
    )
    parser.add_argument(
        "--build-root", type=pathlib.Path, help="Build directory", required=True
    )

    args = parser.parse_args()

    if args.file.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.file.absolute()}")
    if args.build_root.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.build_root.absolute()}")

    # Fail if file is not in build_root
    if args.file.resolve().relative_to(args.build_root.resolve()) is False:
        raise ValueError(
            f"File {args.file.absolute()} is not in build_root {args.build_root.absolute()}"
        )

    fix_rpath(args.file.resolve().absolute(), args.build_root.resolve().absolute())
