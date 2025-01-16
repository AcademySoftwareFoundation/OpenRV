#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2020 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************


import argparse
import glob
import os
import pathlib
import shutil
import sys
import subprocess
import platform
import tempfile
import uuid

from typing import List
from datetime import datetime

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(ROOT_DIR)

OPENSSL_OUTPUT_DIR = ""
SOURCE_DIR = ""
OUTPUT_DIR = ""
TEMP_DIR = ""
VARIANT = ""
ARCH = ""
OPENTIMELINEIO_SOURCE_DIR = ""

SITECUSTOMIZE_FILE_CONTENT = f'''
#
# Copyright (c) {datetime.now().year} Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Site-level module that ensures OpenSSL will have up to date certificate authorities
on Linux and macOS. It gets imported when the Python interpreter starts up, both
when launching Python as a standalone interpreter or as an embedded one.
The OpenSSL shipped with Desktop requires a list of certificate authorities to be
distributed with the build instead of relying on the OS keychain. In order to keep
an up to date list, we're going to pull it from the certifi module, which incorporates
all the certificate authorities that are distributed with Firefox.
"""
import site

try:
    import os
    import certifi

    # Do not set SSL_CERT_FILE to our own if it is already set. Someone could
    # have their own certificate authority that they specify with this env var.
    # Unfortunately this is not a PATH like environment variable, so we can't
    # concatenate multiple paths with ":".
    #
    # To learn more about SSL_CERT_FILE and how it is being used by OpenSSL when
    # verifying certificates, visit
    # https://www.openssl.org/docs/man1.1.0/ssl/SSL_CTX_set_default_verify_paths.html
    if "SSL_CERT_FILE" not in os.environ and "DO_NOT_SET_SSL_CERT_FILE" not in os.environ:
        os.environ["SSL_CERT_FILE"] = certifi.where()

except Exception as e:
    print("Failed to set certifi.where() as SSL_CERT_FILE.", file=sys.stderr)
    print(e, file=sys.stderr)
    print("Set DO_NOT_SET_SSL_CERT_FILE to skip this step in RV's Python initialization.", file=sys.stderr)

try:
    import os

    if "DO_NOT_REORDER_PYTHON_PATH" not in os.environ:
        import site
        import sys

        prefixes = list(set(site.PREFIXES))

        # Python libs and site-packages is the first that should be in the PATH
        new_path_list = list(set(site.getsitepackages()))
        new_path_list.insert(0, os.path.dirname(new_path_list[0]))

        # Then any paths in RV's app package
        for path in sys.path:
            for prefix in prefixes:
                if path.startswith(prefix) is False:
                    continue

                if os.path.exists(path):
                    new_path_list.append(path)

        # Then the remaining paths
        for path in sys.path:
            if os.path.exists(path):
                new_path_list.append(path)

        # Save the new sys.path
        sys.path = new_path_list
        site.removeduppaths()

except Exception as e:
    print("Failed to reorder RV's Python search path", file=sys.stderr)
    print(e, file=sys.stderr)
    print("Set DO_NOT_REORDER_PYTHON_PATH to skip this step in RV's Python initialization.", file=sys.stderr)
'''


def get_python_interpreter_args(python_home: str) -> List[str]:
    """
    Return the path to the python interpreter given a Python home

    :param python_home: Python home of a Python package
    :return: Path to the python interpreter
    """

    build_opentimelineio = platform.system() == "Windows" and VARIANT == "Debug"
    python_name_pattern = "python*" if not build_opentimelineio else "python_d*"

    python_interpreters = glob.glob(
        os.path.join(python_home, python_name_pattern), recursive=True
    )
    python_interpreters += glob.glob(os.path.join(python_home, "bin", python_name_pattern))

    # sort put python# before python#-config
    python_interpreters = sorted(
        [
            p
            for p in python_interpreters
            if os.path.islink(p) is False and os.access(p, os.X_OK)
        ]
    )

    if not python_interpreters or os.path.exists(python_interpreters[0]) is False:
        raise FileNotFoundError()

    print(f"Found python interpreters {python_interpreters}")

    python_interpreter = sorted(python_interpreters)[0]

    return [python_interpreter, "-s", "-E"]


