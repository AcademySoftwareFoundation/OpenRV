#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
import pathlib
import subprocess


def get_packages_from_dir(packages_source_folder: pathlib.Path) -> [pathlib.Path]:
    for file in sorted(packages_source_folder.iterdir()):
        if file.is_file() and file.suffix == ".rvpkg":
            yield file.resolve()


def install_rvpkg_packages(
    *,
    rvpkg_path: pathlib.Path,
    packages_source_folder: pathlib.Path,
    packages_destination_folder: pathlib.Path
) -> None:
    command = [
        rvpkg_path,
        "-force",
        "-install",
        "-add",
        packages_destination_folder.resolve(),
        *list(get_packages_from_dir(packages_source_folder)),
    ]

    subprocess.run(command).check_returncode()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--rvpkg", dest="rvpkg_path", type=pathlib.Path, required=True)
    parser.add_argument(
        "--source", dest="packages_source_folder", type=pathlib.Path, required=True
    )
    parser.add_argument(
        "--destination",
        dest="packages_destination_folder",
        type=pathlib.Path,
        required=True,
    )

    install_rvpkg_packages(**vars(parser.parse_args()))
