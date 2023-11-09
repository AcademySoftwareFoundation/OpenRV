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

from pathlib import Path


def get_object_files(target):
    target = Path(target)

    if target.is_dir():
        for root, dirs, files in os.walk(target):
            for f in files:
                path = Path(root) / f

                file_type = subprocess.check_output(["file", "-bh", path])
                if file_type.startswith(b"Mach-O"):
                    yield str(path.absolute())
    else:
        yield target


def get_rpaths(object_file_path):
    otool_output = subprocess.check_output(
        ["otool", "-l", object_file_path]
    ).decode().splitlines()

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

def get_shared_library_paths(object_file_path):
    otool_output = subprocess.check_output(
        ["otool", "-L", object_file_path]
    ).decode().splitlines()

    i = 0

    while i < len(otool_output):
        otool_line = otool_output[i].strip()
        i += 1
        if "(compatibility version" in otool_line:
            yield otool_line.split("(compatibility")[0].strip()

def delete_rpath(object_file_path, rpath):
    delete_rpath_command = [
        "install_name_tool",
        "-delete_rpath",
        rpath,
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

def fix_rpath(target, root):
    for file in get_object_files(target):
        print(f"Fixing rpaths for {file}")

        for rpath in get_rpaths(file):
            output = f"\trpath: {rpath}"

            if rpath.startswith("@") is False and rpath.startswith(".") is False:
                delete_rpath(file, rpath)
                output += " (Deleted)"

            print(output)

        print(f"Fixing shared library paths for {file}")

        for library_path in get_shared_library_paths(file):
            output = f"\tlibrary path: {library_path}"

            shared_library_name = os.path.basename(library_path)
            if library_path.startswith(root) or library_path == shared_library_name:
                 new_library_path = change_shared_library_path(file, library_path)
                 output += f" (Changed to {new_library_path})"

            print(output)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--target", type=Path, help="Target to sanitize", required=True)
    parser.add_argument("--root", type=Path, help="Root directory", required=True)

    args = parser.parse_args()

    if args.target.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.target.absolute()}")
    if args.root.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.root.absolute()}")

    # fail if target is not in root
    if args.target.resolve().relative_to(args.root.resolve()) is False:
        raise ValueError(f"Target {args.target.absolute()} is not in root {args.root.absolute()}")

    fix_rpath(str(args.target.resolve().absolute()), str(args.root.resolve().absolute()))
