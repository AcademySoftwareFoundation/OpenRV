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

from typing import List

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(ROOT_DIR)

OPENSSL_OUTPUT_DIR = ""
SOURCE_DIR = ""
OUTPUT_DIR = ""
TEMP_DIR = ""
VARIANT = ""
ARCH = ""

LIB_DIR = ""


def get_sitecustomize_content() -> str:
    """
    Load and return the sitecustomize.py content.

    :return: The sitecustomize.py content as a string
    """
    template_path = os.path.join(ROOT_DIR, "sitecustomize.py")
    with open(template_path, "r") as f:
        return f.read()


def get_python_interpreter_args(python_home: str, variant: str) -> List[str]:
    """
    Return the path to the python interpreter given a Python home

    :param python_home: Python home of a Python package
    :return: Path to the python interpreter
    """

    # On Windows Debug, use the debug Python interpreter (python_d.exe)
    use_debug_python = platform.system() == "Windows" and variant == "Debug"
    python_name_pattern = "python*" if not use_debug_python else "python_d*"

    python_interpreters = glob.glob(os.path.join(python_home, python_name_pattern), recursive=True)
    python_interpreters += glob.glob(os.path.join(python_home, "bin", python_name_pattern))

    # sort put python# before python#-config, prioritize debug on Windows debug builds
    python_interpreters = sorted(
        [p for p in python_interpreters if os.path.islink(p) is False and os.access(p, os.X_OK)],
        key=lambda p: (
            "-config" in os.path.basename(p),  # config variants last
            not (
                "_d" in os.path.basename(p) and platform.system() == "Windows" and variant.lower() == "debug"
            ),  # prioritize _d on Windows debug
            os.path.basename(p),  # alphabetical
        ),
    )

    if not python_interpreters:
        raise FileNotFoundError(
            f"No Python interpreter found in {python_home}. "
            f"Searched for pattern '{python_name_pattern}' in {python_home} (recursively) and {os.path.join(python_home, 'bin')}. "
        )

    if not os.path.exists(python_interpreters[0]):
        raise FileNotFoundError(
            f"Python interpreter does not exist: {python_interpreters[0]}. Found interpreters: {python_interpreters}"
        )

    print(f"Found python interpreters {python_interpreters}")

    python_interpreter = sorted(python_interpreters)[0]

    return [python_interpreter, "-s", "-E"]


def patch_python_distribution(python_home: str) -> None:
    """
    Patch the python distribution to make it relocatable.
    Patch the python site-packages to rely on certifi to get the SSL certificates.

    :param python_home: Home of the Python to patch.
    """

    for failed_lib in glob.glob(os.path.join(python_home, "lib", "**", "*_failed.so"), recursive=True):
        # The Python build mark some so as _failed if it cannot load OpenSSL. In our case it's expected because
        # out OpenSSL package works with RPATH and the RPATH is not set on the python build tests. If the lib failed
        # for other reasons, it will fail later in our build script.
        if "ssl" in failed_lib or "hashlib" in failed_lib:
            print(f"Fixing {failed_lib}")
            shutil.move(failed_lib, failed_lib.replace("_failed.so", ".so"))
    if OPENSSL_OUTPUT_DIR:
        if platform.system() == "Darwin":
            openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "lib", "lib*.dylib*"))
            openssl_libs = [lib for lib in openssl_libs if os.path.islink(lib) is False]

            python_openssl_libs = []

            for lib_path in openssl_libs:
                print(f"Copying {lib_path} to the python home")
                dest = os.path.join(python_home, "lib", os.path.basename(lib_path))
                shutil.copyfile(lib_path, dest)
                python_openssl_libs.append(dest)

            libs_to_patch = glob.glob(os.path.join(python_home, "lib", "**", "*.so"), recursive=True)

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
            openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, LIB_DIR, "lib*.so*"))

            for lib_path in openssl_libs:
                print(f"Copying {lib_path} to the python home")
                shutil.copy(lib_path, os.path.join(python_home, LIB_DIR))

        elif platform.system() == "Windows":
            openssl_dlls = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "bin", "lib*"))
            for dll_path in openssl_dlls:
                print(f"Copying {dll_path} to the python home")
                shutil.copy(dll_path, os.path.join(python_home, "bin"))

            openssl_libs = glob.glob(os.path.join(OPENSSL_OUTPUT_DIR, "lib", "lib*"))
            for lib_path in openssl_libs:
                print(f"Copying {lib_path} to the python home")
                shutil.copy(lib_path, os.path.join(python_home, "libs"))

    python_interpreter_args = get_python_interpreter_args(python_home, VARIANT)

    # -I : isolate Python from the user's environment
    python_interpreter_args.append("-I")

    ensure_pip_args = python_interpreter_args + ["-m", "ensurepip"]
    print(f"Ensuring pip with {ensure_pip_args}")
    subprocess.run(ensure_pip_args).check_returncode()

    site_packages = glob.glob(os.path.join(python_home, "**", "site-packages"), recursive=True)[0]

    if os.path.exists(site_packages) is False:
        raise FileNotFoundError()

    print(f"Site packages found at {site_packages}")

    site_customize_path = os.path.join(site_packages, "sitecustomize.py")
    if os.path.exists(site_customize_path):
        os.remove(site_customize_path)

    print(f"Installing the sitecustomize.py payload at {site_customize_path}")
    site_customize_path = os.path.join(site_packages, "sitecustomize.py")
    if os.path.exists(site_customize_path):
        print("Found a sitecustomize.py in the site-packages, the content of the file will be overwritten")
        os.remove(site_customize_path)

    with open(site_customize_path, "w") as sitecustomize_file:
        sitecustomize_file.write(get_sitecustomize_content())


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


