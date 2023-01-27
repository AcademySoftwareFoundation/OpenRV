# Building Open RV on CentOS 7

## Summary

1. [Install Basics](#install-basics)
1. [Install devtoolset-9](#install-devtoolset-9)
1. [Install tools and build dependencies](#install-tools-and-build-dependencies)
1. [Install CMake](#install-cmake)
1. [Install nasm](#install-nasm)
1. [Install Qt5](#install-qt)
1. [Configure CMake](#configure)
1. [Build Open RV](#build-including-incremental-builds)

## Install Basics

Make sure we have some basic tools available on the workstation:

```bash
sudo yum install sudo wget git
```

## Install devtoolset-9

By default the CentOS 7.9 built-in tools won't match the requirements we have to build RV. Install devtoolset-9 to remedy the situation.

```bash
sudo yum install centos-release-scl
sudo yum-config-manager --enable rhel-server-rhscl-7-rpms
sudo yum install devtoolset-9

# Note that you have to activate this all of the time (or add it to shell user initialization (e.g.: .bashrc))
scl enable devtoolset-9 $SHELL
```

## Install tools and build dependencies

Most of the build requirements can be installed using the following command:

```bash
sudo yum install alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison bzip2-devel cmake-gui curl-devel flex glew-devel libXcomposite libXi-devel libaio-devel libffi-devel libncurses-devel libtool libxkbcommon openssl-devel pulseaudio-libs pulseaudio-libs-glib2 ocl-icd opencl-headers python3 python3-devel qt5-qtbase-devel readline-devel sqlite-devel tcl-devel tk-devel yasm zlib-devel 
```

### Install the python requirements

Some of the RV build scripts requires extra python packages. They can be installed using the requirements.txt at the root of the repository.

```bash
python3 -m pip install -r requirements.txt 
```

## Install CMake

You need CMake version 3.24+ to build RV. The yum-installable version is not quite recent enough, you'll to build and install CMake from sources.

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0.tar.gz
tar -zxvf cmake-3.24.0.tar.gz
cd cmake-3.24.0
./bootstrap --parallel=32  # 32 or whatever your machine allows
make -j 32  # 32 or whatever your machine allows
sudo make install

cmake --version  # confirm the version of you're newly installed version of CMake
cmake version 3.24.0
```

## Install nasm

The `nasm` tool is required to build the `libdav1d` and `ffmpeg` dependencies.
The yum-provided version is slightly too old and you'll required building from sources.
Fortunately, building `nasm` from source is as easy as it gets:

```bash
wget https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.gz
tar xf nasm-2.15.05.tar.gz
cd nasm-2.15.05
scl enable devtoolset-9 "$SHELL -c './configure'" 
scl enable devtoolset-9 "$SHELL -c 'make -j'" 
sudo make install
```

## Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.

## Configure

The project uses CMake and requires a configure step before building. It is during the configure step that you provide your Qt package.

From the root of the repository, execute `cmake` specifying the path to an arbitrary build folder and the path to your QT5 package.

For example:

```bash
cmake -B cmake-build -DRV_DEPS_QT5_LOCATION=/Path/To/Your/Qt5/Rootr
```

### Custom generator

You can decide to build with another generator. Ninja is known to work well with the build. If you desire to build with Ninja, you can set`-G Ninja`

## Build (including incremental builds)

Invoke the build tool using cmake (recommended).

For example:

```bash
cmake --build cmake-build --target rv
```
