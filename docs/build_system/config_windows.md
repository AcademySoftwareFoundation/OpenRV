# Building Open RV on Windows

## Summary

1. [Install Microsoft Visual Studio](#1-install-microsoft-visual-studio)
1. [Install Qt](#2-install-qt)
1. [Install Strawberry Perl](#3-install-strawberry-perl)
1. [Install MSYS2](#4-install-msys2)
1. [Install required MSYS2 pacman packages (from an MSYS2-MinGW64 shell)](#5-install-required-msys2-pacman-packages)


## 1. Install Microsoft Visual Studio

Install Microsoft Visual Studio 2022.

Any edition of Microsoft Visual Studio 2022 should do, even the Microsoft Visual Studio 2022 Community Edition. Download it from the Microsoft [older downloads page](https://visualstudio.microsoft.com/vs/older-downloads/).

Make sure to select the "Desktop development with C++" and the latest SDK for Windows 10 or Windows 11 features when installing Microsoft Visual Studio.

You select the Windows SDK based on the target Windows version you plan on running the compiled application on.

## 2. Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.

Note: You will also need `jom`, and it is included with Qt Creator (available from the Qt online installer). If you do not want to install Qt Creator, you can download it from [here](https://download.qt.io/official_releases/jom/) and copy the executable and other files into the QT installation root directory under the Tools/QtCreator/bin/jom folder.

WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package. 

Note: Qt from MSYS2 is missing QtWebEngine.

## 3. Install Strawberry Perl

Download and install the 64-bit version of [Strawberry Perl](https://strawberryperl.com/)

## 4. Install MSYS2

Download and install the latest [MSYS2](https://www.msys2.org/).

Note that RV is NOT a mingw64 build. It is a Microscoft Visual Studio 2022 build.

RV is built with Microsoft Visual Studio 2022 via the cmake "Visual Studio 17 2022" generator.

msys2 is only used for convenience as it comes with a package manager with utility packages required for the RV build such as cmake, git, flex, bison, nasm, unzip, zip, etc.

NOTE: The Windows' WSL2 feature conflict with MSYS2. For simplicity, it is highly recommended to disable Windows' WSL or WSL2 feature entirely.

Additional information can be found on the [MSYS2 github](https://github.com/msys2/setup-msys2/issues/52).

## 5. Install required MSYS2 pacman packages

![MSYS2-MinGW64](../images/rv-msys2-mingw64-shortcut.png)

From an MSYS2-MinGW64 shell, install the following packages which are required to build RV:

```shell
pacman -Sy --needed \
        mingw-w64-x86_64-autotools \
        mingw-w64-x86_64-cmake \
        mingw-w64-x86_64-cmake-cmcldeps \
        mingw-w64-x86_64-glew \
        mingw-w64-x86_64-libarchive \
        mingw-w64-x86_64-make \
        mingw-w64-x86_64-meson \
        mingw-w64-x86_64-python-pip \
        mingw-w64-x86_64-python-psutil \
        mingw-w64-x86_64-toolchain \
        autoconf  \
        automake \
        bison \
        flex \
        git \
        libtool \
        nasm \
        p7zip \
        patch \
        unzip \
        zip
```

While installing the MSYS packages, review the list for any missing package. Some packages might not be installed after the first command.

Note: To confirm which version/location of any tool used inside the MSYS shell, `where` can be used e.g. `where python`. If there's more than one path return, the top one will be used.

### Setting the PATH

The path to MSYS2's mingw64/bin folder must be added to the path.
To set your PATH correctly: edit the MSYS2's ~/.bashrc file and add the following line:
`PATH=/c/msys64/mingw64/bin:${PATH}`

Also add the following line to your ~/.bashrc file:
`export ACLOCAL_PATH=/c/msys64/usr/share/aclocal/`

### Python

You must use Python from mingw64 (msys64/mingw64/bin/python.exe) or your own. Therefore, you must set your PATH EnvVar correctly. Python must be BEFORE **msys64/usr/bin**.

Reminder: you must install, via pip, the requirements which are contained at the root of the project in the file requirements.txt. You must start pip from your mingw64 python executable. If there's any errors while installing via pip, see build_system/build_errors.md.

**Building DEBUG**

To successfully build Open RV in debug on Windows, you must also install a Windows-native python3 ([download page](https://www.python.org/downloads/)) as it is required to build the opentimelineio python wheel in debug.

This step is not required if you do not intend to build the debug version of RV.

### Ninja

A large part of the RV build on Windows uses Ninja. To use Ninja on MSYS, you must add **msys64/mingw64/bin** to your PATH. Ninja is installed as a dependency for `meson` hence it doesn't need to be manually installed.

### CMake

If you have your own install of CMake on your computer, your PATH will need to pick mingw's CMake and not your own. `cmake --help` must return that the default is Ninja e.g. : `* Ninja`. Usually CMake on Windows will default to **Visual Studio** but mingw's CMake defaults to **Ninja which OpenRV is dependent on**.

Symptoms of using Windows' (and not mingw64) CMake:
- During the build, Python scripts such as quoteFile.py, make_openssl.py and make_python.py fail: either they don't work or they aren't found.
- `build.ninja not found` during the build of a dependency. (cmake/dependencies) Reason: CMake in ExternalProject_Add *configured* the Project with the Visual Studio Generator whereas OpenRV's ExternalProject_Add build command expects that Ninja will be used.


### NOTE: Before starting a build

To maximize your chances of successfully building RV, you must:
- Fully update your code base to the latest version (or the version you want to use) with a command like `git pull`.
- Fix all conflicts due to updating the code.
- Revisit all modified files to ensure they aren't using old code that changed during the update such as when the Visual Studio version changes.

### NOTE: Path Length

Even as of Windows 11, for legacy reason, a default system path length is still limited to 254 bytes long.
For that reason we strongly suggest cloning `OpenRV` into drive's root directory e.g.: `C:\`