def patch_python_distribution(python_home: str) -> None:
    """
    Patch the python distribution to make it relocatable.
    Patch the python site-packages to rely on certifi to get the SSL certificates.

    :param python_home: Home of the Python to patch.
    """

    for failed_lib in glob.glob(
        os.path.join(python_home, "lib", "**", "*_failed.so"), recursive=True
    ):
        # The Python build mark some so as _failed if it cannot load OpenSSL. In our case it's expected because
        # out OpenSSL package works with RPATH and the RPATH is not set on the python build tests. If the lib failed
        # for other reasons, it will fail later in our build script.
        if "ssl" in failed_lib or "hashlib" in failed_lib:
            print(f"Fixing {failed_lib}")
            shutil.move(failed_lib, failed_lib.replace("_failed.so", ".so"))
    if OPENSSL_OUTPUT_DIR:
        if platform.system() == "Darwin":
            openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "lib", "lib*.dylib*"))
            openssl_libs = [l for l in openssl_libs if os.path.islink(l) is False]

            python_openssl_libs = []

            for lib_path in openssl_libs:
                print(f"Copying {lib_path} to the python home")
                dest = os.path.join(python_home, "lib", os.path.basename(lib_path))
                shutil.copyfile(lib_path, dest)
                python_openssl_libs.append(dest)

            libs_to_patch = glob.glob(
                os.path.join(python_home, "lib", "**", "*.so"), recursive=True
            )

            for lib_path in python_openssl_libs:
                lib_name = os.path.basename(lib_path)

                for lib_to_patch in libs_to_patch:
                    print(f"Changing the library path of {lib_name} on {lib_to_patch}")
                    install_name_tool_change_args = [
                        "install_name_tool",
                        "-change",
                        lib_path,
                        f"@rpath/{lib_name}",
                        lib_to_patch,
                    ]

                    print(f"Executing {install_name_tool_change_args}")
                    subprocess.run(install_name_tool_change_args).check_returncode()

        elif platform.system() == "Linux":
            openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "lib", "lib*.so*"))

            for lib_path in openssl_libs:
                print(f"Copying {lib_path} to the python home")
                shutil.copy(lib_path, os.path.join(python_home, "lib"))

        elif platform.system() == "Windows":
            openssl_dlls = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "bin", "lib*"))
            for dll_path in openssl_dlls:
                print(f"Copying {dll_path} to the python home")
                shutil.copy(dll_path, os.path.join(python_home, "bin"))

            openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "lib", "lib*"))
            for lib_path in openssl_libs:
                print(f"Copying {lib_path} to the python home")
                shutil.copy(lib_path, os.path.join(python_home, "libs"))

    python_interpreter_args = get_python_interpreter_args(python_home)

    # -I : isolate Python from the user's environment
    python_interpreter_args.append("-I")

    ensure_pip_args = python_interpreter_args + ["-m", "ensurepip"]
    print(f"Ensuring pip with {ensure_pip_args}")
    subprocess.run(ensure_pip_args).check_returncode()

    pip_args = python_interpreter_args + ["-m", "pip"]

    for package in ["pip", "certifi", "six", "wheel", "packaging", "requests"]:
        package_install_args = pip_args + [
            "install",
            "--upgrade",
            "--force-reinstall",
            package,
        ]
        print(f"Installing {package} with {package_install_args}")
        subprocess.run(package_install_args).check_returncode()

    wheel_install_args = pip_args + [
        "install",
        "--upgrade",
        "--force-reinstall",
        "wheel",
    ]
    print(f"Installing wheel with {wheel_install_args}")
    subprocess.run(wheel_install_args).check_returncode()

    site_packages = glob.glob(
        os.path.join(python_home, "**", "site-packages"), recursive=True
    )[0]

    if os.path.exists(site_packages) is False:
        raise FileNotFoundError()

    print(f"Site packages found at {site_packages}")

    site_customize_path = os.path.join(site_packages, "sitecustomize.py")
    if os.path.exists(site_customize_path):
        os.remove(site_customize_path)

    print(f"Installing the sitecustomize.py payload at {site_customize_path}")
    site_customize_path = os.path.join(site_packages, "sitecustomize.py")
    if os.path.exists(site_customize_path):
        print(
            "Found a sitecustomize.py in the site-packages, the content of the file will be overwritten"
        )
        os.remove(site_customize_path)

    with open(site_customize_path, "w") as sitecustomize_file:
        sitecustomize_file.write(SITECUSTOMIZE_FILE_CONTENT)


