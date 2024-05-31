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
import subprocess
import threading

from typing import Iterable

future_lock = threading.Lock()

# Configure logging.
logging.basicConfig(format='-- %(message)s')
logger = logging.getLogger().setLevel(logging.INFO)

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


def set_id(object_file_path: pathlib.Path, id_name: str = "") -> str:
    new_library_id = f"@rpath/{object_file_path.name}"
    if id_name:
        new_library_id = f"@rpath/{id_name}"

    set_id_command = [
        "install_name_tool",
        "-id",
        new_library_id,
        str(object_file_path),
    ]
    logging.info(f"Set id to {new_library_id} for {str(object_file_path)}")
    subprocess.run(set_id_command).check_returncode()

    return new_library_id

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
        str(old_library_path),
        new_library_path,
        str(object_file_path),
    ]
    subprocess.run(change_shared_library_path_command).check_returncode()

    return new_library_path

def fix_rpath(target: pathlib.Path, root: pathlib.Path):
    for file in get_object_files(target):
        try:
            logging.info(f"Fixing rpaths for {file}")

            for rpath in get_rpaths(file):
                output = f"\trpath: {rpath}"

                if rpath.startswith("@") is False and rpath.startswith(".") is False:
                    delete_rpath(file, rpath)
                    output += " (Deleted)"

                logging.debug(output)


            new_id_name = ""
            # Checks if the file is coming from Qt Frameworks.
            # Example: [...]/RV.app/Contents/Frameworks/Qt3DAnimation.framework/Versions/5/Qt3DAnimation
            framework_name_index = file._str.find(f".framework")
            # When Python is build from source, the folder will be called "python" + the version.
            # Assume that it is the case for all Python version and search only for python in the path.
            python_folder = file._str.find(f"python")
            site_package_folder = file._str.find(f"site-packages")

            # Do not change the id for Qt Frameworks and for Python's site-packages.
            if framework_name_index == -1 and python_folder == -1 and site_package_folder == -1:
                lib_id = set_id(file, new_id_name)

            logging.info(f"Fixing shared library paths for {file}")
            for library_path in get_shared_library_paths(file):
                output = f"\t{file} library path: {library_path}"
                library_path = pathlib.Path(library_path)

                # Fix entries containing only library name and paths to the build directory.
                # Do not change entries coming from system location or other paths.

                # e.g. 
                #       /usr/lib/abc.dylib (do no change this rpath)
                #          is_outside_of_build_folder   = True
                #          is_from_build_folder         = False
                #          is_loose_library             = False
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
                    and library_path.is_relative_to(root)
                )

                is_loose_library = (
                    not str(library_path).startswith("/")
                    and not str(library_path).startswith("@")
                    and not str(library_path).startswith(".")
                )

                # logging.info(f"{library_path != file.name}, {is_loose_library}, {is_from_build_folder}")
                if library_path != file.name and (is_loose_library or is_from_build_folder):
                    new_library_path = change_shared_library_path(file, library_path)
                    output += f" (Changed to {new_library_path})"

                logging.info(output)
        except subprocess.CalledProcessError as e:
            logging.error(f"Error fixing rpath for {file}: {e}")
            logging.error(f"Output: {e.output}")
            raise e


def read_paths_from_file(file_path):
    paths = []
    with open(file_path, 'r') as file:
        for line in file:
            paths.append(line.strip()) # strip() removes newline characters
    return paths

def fix_rpath_with_lock(path, root_dir):
    with future_lock:
        return fix_rpath(path, root_dir)
    
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