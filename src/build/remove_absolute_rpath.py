#!/usr/bin/env python

# *****************************************************************************
# Copyright (c) 2020 Autodesk, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************
import argparse
import concurrent.futures
import logging
import os
import pathlib
from pathlib import Path
import subprocess
import threading

from typing import Iterable

future_lock = threading.Lock()

# Configure logging.
logging.basicConfig(format='-- %(message)s')
logger = logging.getLogger().setLevel(logging.INFO)

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
        logging.info(f"Fixing rpaths for {file}")

        # Skipping numpy because there are some dylib in .dylibs in number folder that
        # become invalid due to our rpath fixing logic.
        # IMPORTANT:
        #    Numpy was the only issue found, but it could happend again if new dependencies are added.
        is_numpy = file._str.find(f"numpy")
        if is_numpy > -1:
            logging.info(f"Skipping {file} because numpy")
            return

        # Prevent delete the same rpath multiple time (e.g. MacOS fat binary).
        deletedRPATH = []
        for rpath in get_rpaths(file):
            output = f"\trpath: {rpath}"

            if rpath.startswith("@") is False and rpath.startswith(".") is False and not rpath in deletedRPATH:
                delete_rpath(file, rpath)
                deletedRPATH.append(rpath)
                output += " (Deleted)"

            logging.info(output)

        logging.info(f"Fixing shared library paths for {file}")

        for library_path in get_shared_library_paths(file):
            output = f"\tlibrary path: {library_path}"

            shared_library_name = os.path.basename(library_path)
            if library_path.startswith(root) or library_path == shared_library_name:
                 new_library_path = change_shared_library_path(file, library_path)
                 output += f" (Changed to {new_library_path})"

            logging.info(output)

def read_paths_from_file(file_path):
    paths = []
    with open(file_path, 'r') as file:
        for line in file:
            paths.append(line.strip()) # strip() removes newline characters
    return paths

def fix_rpath_with_lock(path, root_dir):
    with future_lock:
        return fix_rpath(str(path), str(root_dir))
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--file", type=pathlib.Path, dest="file", help="Target to sanitize")
    parser.add_argument("--files-list", type=pathlib.Path, dest="files_list", help="Files to sanitize")
    parser.add_argument("--root", type=pathlib.Path, dest="root_dir", help="Root directory", required=True)

    args = parser.parse_args()

    if args.file is None and args.files_list is None:
        parser.error("At least one of --file or --files-list is required.")

    if args.file and args.file.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.file.absolute()}")
    elif args.files_list and args.files_list.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.files_list.absolute()}")
    if args.root_dir.exists() is False:
        raise FileNotFoundError(f"Unable to locate {args.root_dir.absolute()}")
    
    # file option has priority over files-list.
    if args.file:
        # Fail if file is not in root
        if args.file.resolve().relative_to(args.root_dir.resolve()) is False:
            raise ValueError(f"File {args.file.absolute()} is not in root {args.root_dir.absolute()}")
    elif args.files_list:
        paths = read_paths_from_file(str(args.files_list.resolve().absolute()))
        with concurrent.futures.ThreadPoolExecutor() as executor:
            # Create a list of futures for each path to be processed.
            futures = [executor.submit(fix_rpath_with_lock, pathlib.Path(path), args.root_dir.resolve().absolute()) for path in paths]
            for future in concurrent.futures.as_completed(futures):
                try:
                    result = future.result()
                except Exception as e:
                    logging.error(f"Error processing path: {result}, Error: {e}")
                    raise e