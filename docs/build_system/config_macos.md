# Building Open RV on macOS

(summary)=
## Summary

- [Summary](summary)
- [Allow Terminal to update or delete other applications](allow_terminal)
- [Install XCode](install_xcode)
- [Install Homebrew](install_homebrew)
- [Install tools and build dependencies](install_tools_and_build_dependencies)
- [Install Qt](install_qt)
- [Build Open RV](build_openrv)

````{warning}
**Qt Open Source version 5.15.2** is the latest with publicly available binaries, but it lacks *arm64* libraries. 
Therefore, OpenRV builds using **Qt 5.15.2** are limited to *x86_64* architecture. To build natively on *arm64*, you will
have to build a recent version of Qt 5 from source or use the commercial version.

See [Qt](install_qt) section for more information
````

````{note}
OpenRV can be build for *x86_64* by changing the architecture of the terminal to *x86_64* with the following command:
````arch -x86_64 $SHELL````

**It is important to use that *x86_64* terminal for all the subsequent steps.**
````

(allow_terminal)=
## Allow Terminal to update or delete other applications

From the macOS System Settings/Privacy & Security/App Management, allow Terminal to update or delete other applications.

(install_xcode)=
## Install XCode

From the App Store, download XCode 14.3.1. Make sure that it's the source of the active developer directory.
Note that using an XCode version more recent than 14.3.1 will result in an FFmpeg build break.

`xcode-select -p` should return `/Applications/Xcode.app/Contents/Developer`. If it's not the case, run `sudo xcode-select -s /Applications/Xcode.app`

Note that XCode 15 is not compatible with Boost 1.80. If XCode 15 is installed, RV will automatically default to using Boost 1.81 instead. 
Install XCode 14.3.1 if you absolutely want to use Boost version 1.80 as per VFX reference platform CY2023.

