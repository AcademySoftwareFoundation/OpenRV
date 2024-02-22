#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2024 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************


import argparse
import concurrent.futures
import os
import pathlib
import platform
import shutil
import sys
import time
import threading

from typing import Tuple

from remove_absolute_rpath import fix_rpath

os.environ["PYTHONUNBUFFERED"] = "1"

# Lock to ensure that only one thread is printing at a time.
print_lock = threading.Lock()


def threadsafe_print(message: str, options: dict):
    """
    Print a message in a thread safe manner.

    :param message: The message to print.
    :param options: Options to pass to the print function.

    :return: None
    """
    if options.get("quiet", False):
        return

    with print_lock:
        print(message, flush=True, file=sys.stdout)


def threadsafe_print_error(message: str, _) -> None:
    """
    Print a message in a thread safe manner.

    :param message: The message to print.
    :param _: Placeholder to have the same signature as threadsafe_print.

    :return: None
    """

    with print_lock:
        print(message, flush=True, file=sys.stderr)


# List of futures to burn.
futures = []

# Lock to ensure that only one thread is accessing the list of futures at a time.
future_lock = threading.Lock()


def save_future(future: concurrent.futures.Future):
    """
    Save a future to be burned later. This is used to ensure that all futures are burned before the program exits.

    :param future: The future to save.
    :return: None
    """
    with future_lock:
        futures.append(future)


def burn_future() -> bool:
    """
    Burn a future. This will block until the future is complete.

    :return: weather or not a future was burned.
    """
    future = None

    with future_lock:
        if len(futures) > 0:
            future = futures.pop()

    if future:
        future.result()

    return future is not None


def copy(
    executor: concurrent.futures.Executor,
    source: pathlib.Path,
    destination: pathlib.Path,
    options: dict,
) -> None:
    """
    Copy a file, directory, or symlink. This will call the appropriate function based on the type of the source.

    :param executor: Executor to use for copying files in the directory.
    :param source: Source file, directory, or symlink.
    :param destination: Destination file, directory, or symlink.
    :param options: Options to pass to the copy function.
    :return: None
    """
    f = None

    if source.is_symlink():
        f = copy_symlink
    elif source.is_file():
        f = copy_file
    elif source.is_dir():
        f = copy_dir

    if f:
        f(executor, source, destination, options)
    else:
        raise RuntimeError(f"Unable to copy {source} -- Unknown file type")


def copy_symlink(
    _, symlink_path: pathlib.Path, destination: pathlib.Path, options: dict
) -> None:
    """
    Copy a symlink.

    :param _: Placeholder to have the same signature as copy_dir.
    :param symlink_path: Symlink to copy.
    :param destination: Destination file.
    :param options: Options to pass to the copy function.
    :return: None
    """
    start = time.time()

    message = f"LINK:\tCopying {symlink_path} to {destination}"
    threadsafe_print(message, options)

    ext = "".join(symlink_path.suffixes).lower()
    if ext in options.get("denied_extensions", []) or symlink_path.name == ".DS_Store":
        threadsafe_print(f"{message} -- IGNORED", options)
        return

    if destination.exists() or destination.is_symlink():
        if options.get("override", False):
            threadsafe_print(f"{message} -- REMOVING OLD COPY", options)
            destination.unlink()
        else:
            threadsafe_print(f"{message} -- ALREADY EXISTS", options)
            return

    destination.symlink_to(
        os.readlink(str(symlink_path)), target_is_directory=symlink_path.is_dir()
    )

    threadsafe_print(f"{message} -- DONE in {time.time() - start} seconds", options)