def test_python_distribution(python_home: str) -> None:
    """
    Test the Python distribution.

    :param python_home: Package root of an Python package
    """
    tmp_dir = os.path.join(tempfile.gettempdir(), str(uuid.uuid4()))
    os.makedirs(tmp_dir)

    tmp_python_home = os.path.join(tmp_dir, os.path.basename(python_home))
    try:
        print(f"Moving {python_home} to {tmp_python_home}")
        shutil.move(python_home, tmp_python_home)

        python_interpreter_args = get_python_interpreter_args(tmp_python_home)

        # Note: We need to build opentimelineio from sources in Windows+Debug
        #       because the official wheel links with the release version of
        #       python while RV uses the debug version.
        build_opentimelineio = platform.system() == "Windows" and VARIANT == "Debug"
        if build_opentimelineio:
            print(f"Building opentimelineio")

            # Request an opentimelineio debug build
            my_env = os.environ.copy()
            my_env["OTIO_CXX_DEBUG_BUILD"] = "1"

            # Specify the location of the debug python import lib (eg. python39_d.lib)
            python_include_dirs = os.path.join(tmp_python_home, "include")
            python_lib = os.path.join(tmp_python_home, "libs", f"python{PYTHON_VERSION}_d.lib")
            my_env["CMAKE_ARGS"] = f"-DPython_LIBRARY={python_lib} -DCMAKE_INCLUDE_PATH={python_include_dirs}"

            opentimelineio_install_arg = python_interpreter_args + [
                "-m",
                "pip",
                "install",
                ".",
            ]

            result = subprocess.run(
                opentimelineio_install_arg,
                env=my_env,
                cwd=OPENTIMELINEIO_SOURCE_DIR,
            ).check_returncode()

            # Note : The OpenTimelineIO build will generate the pyd with names that are not loadable by default
            # Example: _opentimed_d.cp39-win_amd64.pyd instead of _opentime_d.pyd
            # and _otiod_d.cp39-win_amd64.pyd instead of _otio_d.pyd
            # We fix those names here
            otio_module_dir = os.path.join(
                tmp_python_home, "lib", "site-packages", "opentimelineio"
            )
            for _file in os.listdir(otio_module_dir):
                if _file.endswith("pyd"):
                    otio_lib_name_split = os.path.basename(_file).split(".")
                    if len(otio_lib_name_split) > 2:
                        new_otio_lib_name = (
                            otio_lib_name_split[0].replace("d_d", "_d") + ".pyd"
                        )
                        src_file = os.path.join(otio_module_dir, _file)
                        dst_file = os.path.join(otio_module_dir, new_otio_lib_name)
                        shutil.copyfile(src_file, dst_file)

        wheel_install_arg = python_interpreter_args + [
            "-m",
            "pip",
            "install",
            "cryptography",
        ]

        if not build_opentimelineio:
            wheel_install_arg.append("opentimelineio")

        print(f"Validating the that we can install a wheel with {wheel_install_arg}")
        subprocess.run(wheel_install_arg).check_returncode()

        python_validation_args = python_interpreter_args + [
            "-c",
            "\n".join(
                [
                    # Check for tkinter
                    "try:",
                    "    import tkinter",
                    "except:",
                    "    import Tkinter as tkinter",
                    # Make sure certifi is available
                    "import certifi",
                    # Make sure the SSL_CERT_FILE variable is sett
                    "import os",
                    "assert certifi.where() == os.environ['SSL_CERT_FILE']",
                    # Make sure ssl is correctly built and linked
                    "import ssl",
                    # Misc
                    "import sqlite3",
                    "import ctypes",
                    "import ssl",
                    "import _ssl",
                    "import zlib",
                ]
            ),
        ]
        print(f"Validating the python package with {python_validation_args}")
        subprocess.run(python_validation_args).check_returncode()

        dummy_ssl_file = os.path.join("Path", "To", "Dummy", "File")
        python_validation2_args = python_interpreter_args + [
            "-c",
            "\n".join(
                [
                    "import os",
                    f"assert os.environ['SSL_CERT_FILE'] == '{dummy_ssl_file}'",
                ]
            ),
        ]
        print(f"Validating the python package with {python_validation2_args}")
        subprocess.run(
            python_validation2_args, env={**os.environ, "SSL_CERT_FILE": dummy_ssl_file}
        ).check_returncode()

        python_validation3_args = python_interpreter_args + [
            "-c",
            "\n".join(
                [
                    "import os",
                    "assert 'SSL_CERT_FILE' not in os.environ",
                ]
            ),
        ]
        print(f"Validating the python package with {python_validation3_args}")
        subprocess.run(
            python_validation3_args,
            env={**os.environ, "DO_NOT_SET_SSL_CERT_FILE": "bleh"},
        ).check_returncode()

    finally:
        print(f"Moving {tmp_python_home} to {python_home}")
        shutil.move(tmp_python_home, python_home)