Please reference [this workaround](https://forums.developer.apple.com/forums/thread/734709) to use XCode 14.3.1 on Sonoma, as it is no longer 
compatible by default.

(install_homebrew)=
## Install Homebrew

Homebrew is the one stop shop providing all the build requirements. You can install it following the instructions on the [Homebrew page](https://brew.sh).

Make sure Homebrew's binary directory is in your PATH and that `brew` is resolved from your terminal.

(install_tools_and_build_dependencies)=
## Install tools and build dependencies

Most of the build requirements can be installed by running the following brew install command:

```bash
brew install cmake ninja readline sqlite3 xz zlib tcl-tk autoconf automake libtool python yasm clang-format black meson nasm pkg-config glew
```

Make sure `python` resolves in your terminal. In some case, depending on how the python formula is built, there's no `python` symbolic link.
In that case, you can create one with this command `ln -s python3 $(dirname $(which python3))/python`.

(install_qt)=
## Install Qt

````{warning}
For arm64, Qt must be compiled from source because the latest version is needed. Qt 5.15.2 does not have arm64 support.

For x86_64, Qt 5.15.2 can be used - but OpenRV must be built within a x86_64 terminal. You can change the architecture 
of the terminal with this command: ````arch -x86_64 $SHELL````

````

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.


WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.

Note: Qt5 from homebrew is known to not work well with OpenRV.


### Quick guide to build Qt from source

````{warning}
If you really do not need a arm64 build, it is **recommended** to build OpenRV for **x86_64** and use **Qt 5.15.2**. \
\
Building Qt from source is **difficult** even for developpers, and takes some times depending on your machine.
````

````{note}
The quick guide provided here is based
 on the [OpenRV GitHub Action workflow](https://github.com/AcademySoftwareFoundation/OpenRV/blob/main/.github/actions/build-qt5-for-arm64/action.yml) that OpenRV uses to build Qt from source for
arm64.

Quick list of the dependencies: \
XCode 14, homebrew and a multiple of packages, Ninja 1.11.1, Python2 and Python3.
````

Here is the quick guide on how to build the latest Qt for arm64:
````bash
# Adapt the version for the XCode present on your machine. 
# XCode 14 must be used.
sudo xcode-select -switch /Applications/Xcode_14.3.1.app
````

````bash
# Install all the homebrew pacakges
brew install --quiet --formula libiconv libpng libpq libtool libuv libxau libxcb libxdmcp
brew install --quiet --formula autoconf automake cmake pcre2 harfbuzz freetype node@18 nspr nss
brew install --quiet --formula xcb-proto xcb-util xcb-util-cursor xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm
brew install --quiet --formula brotli bzip2 dbus glew icu4c jpeg md4c openssl@1.1 pkg-config sqlite xorgproto zlib zstd
````

````bash
# Install Ninja 1.11.1
wget https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-mac.zip
unzip ninja-mac.zip -d ./ninja
# Add Ninja to the PATH environment variable.
echo 'export PATH=$(pwd)/ninja:$PATH' >> ~/.zprofile
````

````bash
# Install and use pyenv to manage python2 and python3.
curl https://pyenv.run | bash
echo 'export PYENV_ROOT=$HOME/.pyenv' >> ~/.zprofile
echo 'export PATH=$PYENV_ROOT/shims:$PYENV_ROOT/bin:$PATH' >> ~/.zprofile

source ~/.zprofile
pyenv install 3.10.13 2.7.18
pyenv global 3.10.13 2.7.18
````

````bash
# Download Qt 5.15.15 source. A more recent version can be used as well.
curl https://www.nic.funet.fi/pub/mirrors/download.qt-project.org/official_releases/qt/5.15/5.15.15/single/qt-everywhere-opensource-src-5.15.15.tar.xz -o qt.tar.xz
tar xf qt.tar.xz
mv qt-everywhere-src-5.15.15 qt-src
````

````bash
# Create a folder for the build
mkdir -p qt-build
````

````bash
# Change to the qt-build directory.
cd qt-build

# Configure Qt build.
# Make sure that /opt/homebrew/Cellar/openssl@1.1/1.1.1w and /opt/homebrew/Cellar/icu4c/74.2 exists.
../qt-src/configure \
  --prefix="../myQt" \
  -no-strip \
  -no-rpath \
  -opensource \
  -plugin-sql-sqlite \
  -openssl \
  -verbose \
  -opengl desktop \
  -no-warnings-are-errors \
  -no-libudev \
  -no-egl \
  -nomake examples \
  -nomake tests \
  -c++std c++14 \
  -confirm-license \
  -no-use-gold-linker \
  -release \
  -no-sql-mysql \
  -no-xcb \
  -qt-libjpeg \
  -qt-libpng \
  -bundled-xcb-xinput \
  -sysconfdir /etc/xdg \
  -qt-pcre \
  -qt-harfbuzz \
  -R . \
  -icu \
  -skip qtnetworkauth \
  -skip qtpurchasing \
  -skip qtlocation \
  -I /opt/homebrew/Cellar/openssl@1.1/1.1.1w/include -L /opt/homebrew/Cellar/openssl@1.1/1.1.1w/lib \
  -I /opt/homebrew/Cellar/icu4c/74.2/include -L /opt/homebrew/Cellar/icu4c/74.2/lib

# Start the build
make -j$(python -c 'import os; print(os.cpu_count())')

# Install Qt to the folder specified with the --prefix options previously.
make install -j$(python -c 'import os; print(os.cpu_count())')
````

Final step is to set the QT_HOME environment variable for OpenRV build system:
````bash
# Change the path based on the location of the myQt folder that was created previously.
echo "export QT_HOME=<path to...>/myQt/Qt/5.15.15/clang_64" >> ~/.zprofile
source ~/.zprofile
````

(build_openrv)=
## 8. Build Open RV

(build_openrv1)=
### Before executing any commands

To maximize your chances of successfully building Open RV, you must:
- Fully update your code base to the latest version (or the version you want to use) with a command like `git pull`.
- Fix all conflicts due to updating the code.
- Revisit all modified files to ensure they aren't using old code that changed during the update such as when the Visual Studio version changes.

(build_openrv2)=
### Get Open RV source code

Clone the Open RV repository and change directory into the newly created folder. Typically, the command would be:

Using a password-protected SSH key:
```shell
git clone --recursive git@github.com:AcademySoftwareFoundation/OpenRV.git
cd OpenRV
```

Using the web URL:
```shell
git clone --recursive https://github.com/AcademySoftwareFoundation/OpenRV.git
cd OpenRV
```

(build_openrv3)=
### Load aliases for Open RV

From the Open RV directory:
```shell
source rvcmds.sh
```

(build_openrv4)=
### Install Python dependencies

````{note}
This section need to be done only one time when a fresh Open RV repository is cloned. 
The first time the `rvsetup` is executed, it will create a Python virtual environment in the current directory under `.venv`.
````

From the Open RV directory, the following command will download and install the Python dependencies.
```shell
rvsetup
```

(build_openrv5)=
### Configure the project

From the Open RV directory, the following command will configure CMake for the build:

````{tabs}
```{code-tab} bash Release
rvcfg
```
```{code-tab} bash Debug
rvcfgd
```
````

(build_openrv6)=
### Build the dependencies

From the Open RV directory, the following command will build the dependencies:

````{tabs}
```{code-tab} bash Release
rvbuildt dependencies
```
```{code-tab} bash Debug
rvbuildtd dependencies
```
````

(build_openrv7)=
### Build the main executable

From the Open RV directory, the following command will build the main executable:

````{tabs}
```{code-tab} bash Release
rvbuildt main_executable
```
```{code-tab} bash Debug
rvbuildtd main_executable
```
````

(build_openrv8)=
### Opening Open RV executable

````{tabs}
```{tab} Release
Once the build is completed, the Open RV application can be found in the Open RV directory under `_build/stage/app/RV.app/Contents/MacOS/RV`.
```
```{tab} Debug
Once the build is completed, the Open RV application can be found in the Open RV directory under `_build_debug/stage/app/RV.app/Contents/MacOS/RV`.
```
````

