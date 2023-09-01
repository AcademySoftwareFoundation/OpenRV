# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2020 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

import itertools
import os
import platform
import requests
import shutil
import subprocess
import sys

from py7zr import SevenZipFile, exceptions
from typing import List
from zipfile import ZipFile


def _windows_zip_impl(input_folder: str, output_zip: str) -> None:
    """
    Windows implementation of the zip function.

    This implementation relies on shutil.make_archive because nothing special have to be done on Windows.

    :param str input_folder: Folder to archive
    :param str output_zip: Archive file name
    """
    result = shutil.make_archive(output_zip, "zip", input_folder)
    os.rename(result, output_zip)  # make_archive append .zip and this is not desirable.


def _windows_unzip_impl(input_zip: str, output_folder: str) -> None:
    """
    Windows implementation of the zip function.

    This implementation relies on ZipFile because nothing special have to be done on Windows.

    :param input_zip: Path to the zip file to uncompress
    :param output_folder: Location to wich the zip have to be uncompressed
    """
    with ZipFile(input_zip) as zip_file:
        zip_file.extractall(output_folder)


def _darwin_zip_impl(input_folder: str, output_zip: str) -> None:
    """
    macOS implementation of the zip function.

    This implementation relies on ditto so all the MacOS special sauce can be captured inside the archive.

    :param str input_folder: Folder to archive
    :param str output_zip: Archive file name
    """
    zip_args = ["/usr/bin/ditto", "-c", "-k", input_folder, output_zip]

    print(f"Executing {zip_args}")
    subprocess.run(zip_args).check_returncode()


def _darwin_unzip_impl(input_zip: str, output_folder: str) -> None:
    """
    macOS implementation of the zip function.

    This implementation relies on ditto so all the MacOS special sauce can be extracted from the archive.

    :param input_zip: Path to the zip file to uncompress
    :param output_folder: Location to which the zip have to be uncompressed
    """
    unzip_args = ["/usr/bin/ditto", "-x", "-k", input_zip, output_folder]

    print(f"Executing {unzip_args}")
    subprocess.run(unzip_args).check_returncode()


def _linux_zip_impl(input_folder: str, output_zip: str) -> None:
    """
    Linux implementation of the zip function.

    This implementation relies on the zip command line tool. This is prefered because that way, we can archive keeping
    the symlinks as-is.

    :param str input_folder: Folder to archive
    :param str output_zip: Archive file name
    """
    zip_args = ["zip", "--symlinks", "-r", output_zip, "."]

    print(f"Executing {zip_args}")
    subprocess.run(zip_args, cwd=input_folder).check_returncode()


def _linux_unzip_impl(input_zip: str, output_folder: str) -> None:
    """
    Linux implementation of the zip function.

    This implementation relies on the unzip command line tool. This is prefered because ZipFile doesn't preserve
    symlinks.

    :param input_zip: Path to the zip file to uncompress
    :param output_folder: Location to which the zip have to be uncompressed
    """
    unzip_args = ["unzip", "-q", input_zip, "-d", output_folder]

    print(f"Executing {unzip_args}")
    subprocess.run(unzip_args).check_returncode()


def create_zip_archive(input_folder: str, output_zip: str) -> None:
    """
    Implementation of the zip function.

    :param str input_folder: Folder to archive
    :param str output_zip: Archive file name
    """
    print(f"Archiving {input_folder} into {output_zip}")

    if platform.system() == "Linux":
        _linux_zip_impl(input_folder, output_zip)
    elif platform.system() == "Darwin":
        _darwin_zip_impl(input_folder, output_zip)
    else:
        _windows_zip_impl(input_folder, output_zip)


def extract_zip_archive(input_zip: str, output_folder: str) -> None:
    """
    Implementation of the unzip function.

    :param input_zip: Path to the zip file to uncompress
    :param output_folder: Location to which the zip have to be uncompressed
    """
    print(f"Extracting {input_zip} into {output_folder}")

    if platform.system() == "Linux":
        _linux_unzip_impl(input_zip, output_folder)
    elif platform.system() == "Darwin":
        _darwin_unzip_impl(input_zip, output_folder)
    else:
        _windows_unzip_impl(input_zip, output_folder)


