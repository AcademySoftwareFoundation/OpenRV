#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2024 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************


import argparse
import pathlib
import platform
import shutil
import subprocess
import os


def create_symlink(source: pathlib.Path, destination: pathlib.Path) -> None:
    """
    Create a symlink from source to destination using the relative path of the destination from the source.
    It will remove the destination if it exists.

    :param source: Source of the symlink.
    :param destination: Destination of the symlink.
    :return:
    """
    symlink_val = os.path.relpath(source, destination)

    print(f"Creating symlink {destination} -> {symlink_val}")

    if destination.is_symlink() or destination.exists():
        try:
            destination.unlink()
        except:
            shutil.rmtree(destination)

    destination.symlink_to(symlink_val)


def fix_plugins(baked_libraries: pathlib.Path, official_plugins: pathlib.Path) -> None:
    """
    Fixes the plugins baked into PySide2.

    :param baked_libraries: Path to the baked plugins
    :param official_plugins: Path to the official plugins
    :return:
    """

    for d in baked_libraries.iterdir():
        if d.is_symlink() or d.is_file():
            continue

        for l in d.iterdir():
            symlink_source = official_plugins / d.name / l.name
            if not symlink_source.exists():
                continue

            create_symlink(symlink_source, l)


def fix_libraries(
    baked_libraries: pathlib.Path, official_libraries: pathlib.Path
) -> None:
    """
    Fixes the libraries baked into PySide2.

    :param baked_libraries: Path to the baked libraries
    :param official_libraries: Path to the official libraries
    :return:
    """

    for d in baked_libraries.iterdir():
        if d.is_symlink() or d.is_file():
            continue

        symlink_source = official_libraries / d.name
        if not symlink_source.exists():
            continue

        create_symlink(symlink_source, d)


def fix_qt_libraries(
    *,
    python_executable: pathlib.Path,
    libraries: pathlib.Path,
    plugins: pathlib.Path,
) -> None:
    """
    Fixes the qt libraries baked into PySide2.

    :param python_executable: Path to the python executable containing PySide2
    :param libraries: Path to the official libraries
    :param plugins: Path to the official plugins
    :return:
    """
    if platform.system() != "Darwin":
        # It's not necessary to fix the libraries on Linux or Windows.
        return None

    pyside_root = (
        subprocess.check_output(
            [
                python_executable,
                "-c",
                "import PySide2; print(PySide2.__path__[0])",
            ]
        )
        .decode()
        .strip()
    )

    embedded_qt = pathlib.Path(pyside_root) / "Qt"

    embedded_qt_libs = embedded_qt / "lib"
    embedded_qt_plugins = embedded_qt / "plugins"

    fix_libraries(embedded_qt_libs.absolute(), libraries.absolute())
    fix_plugins(embedded_qt_plugins.absolute(), plugins.absolute())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog=__file__, description="Fixes the qt libraries baked into PySide2"
    )

    parser.add_argument(
        "--python-executable",
        dest="python_executable",
        type=pathlib.Path,
        required=True,
        help="The source folder to copy from.",
    )

    parser.add_argument(
        "--libraries",
        dest="libraries",
        type=pathlib.Path,
        required=True,
        help="Path to Qt libraries.",
    )

    parser.add_argument(
        "--plugins",
        dest="plugins",
        type=pathlib.Path,
        required=True,
        help="Path to Qt plugins.",
    )

    args = vars(parser.parse_args())
    print(args)

    fix_qt_libraries(**args)
