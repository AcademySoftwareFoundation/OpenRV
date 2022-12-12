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
                    yield path
    else:
        yield target


def get_rpaths(object_file_path):
    otool_output = subprocess.check_output(
        ["otool", "-l", object_file_path]
    ).splitlines()

    i = 0
    while i < len(otool_output):
        otool_line = otool_output[i].strip()
        i += 1

        if otool_line.split()[-1] == b"LC_RPATH":
            for y in range(i, i + 2):
                rpath_line = otool_output[y].split()

                if rpath_line[0] == b"path":
                    yield rpath_line[1]
                    break


def delete_rpath(object_file_path, rpath):
    delete_rpath_command = [
        "install_name_tool",
        "-delete_rpath",
        rpath,
        object_file_path,
    ]
    subprocess.run(delete_rpath_command).check_returncode()


def remove_absolute_rpath(target):
    for file in get_object_files(target):
        print(f"Found Mach-O file: {file}")

        for rpath in get_rpaths(file):
            output = f"\trpath: {rpath}"

            if rpath.startswith(b"@") is False and rpath.startswith(b".") is False:
                delete_rpath(file, rpath)
                output += " (Deleted)"

            print(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--target", type=Path, help="Target to sanitize", required=True)

    args = parser.parse_args()

    if args.target.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.target.absolute()}")

    remove_absolute_rpath(args.target)
