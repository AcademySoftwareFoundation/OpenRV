# Building Open RV on macOS

## Summary

- [Building Open RV on macOS](#building-open-rv-on-macos)
  - [Summary](#summary)
  - [Install XCode](#install-xcode)
  - [Install Homebrew](#install-homebrew)
  - [Install tools and build dependencies](#install-tools-and-build-dependencies)
  - [Install the python requirements](#install-the-python-requirements)
  - [Install Qt](#install-qt)

## Install XCode

From the App Store, download XCode 14.3.1. Make sure that it's the source of the active developer directory.
Note that using an XCode version more recent than 14.3.1 will result in an FFmpeg build break.

`xcode-select -p` should return `/Applications/Xcode.app/Contents/Developer`. If it's not the case, run `sudo xcode-select -s /Applications/Xcode.app`

Note that XCode 15 is not compatible with Boost 1.80. If XCode 15 is installed, RV will automatically default to using Boost 1.81 instead. 
Install XCode 14.3.1 if you absolutely want to use Boost version 1.80 as per VFX reference platform CY2023.

Please reference [this workaround](https://forums.developer.apple.com/forums/thread/734709) to use XCode 14.3.1 on Sonoma, as it is no longer 
compatible by default.

## Install Homebrew

Homebrew is the one stop shop providing all the build requirements. You can install it following the instructions on the [Homebrew page](https://brew.sh).

Make sure Homebrew's binary directory is in your PATH and that `brew` is resolved from your terminal.

## Install tools and build dependencies

Most of the build requirements can be installed by running the following brew install command:

```bash
brew install cmake ninja readline sqlite3 xz zlib tcl-tk autoconf automake libtool python yasm clang-format black meson nasm pkg-config glew
```

Make sure `python` resolves in your terminal. In some case, depending on how the python formula is built, there's no `python` symbolic link.
In that case, you can create one with this command `ln -s python3 $(dirname $(which python3))/python`.

## Install the python requirements

See the **Using a Python Virtual Environment** in `README.md` in the root directory before running any of the commands in this section. Python packages should be installed inside of a virtual environment. 

Some of the RV build scripts requires extra python packages. They can be installed using the requirements.txt at the root of the repository.

```bash
python3 -m pip install -r requirements.txt 
```

## Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.


WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.

Note: Qt5 from homebrew is known to not work well with OpenRV.

## Allow Terminal to update or delete other applications

From the macOS System Settings/Privacy & Security/App Management, allow Terminal to update or delete other applications.

