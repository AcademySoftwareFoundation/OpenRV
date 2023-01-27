# Building Open RV on macOS

## Summary

1. [Install XCode](#install-xcode)
1. [Install Homebrew](#install-homebrew)
1. [Install tools and build dependencies](#install-tools-and-build-dependencies)
1. [Install the python requirements](#install-the-python-requirements)
1. [Install Qt](#install-qt)
1. [Configure CMake](#configure)
1. [Invoke CMake](#build-including-incremental-builds)

## Install XCode

From the App Store, download XCode. Make sure that it's the source of the active developer directory.

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

## Configure

The project uses CMake and requires a configure step before building. It is during the configure step that you provide your Qt package.

From the root of the repository, execute `cmake` specifying the path to an arbitrary build folder and the path to your QT5 package.

For example:

```bash
cmake -B cmake-build -DCMAKE_BUILD_TYPE=Release -DRV_DEPS_QT5_LOCATION=$HOME/Qt/5.15.2/clang_64
```

### Custom generator

You can decide to build with another generator. Ninja is known to work well with the build. If you desire to build with Ninja, you can set`-G Ninja`

## Build (including incremental builds)

Invoke the build tool using cmake (recommended).

For example

```bash
cmake --build cmake-build --target RV
```
