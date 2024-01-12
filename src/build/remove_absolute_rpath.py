#!/usr/bin/env python

# *****************************************************************************
# Copyright (c) 2020 Autodesk, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************
import argparse
import subprocess
import os
import pathlib

from typing import Iterable


def get_object_files(target: pathlib.Path) -> Iterable[pathlib.Path]:
    if target.is_dir():
        for root, dirs, files in os.walk(target):
            for f in files:
                path = pathlib.Path(root) / f

                file_type = subprocess.check_output(["file", "-bh", path])
                if file_type.startswith(b"Mach-O") and b"dSYM" not in file_type:
                    yield path.absolute()
    else:
        file_type = subprocess.check_output(["file", "-bh", target])
        if file_type.startswith(b"Mach-O") and b"dSYM" not in file_type:
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


def fix_rpath(target: pathlib.Path, root: pathlib.Path):
    for file in get_object_files(target):
        try:
            print(f"Fixing rpaths for {file}")

            for rpath in get_rpaths(file):
                output = f"\trpath: {rpath}"

                if rpath.startswith("@") is False and rpath.startswith(".") is False:
                    delete_rpath(file, rpath)
                    output += " (Deleted)"

                print(output)

            print(f"Setting shared  library identification name for {file}")
            lib_id = set_id(file)

            print(f"\t{file} identification name: {lib_id}")

            print(f"Fixing shared library paths for {file}")

            for library_path in get_shared_library_paths(file):
                output = f"\t{file} library path: {library_path}"

                is_full_system_path = (
                    library_path.is_absolute()
                    and library_path.exists()
                    and not library_path.is_relative_to(root)
                )
                is_loose_library = (
                    not str(library_path).startswith("/")
                    and not str(library_path).startswith("@")
                    and not str(library_path).startswith(".")
                )

                if library_path != file.name and (
                    is_full_system_path or is_loose_library
                ):
                    new_library_path = change_shared_library_path(file, library_path)
                    output += f" (Changed to {new_library_path})"

                print(output)
        except subprocess.CalledProcessError as e:
            print(f"Error fixing rpath for {file}: {e}")
            print(f"Output: {e.output}")

            raise e


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--target", type=pathlib.Path, help="Target to sanitize", required=True
    )
    parser.add_argument(
        "--root", type=pathlib.Path, help="Root directory", required=True
    )

    args = parser.parse_args()

    if args.target.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.target.absolute()}")
    if args.root.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.root.absolute()}")

    # fail if target is not in root
    if args.target.resolve().relative_to(args.root.resolve()) is False:
        raise ValueError(
            f"Target {args.target.absolute()} is not in root {args.root.absolute()}"
        )

    fix_rpath(args.target.resolve().absolute(), args.root.resolve().absolute())
