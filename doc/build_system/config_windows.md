# Building Open RV on Windows

## Summary

1. [Install Microsoft Visual Studio](#1-install-microsoft-visual-studio)
1. [Install Qt](#2-install-qt)
1. [Install Strawberry Perl](#3-install-strawberry-perl)
1. [Install MSYS2](#4-install-msys2)
1. [Install required MSYS2 pacman packages (from an MSYS2-MinGW64 shell)](#5-install-required-msys2-pacman-packages)
1. [Install the Python requirements (from an MSYS2-MinGW64 shell)](#6-install-the-python-requirements)
1. [Clone the RV repo (from an MSYS2-MinGW64 shell)](#7-clone-the-rv-repo)
1. [Invoke CMake](#8-invoke-cmake)

## 1. Install Microsoft Visual Studio

Install Microsoft Visual Studio 2017 & 2019.

Any edition of Microsoft Visual Studio 2019 should do, even the Microsoft Visual Studio 2019 Community Edition. Download it from the Microsoft [older downloads page](https://visualstudio.microsoft.com/vs/older-downloads/).

Make sure to select the "Desktop development with C++" and the latest SDK for Windows 10 or Windows 11 features when installing Microsoft Visual Studio.

You select the Windows SDK based on the target Windows version you plan on running the compiled application on.

Note: The current version of PySide2 required by RV (5.15.2.1) cannot be built with Microsoft Visual Studio 2019 : it can only be built with Microsoft Visual Studio 2017. Therefore, Microsoft Visual Studio 2017 needs to be installed as well.

## 2. Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.

Note: You will also need `jom`, and it is included with Qt Creator (available from the Qt online installer). If you do not want to install Qt Creator, you can download it from [here](https://download.qt.io/official_releases/jom/) and copy the executable into the QT installation root directory under Tools/QtCreator/bin/jom.

## 3. Install Strawberry Perl

Download and install the 64-bit version of [Strawberry Perl](https://strawberryperl.com/)

## 4. Install MSYS2

Download and install the latest [MSYS2](https://www.msys2.org/).

Note that RV is NOT a mingw64 build. It is a Microscoft Visual Studio 2019 build.

RV is built with Microsoft Visual Studio 2019 via the cmake "Visual Studio 16 2019" generator.

msys2 is only used for convenience as it comes with a package manager with utility packages required for the RV build such as cmake, git, flex, bison, nasm, unzip, zip, etc.

NOTE: The Windows' WSL2 feature conflict with MSYS2. For simplicity, it is recommanded to disable Windows' WSL or WSL2 feature entirely.

Additional information can be found on the [MSYS2 github](https://github.com/msys2/setup-msys2/issues/52).

## 5. Install required MSYS2 pacman packages

From an MSYS2-MinGW64 shell, install the following packages which are required to build RV:

```shell
pacman -S --needed \
        mingw-w64-x86_64-autotools \
        mingw-w64-x86_64-cmake \
        mingw-w64-x86_64-glew \
        mingw-w64-x86_64-libarchive \
        mingw-w64-x86_64-make \
        mingw-w64-x86_64-meson \
        mingw-w64-x86_64-python-pip \
        mingw-w64-x86_64-python-psutil \
        mingw-w64-x86_64-toolchain \
        bison \
        flex \
        git \
        nasm \
        p7zip \
        unzip \
        zip
```

## 6. Install the Python requirements

From an MSYS2-MinGW64 shell, install the required python dependencies using:

```shell
python3 -m pip install --user -r requirements.txt
```

Note: Enter the following command from the same MSYS2-MinGW64 shell if you encounter an error when installing the py7zr python requirement as it seems to be an issue with the latest version of MSYS2:

```shell
SETUPTOOLS_USE_DISTUTILS=stdlib pip install py7zr
```

Note that to successfully build Open RV in debug on Windows, you must also install python3 ([download page](https://www.python.org/downloads/)) as it is required to build the opentimelineio python wheel in debug.

This step is not required if you do not intend to build the debug version of RV.

## 7. Clone the RV repo

From an MSYS2-MinGW64 shell:

```shell
cd /c
git clone git@github.com:AcademySoftwareFoundation/OpenRV.git

OR

git clone https://github.com/AcademySoftwareFoundation/OpenRV.git
```

NOTE: Even as of Windows 11, for legacy reason, a default system path length is still limited to 254 bytes long.
For that reason we strongly suggest cloning `OpenRV` into drive's root directory e.g.: `C:\`

## 8. Invoke CMake

From an MSYS2-MinGW64 shell:

```shell
cmake -B cmake-build -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DRV_DEPS_WIN_PERL_ROOT=/c/Strawberry/perl/bin -DRV_DEPS_QT5_LOCATION=/c/Qt/Qt5.15.11/5.15.11/msvc2019_64
cmake --build cmake-build --config Release --target rv -j8 
cmake --install cmake-build --config Release
```

NOTE: It is recommanded to use Unix-like syntax with the`RV_DEPS_QT5_LOCATION` defininition, otherwise one might use the Windows-like syntax such as `C:\\QT\\5.15.11...` note of the escaping of the backslash characters.