def extract_7z_archive(input_7z: str, output_folder: str) -> None:
    """
    Implementation of the uncompress function for 7z files.

    :param input_zip: Path to the 7z file to uncompress
    :param output_folder: Location to which the 7z have to be uncompressed
    """
    print(f"Extracting {input_7z} into {output_folder}")

    with SevenZipFile(input_7z, "r") as z:
        z.extractall(output_folder)


def verify_7z_archive(input_7z: str) -> bool or None:
    """
    Returns True/False if CRC-check passes or None if there is no CRC

    :param input_zip: Path to the 7z file to uncompress
    """
    print(f"Verifying {input_7z}")
    try:
        with SevenZipFile(input_7z, "r") as z:
            return z.test()
    except exceptions.Bad7zFile:
        return None


def _darwin_copy_recursively(src_folder: str, destination_folder) -> None:
    """
    Implementation of the copy function.

    This implementation uses ditto as it's the preferred copy command on MacOS X

    :param src_folder: Path of source of the copy
    :param destination_folder: Path to the destination the copy
    """
    copy_args = ["ditto", src_folder, destination_folder]

    print(f"Executing {copy_args}")
    subprocess.run(copy_args).check_returncode()


def _linux_copy_recursively(src_folder: str, destination_folder) -> None:
    """
    Implementation of the copy function.

    This implementation uses cp as it's the preferred copy command on Linux.

    :param src_folder: Path of source of the copy
    :param destination_folder: Path to the destination the copy
    """
    copy_args = ["cp", "-r", src_folder, destination_folder]

    print(f"Executing {copy_args}")
    subprocess.run(copy_args).check_returncode()


def _windows_copy_recursively(src_folder: str, destination_folder) -> None:
    """
    Implementation of the copy function.

    :param src_folder: Path of source of the copy
    :param destination_folder: Path to the destination the copy
    """
    raise NotImplementedError()


def copy_recursively(src_folder: str, destination_folder: str) -> None:
    """
    Implementation of the copy function.

    :param src_folder: Path of source of the copy
    :param destination_folder: Path to the destination the copy
    """

    print(f"Copying {src_folder} into {destination_folder}")

    if platform.system() == "Linux":
        _linux_copy_recursively(src_folder, destination_folder)
    elif platform.system() == "Darwin":
        _darwin_copy_recursively(src_folder, destination_folder)
    else:
        _windows_copy_recursively(src_folder, destination_folder)


def download_file(url, file_path):
    """
    Download a file with HTTP or HTTPS.

    param: url: url to download
    param file_path: destination file path
    """

    print(f"Downloading {url} into {file_path}")
    req = requests.get(url, stream=True)

    with open(file_path, "wb") as file:
        total = req.headers.get("content-length")
        if total is None:
            file.write(req.content)
        else:
            total = int(total)
            current = 0

            for chunk in req.iter_content(round(total / 100)):
                current += len(chunk)
                file.write(chunk)
                print(f"{current} / {total} ({round((current / total) * 100)}%)")


def get_cygpath_windows(cygpath: str) -> str:
    """
    Returns the windows path corresponding to the cygpath passed as parameter.
    :param string cygpath: cygpath to be translated. Example: C:/git/OpenRV/_build
    """
    return (
        subprocess.check_output(["cygpath", "--windows", "--absolute", f"{cygpath}"])
        .rstrip()
        .decode("utf-8")
    )


def update_env_path(newpaths: str) -> None:
    """
    Augment the PATH environment variable with newpaths
    """
    paths = os.environ["PATH"].lower().split(os.pathsep)
    for path in newpaths:
        if not path.lower() in paths:
            paths.insert(0, path)
            os.environ["PATH"] = "{}{}{}".format(path, os.pathsep, os.environ["PATH"])


