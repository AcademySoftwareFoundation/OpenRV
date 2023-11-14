# Building Open RV on macOS

## Summary

1. [Install XCode](#install-xcode)
1. [Install Homebrew](#install-homebrew)
1. [Install tools and build dependencies](#install-tools-and-build-dependencies)
1. [Install the python requirements](#install-the-python-requirements)
1. [Install Qt](#install-qt)

## Install XCode

From the App Store, download XCode 14.3.1. Make sure that it's the source of the active developer directory.
Note that using an XCode version more recent than 14.3.1 will result in an FFmpeg build break.

`xcode-select -p` should return `/Applications/Xcode.app/Contents/Developer`. If it's not the case, run `sudo xcode-select -s /Applications/Xcode.app`

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

Some of the RV build scripts requires extra python packages. They can be installed using the requirements.txt at the root of the repository.

```bash
python3 -m pip install -r requirements.txt 
```

## Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.


WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.

Note: Qt5 from homebrew is known to not work well with OpenRV.