def clean() -> None:
    """
    Run the clean step of the build. Removes everything.
    """

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)

    if os.path.exists(TEMP_DIR):
        shutil.rmtree(TEMP_DIR)

    git_clean_command = ["git", "clean", "-dxff"]

    print(f"Executing {git_clean_command} from {SOURCE_DIR}")
    subprocess.run(git_clean_command, cwd=SOURCE_DIR)


def configure() -> None:
    """
    Run the configure step of the build. It builds the makefile required to build Python with the path correctly set.
    """

    if platform.system() != "Windows":
        configure_args = [
            os.path.join(SOURCE_DIR, "configure"),
            f"--srcdir={SOURCE_DIR}",
            f"--prefix={OUTPUT_DIR}",
            f"--exec-prefix={OUTPUT_DIR}",
            "--enable-shared",
        ]

        if OPENSSL_OUTPUT_DIR:
            configure_args.append(f"--with-openssl={OPENSSL_OUTPUT_DIR}")


        if VARIANT == "Release":
            configure_args.append("--enable-optimizations")

        CPPFLAGS = []
        LD_LIBRARY_PATH = []

        if platform.system() == "Darwin":
            readline_prefix_proc = subprocess.run(
                ["brew", "--prefix", "readline"], capture_output=True
            )
            readline_prefix_proc.check_returncode()

            tcl_prefix_proc = subprocess.run(
                ["brew", "--prefix", "tcl-tk"], capture_output=True
            )
            tcl_prefix_proc.check_returncode()

            xz_prefix_proc = subprocess.run(
                ["brew", "--prefix", "xz"], capture_output=True
            )
            xz_prefix_proc.check_returncode()

            sdk_prefix_proc = subprocess.run(
                ["xcrun", "--show-sdk-path"], capture_output=True
            )
            sdk_prefix_proc.check_returncode()

            readline_prefix = readline_prefix_proc.stdout.strip().decode("utf-8")
            tcl_prefix = tcl_prefix_proc.stdout.strip().decode("utf-8")
            xz_prefix = xz_prefix_proc.stdout.strip().decode("utf-8")
            sdk_prefix = sdk_prefix_proc.stdout.strip().decode("utf-8")

            CPPFLAGS.append(f'-I{os.path.join(readline_prefix, "include")}')
            CPPFLAGS.append(f'-I{os.path.join(tcl_prefix, "include")}')
            CPPFLAGS.append(f'-I{os.path.join(xz_prefix, "include")}')
            CPPFLAGS.append(f'-I{os.path.join(sdk_prefix, "usr", "include")}')

            LD_LIBRARY_PATH.append(os.path.join(readline_prefix, "lib"))
            LD_LIBRARY_PATH.append(os.path.join(tcl_prefix, "lib"))
            LD_LIBRARY_PATH.append(os.path.join(xz_prefix, "lib"))
            LD_LIBRARY_PATH.append(os.path.join(sdk_prefix, "usr", "lib"))

        LDFLAGS = [f"-L{d}" for d in LD_LIBRARY_PATH]

        configure_args.append(f'CPPFLAGS={" ".join(CPPFLAGS)}')
        configure_args.append(f'LDFLAGS={" ".join(LDFLAGS)}')

        if platform.system() == "Linux":
            configure_args.append(f'LD_LIBRARY_PATH={":".join(LD_LIBRARY_PATH)}')

        print(f"Executing {configure_args} from {SOURCE_DIR}")

        subprocess_env = {**os.environ}
        if OPENSSL_OUTPUT_DIR:
            subprocess_env["LC_RPATH"] = os.path.join(OPENSSL_OUTPUT_DIR, "lib")

        subprocess.run(
            configure_args,
            cwd=SOURCE_DIR,
            env=subprocess_env,
        ).check_returncode()

        makefile_path = os.path.join(SOURCE_DIR, "Makefile")
        old_makefile_path = os.path.join(SOURCE_DIR, "Makefile.old")
        os.rename(makefile_path, old_makefile_path)
        with open(old_makefile_path) as old_makefile:
            with open(makefile_path, "w") as makefile:
                for line in old_makefile:
                    new_line = line.replace(
                        "-Wl,-install_name,$(prefix)/lib/libpython$(",
                        "-Wl,-install_name,@rpath/libpython$(",
                    )

                    # Adjust RPaths
                    if new_line.startswith("LINKFORSHARED="):
                        if platform.system() == "Linux":
                            makefile.write(
                                new_line.strip()
                                + " -Wl,-rpath,'$$ORIGIN/../lib',-rpath,'$$ORIGIN/../Qt'"
                                + "\n"
                            )
                        elif platform.system() == "Darwin":
                            makefile.write(
                                new_line.strip()
                                + " -Wl,-rpath,@executable_path/../lib,-rpath,@executable_path/../Qt"
                                + "\n"
                            )
                    else:
                        makefile.write(new_line)