def source_widows_msvc_env(msvc_year: str) -> None:
    """
    Source the Microsoft Windows Visual Studio x environment so that a subprocess build
    inherits the proper msvc environement to build.
    Note: Equivalent to call vcvars64.bat from a command prompt.
    :param string msvc_year: Visual Studio x environment to be sourced. Example: 2019
    """

    def get_environment_from_batch_command(env_cmd, initial=None):
        """
        Take a command (either a single command or list of arguments)
        and return the environment created after running that command.
        Note that the command must be a batch file or .cmd file, or the
        changes to the environment will not be captured.

        If initial is supplied, it is used as the initial environment passed
        to the child process.
        """

        def validate_pair(ob):
            try:
                if not (len(ob) == 2):
                    print("Unexpected result: {}".format(ob))
                    raise ValueError
            except:
                return False
            return True

        def consume(iter):
            try:
                while True:
                    next(iter)
            except StopIteration:
                pass

        if not isinstance(env_cmd, (list, tuple)):
            env_cmd = [env_cmd]
        # construct the command that will alter the environment
        env_cmd = subprocess.list2cmdline(env_cmd)
        # create a tag so we can tell in the output when the proc is done
        tag = "Done running command"
        # construct a cmd.exe command to do accomplish this
        cmd = 'cmd.exe /E:ON /V:ON /s /c "{} && echo "{}" && set"'.format(env_cmd, tag)
        # launch the process
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, env=initial)
        # parse the output sent to stdout
        lines = proc.stdout
        if sys.version_info[0] > 2:
            # make sure the lines are strings
            lines = map(lambda s: s.decode(), lines)
        # consume whatever output occurs until the tag is reached
        consume(itertools.takewhile(lambda l: tag not in l, lines))
        # define a way to handle each KEY=VALUE line
        # parse key/values into pairs
        pairs = map(lambda l: l.rstrip().split("=", 1), lines)
        # make sure the pairs are valid
        valid_pairs = filter(validate_pair, pairs)
        # construct a dictionary of the pairs
        result = dict(valid_pairs)
        # let the process finish
        proc.communicate()
        return result

    mvs_base_path = os.path.join(
        os.path.expandvars("%SystemDrive%") + os.sep,
        "Program Files",
        "Microsoft Visual Studio",
    )
    vcvars_path = os.path.join(
        mvs_base_path,
        msvc_year,
        "Professional",
        "VC",
        "Auxiliary",
        "Build",
        "vcvars64.bat",
    )
    if not os.path.exists(vcvars_path):
        vcvars_path = os.path.join(
            mvs_base_path,
            msvc_year,
            "Entreprise",
            "VC",
            "Auxiliary",
            "Build",
            "vcvars64.bat",
        )
        if not os.path.exists(vcvars_path):
            vcvars_path = os.path.join(
                mvs_base_path,
                msvc_year,
                "Community",
                "VC",
                "Auxiliary",
                "Build",
                "vcvars64.bat",
            )

    if not os.path.exists(vcvars_path):
        print(
            "ERROR: Failed to find the MSVC compiler environment init script (vcvars64.bat) on your system."
        )
        return
    else:
        print(
            f"Found MSVC compiler environment init script (vcvars64.bat):\n{vcvars_path}"
        )

    # Get MSVC env
    vcvars_cmd = [vcvars_path]
    msvc_env = get_environment_from_batch_command(vcvars_cmd)
    msvc_env_paths = os.pathsep.join(
        [msvc_env[k] for k in msvc_env if k.upper() == "PATH"]
    ).split(os.pathsep)
    msvc_env_without_paths = dict(
        [(k, msvc_env[k]) for k in msvc_env if k.upper() != "PATH"]
    )

    # Extend os.environ with MSVC env
    update_env_path(msvc_env_paths)
    for k in sorted(msvc_env_without_paths):
        v = msvc_env_without_paths[k]
        os.environ[k] = v


def get_mingw64_path_on_windows(winpath: str) -> str:
    """
    On Windows: returns the mingw64 path corresponding to the windows passed as parameter.
    On other platforms: simply returns the path passed as parameter
    :param string winpath: winpath to be translated. Example: C:\git\OpenRV\_build
    """
    if platform.system() != "Windows":
        return winpath

    return (
        subprocess.check_output(
            ["c:\\msys64\\usr\\bin\\cygpath.exe", "--mixed", "--absolute", f"{winpath}"]
        )
        .rstrip()
        .decode("utf-8")
    )


def get_mingw64_args(cmd_args: List[str]) -> List[str]:

    updated_cmd_args = cmd_args
    updated_cmd_args.append(";sleep 10")

    mingw64_cmd_args = [
        "c:\\msys64\\msys2_shell.cmd",
        "-defterm",
        "-mingw64",
        "-here",
        "-c",
        " ".join(updated_cmd_args),
    ]

    return mingw64_cmd_args