def add_path_to_env_var(env, envvar: str, paths: list) -> None:
    current_value = env.get(envvar, "")
    for path in paths:
        p = pathlib.Path(path).resolve()
        if current_value:
            current_value = f"{p.__str__()}{os.pathsep}{current_value}"
        else:
            current_value = p.__str__()
    env[envvar] = current_value


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

            if platform.system() == "Linux":
                # This option prevent Python to build the _ssl module correctly on Darwin.
                configure_args.append(f"--with-openssl-rpath={OPENSSL_OUTPUT_DIR}/{LIB_DIR}")

        if VARIANT == "Release":
            configure_args.append("--enable-optimizations")

        CPPFLAGS = []
        LD_LIBRARY_PATH = []
        PKG_CONFIG_PATH = []

        if platform.system() == "Darwin":
            readline_prefix_proc = subprocess.run(["brew", "--prefix", "readline"], capture_output=True)
            readline_prefix_proc.check_returncode()

            tcl_prefix_proc = subprocess.run(["brew", "--prefix", "tcl-tk@8"], capture_output=True)
            tcl_prefix_proc.check_returncode()

            python_tk_prefix_proc = subprocess.run(["brew", "--prefix", "python-tk"], capture_output=True)
            python_tk_prefix_proc.check_returncode()

            xz_prefix_proc = subprocess.run(["brew", "--prefix", "xz"], capture_output=True)
            xz_prefix_proc.check_returncode()

            sdk_prefix_proc = subprocess.run(["xcrun", "--show-sdk-path"], capture_output=True)
            sdk_prefix_proc.check_returncode()

            readline_prefix = pathlib.Path(readline_prefix_proc.stdout.strip().decode("utf-8")).resolve().__str__()
            tcl_prefix = pathlib.Path(tcl_prefix_proc.stdout.strip().decode("utf-8")).resolve().__str__()
            xz_prefix = pathlib.Path(xz_prefix_proc.stdout.strip().decode("utf-8")).resolve().__str__()
            sdk_prefix = pathlib.Path(sdk_prefix_proc.stdout.strip().decode("utf-8")).resolve().__str__()
            python_tk_prefix = pathlib.Path(python_tk_prefix_proc.stdout.strip().decode("utf-8")).resolve().__str__()

            CPPFLAGS.append(f"-I{os.path.join(readline_prefix, 'include')}")
            CPPFLAGS.append(f"-I{os.path.join(tcl_prefix, 'include')}")
            CPPFLAGS.append(f"-I{os.path.join(xz_prefix, 'include')}")
            CPPFLAGS.append(f"-I{os.path.join(sdk_prefix, 'usr', 'include')}")
            CPPFLAGS.append(f"-I{os.path.join(python_tk_prefix, 'include')}")

            LD_LIBRARY_PATH.append(os.path.join(readline_prefix, "lib"))
            LD_LIBRARY_PATH.append(os.path.join(tcl_prefix, "lib"))
            LD_LIBRARY_PATH.append(os.path.join(xz_prefix, "lib"))
            LD_LIBRARY_PATH.append(os.path.join(sdk_prefix, "usr", "lib"))
            LD_LIBRARY_PATH.append(os.path.join(python_tk_prefix, "lib"))

            PKG_CONFIG_PATH.append(os.path.join(tcl_prefix, "lib", "pkgconfig"))

        LDFLAGS = [f"-L{d}" for d in LD_LIBRARY_PATH]

        if OPENSSL_OUTPUT_DIR:
            if platform.system() == "Darwin":
                configure_args.append(f"LC_RPATH={os.path.join(OPENSSL_OUTPUT_DIR, LIB_DIR)}")
                configure_args.append(f"LDFLAGS={' '.join(LDFLAGS)}")
                configure_args.append(f"CPPFLAGS={' '.join(CPPFLAGS)}")
                configure_args.append(f"DYLD_LIBRARY_PATH={os.path.join(OPENSSL_OUTPUT_DIR, LIB_DIR)}")
            else:
                # Linux (NOT Windows was checked before)
                configure_args.append(f"LDFLAGS=-L{OPENSSL_OUTPUT_DIR}/{LIB_DIR}")
                configure_args.append(f"CPPFLAGS=-I{OPENSSL_OUTPUT_DIR}/include")

        subprocess_env = {**os.environ}
        if platform.system() == "Darwin":
            add_path_to_env_var(subprocess_env, "PKG_CONFIG_PATH", PKG_CONFIG_PATH)

        print(f"Executing {configure_args} from {SOURCE_DIR}")

        subprocess.run(configure_args, cwd=SOURCE_DIR, env=subprocess_env).check_returncode()

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
                                new_line.strip() + " -Wl,-rpath,'$$ORIGIN/../lib',-rpath,'$$ORIGIN/../Qt'" + "\n"
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
            subprocess_env["LC_RPATH"] = os.path.join(OPENSSL_OUTPUT_DIR, LIB_DIR)

        subprocess.run(
            make_args,
            cwd=SOURCE_DIR,
            env=subprocess_env,
        ).check_returncode()