def build() -> None:
    """
    Run the build step of the build. It compile every target of the project.
    """

    if platform.system() == "Windows":

        build_args = [
            os.path.join(SOURCE_DIR, "PCBuild", "build.bat"),
            "-p",
            "x64",
            "-c",
            VARIANT,
            "-t",
            "Rebuild",
        ]

        print(f"Executing {build_args} from {SOURCE_DIR}")

        path_env = os.path.pathsep.join(
            [
                os.path.dirname(sys.executable),
                os.environ.get("PATH", ""),
            ]
        )

        python_env = sys.executable

        subprocess_env = {**os.environ, "PYTHON": python_env, "PATH": path_env}
        if OPENSSL_OUTPUT_DIR:
            subprocess_env["LC_RPATH"] = os.path.join(OPENSSL_OUTPUT_DIR, "lib")


        subprocess.run(
            build_args,
            cwd=SOURCE_DIR,
            env=subprocess_env,
        ).check_returncode()
    else:
        make_args = ["make", f"-j{os.cpu_count() or 1}"]

        print(f"Executing {make_args} from {SOURCE_DIR}")
        subprocess_env = {**os.environ}
        if OPENSSL_OUTPUT_DIR:
            subprocess_env["LC_RPATH"] = os.path.join(OPENSSL_OUTPUT_DIR, "lib")

        subprocess.run(
            make_args,
            cwd=SOURCE_DIR,
            env=subprocess_env,
        ).check_returncode()