def copy_file(
    _, file_path: pathlib.Path, destination: pathlib.Path, options: dict
) -> None:
    """
    Copy a file.

    :param _: Placeholder to have the same signature as copy_dir.
    :param file_path: File to copy.
    :param destination: Destination file.
    :param options: Options to pass to the copy function.
    :return: None
    """
    start = time.time()

    message = f"FILE:\tCopying {file_path} to {destination}"
    threadsafe_print(message, options)

    ext = "".join(file_path.suffixes).lower()
    if ext in options.get("denied_extensions", []) or file_path.name == ".DS_Store":
        threadsafe_print(f"{message} -- IGNORED", options)
        return

    if destination.exists() or destination.is_symlink():
        if options.get("override", False):
            threadsafe_print(f"{message} -- REMOVING OLD COPY", options)
            destination.unlink()
        else:
            threadsafe_print(f"{message} -- ALREADY EXISTS", options)
            return

    shutil.copy(file_path, destination, follow_symlinks=False)

    if platform.system() == "Darwin":
        fix_rpath(destination, options.get("build_root", pathlib.Path(".")))

    threadsafe_print(f"{message} -- DONE in {time.time() - start} seconds", options)


def copy_dir(
    executor: concurrent.futures.Executor,
    dir_path: pathlib.Path,
    destination: pathlib.Path,
    options: dict,
) -> None:
    """
    Copy a directory.

    :param executor: Executor to use for copying files in the directory.
    :param dir_path: Directory to copy.
    :param destination: Destination directory.
    :param options: Options to pass to the copy function.
    :return: None
    """

    start = time.time()
    message = f"DIR:\tCopying {dir_path} to {destination}"
    threadsafe_print(message, options)

    destination.mkdir(exist_ok=True)

    for child in dir_path.iterdir():
        save_future(
            executor.submit(copy, executor, child, destination / child.name, options)
        )

    threadsafe_print(f"{message} -- DONE in {time.time() - start} seconds", options)


def copy_third_party(
    *,
    source: pathlib.Path,
    destination: pathlib.Path,
    build_root: pathlib.Path,
    denied_extensions: Tuple[str],
    quiet: bool = False,
    override: bool = True,
) -> None:
    """
    Copy a third party library to a destination folder. This uses a thread pool to copy files in parallel.

    :param source: Original third party library.
    :param destination: Destination folder.
    :param build_root: Root directory of the build.
    :param denied_extensions: Extensions to ignore.
    :param quiet: Whether to print progress messages.
    :param override: Whether to override the destination if it exists.

    :return:
    """

    with concurrent.futures.ThreadPoolExecutor() as executor:
        save_future(
            executor.submit(
                copy,
                executor,
                source.absolute(),
                destination.absolute(),
                {
                    "denied_extensions": denied_extensions,
                    "quiet": quiet,
                    "override": override,
                    "build_root": build_root,
                },
            )
        )

        while burn_future():
            pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog=__file__,
        description="Copy third party libraries to a destination folder. This uses a "
        "thread pool to copy files in parallel. This is useful for copying "
        "large third party libraries.",
    )

    parser.add_argument(
        "--source",
        dest="source",
        type=pathlib.Path,
        required=True,
        help="The source folder to copy from.",
    )

    parser.add_argument(
        "--destination",
        dest="destination",
        type=pathlib.Path,
        required=True,
        help="The destination folder to copy to.",
    )

    parser.add_argument(
        "--build-root",
        dest="build_root",
        type=pathlib.Path,
        required=platform.system() == "Darwin",
        default=pathlib.Path(__file__).parent.parent.parent,
        help="Root directory of the build.",
    )

    parser.add_argument(
        "--denied-extensions",
        dest="denied_extensions",
        type=tuple,
        nargs="*",
        required=False,
        help="A list of extensions to ignore.",
        default=(".prl", ".o", ".obj", ".la"),
    )

    parser.add_argument(
        "--quiet",
        dest="quiet",
        action="store_true",
        required=False,
        default=False,
        help="Don't print progress messages.",
    )

    override = parser.add_mutually_exclusive_group(required=False)
    override.add_argument(
        "--override",
        dest="override",
        action="store_true",
        help="Override the destination if it exists.",
    )

    override.add_argument(
        "--no-override",
        dest="override",
        action="store_false",
        help="Do not override the destination if it exists.",
    )

    args = vars(parser.parse_args())
    print(args)

    copy_third_party(**args)