def install_python_vfx2023() -> None:
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
        python_libs = glob.glob(os.path.join(SOURCE_DIR, "PCBuild", "amd64", "python*.lib"))
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
    # Note: Testing is now done via test_python.py after requirements.txt installation


def install_python_vfx2024() -> None:
    """
    Run the install step of the build. It puts all the files inside of the output directory and make them ready to be
    packaged.
    """

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)

    if platform.system() == "Windows":
        build_path = os.path.join(SOURCE_DIR, "PCBuild", "amd64")
        parent = os.path.dirname(SOURCE_DIR)

        # Using the python that OpenRV built to execute the script because it seems
        # like it need to be the same dot release. (e.g. run the script with 3.11 to install a 3.11)
        python_executable = "python"
        if VARIANT == "Debug":
            python_executable = "python_d"
        python_executable = os.path.join(build_path, python_executable)

        install_args = [
            python_executable,
            os.path.join(SOURCE_DIR, "PC", "layout", "main.py"),
            "-vv",
            "--source",
            SOURCE_DIR,
            "--build",
            build_path,
            "--copy",
            OUTPUT_DIR,
            "--temp",
            os.path.join(parent, "temp"),
            "--preset-default",
            "--include-dev",
            "--include-symbols",
            "--include-tcltk",
            "--include-tests",
            "--include-venv",
        ]

        if VARIANT == "Debug":
            install_args.append("--debug")

        print(f"Executing {install_args}")

        subprocess.run(install_args, cwd=SOURCE_DIR).check_returncode()

        # bin
        libs_dir = os.path.join(OUTPUT_DIR, "libs")
        dst_dir = os.path.join(OUTPUT_DIR, "bin")

        # Create a python3.exe file to mimic Mac+Linux
        if VARIANT == "Debug":
            src_python_exe = "python_d.exe"
        else:
            src_python_exe = "python.exe"

        src_file = os.path.join(OUTPUT_DIR, src_python_exe)
        dst_file = os.path.join(dst_dir, "python3.exe")
        print(f"Copy {src_file} to {dst_file}")

        os.makedirs(dst_dir, exist_ok=True)
        shutil.copyfile(src_file, dst_file)

        # Move files under root directory into the bin folder.
        for filename in os.listdir(os.path.join(OUTPUT_DIR)):
            file_path = os.path.join(OUTPUT_DIR, filename)
            if os.path.isfile(file_path):
                shutil.move(file_path, os.path.join(dst_dir, filename))

        # Manually move python3.lib because the script provided do not copy it.
        python3_lib = "python3.lib"
        python3xx_lib = f"python{PYTHON_VERSION}.lib"

        if VARIANT == "Debug":
            python3_lib = "python3_d.lib"
            python3xx_lib = f"python{PYTHON_VERSION}_d.lib"

        shutil.copy(os.path.join(build_path, python3_lib), os.path.join(dst_dir, python3_lib))
        shutil.copy(os.path.join(build_path, python3_lib), os.path.join(libs_dir, python3_lib))
        shutil.copy(os.path.join(build_path, python3xx_lib), os.path.join(dst_dir, python3xx_lib))

        # Tcl and Tk DLL are not copied by the main.py script in Debug.
        # Assuming that Tcl and Tk are not built in debug.
        # Manually copy the DLL.
        if VARIANT == "Debug":
            tcl_dll = os.path.join(build_path, "tcl86t.dll")
            tk_dll = os.path.join(build_path, "tk86t.dll")
            shutil.copyfile(tcl_dll, os.path.join(dst_dir, "tcl86t.dll"))
            shutil.copyfile(tk_dll, os.path.join(dst_dir, "tk86t.dll"))

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
    # Note: Testing is now done via test_python.py after requirements.txt installation


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--clean", dest="clean", action="store_true")
    parser.add_argument("--configure", dest="configure", action="store_true")
    parser.add_argument("--build", dest="build", action="store_true")
    parser.add_argument("--install", dest="install", action="store_true")

    parser.add_argument("--source-dir", dest="source", type=pathlib.Path, required=True)
    parser.add_argument("--openssl-dir", dest="openssl", type=pathlib.Path, required=False)
    parser.add_argument("--temp-dir", dest="temp", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", dest="output", type=pathlib.Path, required=True)

    parser.add_argument("--variant", dest="variant", type=str, required=True)
    parser.add_argument("--arch", dest="arch", type=str, required=False, default="")

    parser.add_argument("--vfx_platform", dest="vfx_platform", type=int, required=True)

    if platform.system() == "Windows":
        # Major and minor version of Python without dots. E.g. 3.10.3 -> 310
        parser.add_argument(
            "--python-version",
            dest="python_version",
            type=str,
            required=True,
            default="",
        )

    parser.set_defaults(clean=False, configure=False, build=False, install=False)

    args = parser.parse_args()

    SOURCE_DIR = args.source
    OUTPUT_DIR = args.output
    TEMP_DIR = args.temp
    OPENSSL_OUTPUT_DIR = args.openssl
    VARIANT = args.variant
    ARCH = args.arch
    VFX_PLATFORM = args.vfx_platform

    if platform.system() == "Darwin":
        LIB_DIR = "lib"
    else:
        # Assuming Linux because that variable is not used for Windows.
        # TODO: Note: This might not be right on Debian based platform.
        if VFX_PLATFORM == 2023:
            LIB_DIR = "lib"
        elif VFX_PLATFORM >= 2024:
            LIB_DIR = "lib64"

    if platform.system() == "Windows":
        PYTHON_VERSION = args.python_version

    if args.clean:
        clean()

    if args.configure:
        configure()

    if args.build:
        build()

    if args.install:
        # The install functions has a lot of similiarity but I think it is better to have two differents
        # functions because it will be easier to drop the support for a VFX platform.
        if VFX_PLATFORM == 2023:
            install_python_vfx2023()
        elif VFX_PLATFORM >= 2024:
            install_python_vfx2024()