def install() -> None:
    """
    Run the install step of the build. It puts all the files inside of the output directory and make them ready to be
    packaged.
    """

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)

    if platform.system() == "Windows":
        # include
        src_dir = os.path.join(SOURCE_DIR, "Include")
        dst_dir = os.path.join(OUTPUT_DIR, "include")
        shutil.copytree(src_dir, dst_dir)
        src_file = os.path.join(SOURCE_DIR, "PC", "pyconfig.h")
        dst_file = os.path.join(dst_dir, "pyconfig.h")
        shutil.copyfile(src_file, dst_file)

        # lib
        src_dir = os.path.join(SOURCE_DIR, "Lib")
        dst_dir = os.path.join(OUTPUT_DIR, "lib")
        shutil.copytree(src_dir, dst_dir)

        # libs - required by pyside2
        dst_dir = os.path.join(OUTPUT_DIR, "libs")
        os.mkdir(dst_dir)
        python_libs = glob.glob(
            os.path.join(SOURCE_DIR, "PCBuild", "amd64", "python*.lib")
        )
        if python_libs:
            for python_lib in python_libs:
                shutil.copy(python_lib, dst_dir)

        # bin
        src_dir = os.path.join(SOURCE_DIR, "PCBuild", "amd64")
        dst_dir = os.path.join(OUTPUT_DIR, "bin")
        shutil.copytree(src_dir, dst_dir)
        # Create a python3.exe file to mimic Mac+Linux
        if VARIANT == "Debug":
            src_python_exe = "python_d.exe"
        else:
            src_python_exe = "python.exe"
        src_file = os.path.join(src_dir, src_python_exe)
        dst_file = os.path.join(dst_dir, "python3.exe")
        shutil.copyfile(src_file, dst_file)

    else:
        make_args = ["make", "install", f"-j{os.cpu_count() or 1}", "-s"]

        print(f"Executing {make_args} from {SOURCE_DIR}")
        subprocess_env = {**os.environ}
        if OPENSSL_OUTPUT_DIR:
            subprocess_env["LC_RPATH"] = os.path.join(OPENSSL_OUTPUT_DIR, "lib")
        subprocess.run(
            make_args,
            cwd=SOURCE_DIR,
            env=subprocess_env,
        ).check_returncode()

    # Create the 'python' symlink
    if platform.system() != "Windows":
        python3_path = os.path.realpath(os.path.join(OUTPUT_DIR, "bin", "python3"))
        python_path = os.path.join(os.path.dirname(python3_path), "python")
        os.symlink(os.path.basename(python3_path), python_path)

    patch_python_distribution(OUTPUT_DIR)
    test_python_distribution(OUTPUT_DIR)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--clean", dest="clean", action="store_true")
    parser.add_argument("--configure", dest="configure", action="store_true")
    parser.add_argument("--build", dest="build", action="store_true")
    parser.add_argument("--install", dest="install", action="store_true")

    parser.add_argument("--source-dir", dest="source", type=pathlib.Path, required=True)
    parser.add_argument(
        "--openssl-dir", dest="openssl", type=pathlib.Path, required=False
    )
    parser.add_argument("--temp-dir", dest="temp", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", dest="output", type=pathlib.Path, required=True)

    parser.add_argument("--variant", dest="variant", type=str, required=True)
    parser.add_argument("--arch", dest="arch", type=str, required=False, default="")

    parser.add_argument(
        "--opentimelineio-source-dir",
        dest="otio_source_dir",
        type=str,
        required=False,
        default="",
    )

    if platform.system() == "Windows":
        # Major and minor version of Python without dots. E.g. 3.10.3 -> 310
        parser.add_argument("--python-version", dest="python_version", type=str, required=True, default="")

    parser.set_defaults(clean=False, configure=False, build=False, install=False)

    args = parser.parse_args()

    SOURCE_DIR = args.source
    OUTPUT_DIR = args.output
    TEMP_DIR = args.temp
    OPENSSL_OUTPUT_DIR = args.openssl
    VARIANT = args.variant
    ARCH = args.arch
    OPENTIMELINEIO_SOURCE_DIR = args.otio_source_dir

    if platform.system() == "Windows":
        PYTHON_VERSION = args.python_version

    if args.clean:
        clean()

    if args.configure:
        configure()

    if args.build:
        build()

    if args.install:
        install()