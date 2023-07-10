# Building Open RV on Rocky 9

## Summary

1. [Install Basics](#install-basics)
1. [Install tools and build dependencies](#install-tools-and-build-dependencies)
1. [Install CMake](#install-cmake)
1. [Install Qt5](#install-qt)

## Install Basics

Make sure we have some basic tools available on the workstation:

```bash
sudo dnf install wget git
```

## Install tools and build dependencies

Some of the build dependencies come from outside the main AppStream repo. So first we will enable those and then install our dependencies:

```bash
sudo dnf install epel-release
sudo dnf config-manager --set-enabled crb devel
sudo dnf install alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison bzip2-devel cmake-gui curl-devel flex gcc gcc-c++ libXcomposite libXi-devel libaio-devel libffi-devel nasm ncurses-devel nss libtool libxkbcommon libXcomposite libXdamage libXrandr libXtst libXcursor meson ninja-build openssl-devel pulseaudio-libs pulseaudio-libs-glib2 ocl-icd ocl-icd-devel opencl-headers python3 python3-devel qt5-qtbase-devel readline-devel sqlite-devel tcl-devel tcsh tk-devel yasm zip zlib-devel 
```

You can disable the devel repo afterwards since dnf will warn about it:
```bash
sudo dnf config-manager --set-disabled devel
```

### GLU

You may or may not have libGLU on your system depending on your graphics driver setup.  To know you can check if you have the lib /usr/lib64/libGLU.so.1.  If it's there you can skip this step, there's nothing further to do in this section.  If not, you can install the graphics drivers for your system. As as last resort, you can install a software (mesa) libGLU version, but you may not have ideal performance with this option.  To do so:

```bash
sudo dnf install mesa-libGLU mesa-libGLU-devel
```

### OPENSSL

Warning: RV's OpenSSL breaks this Linux version: Building OpenRV will fail and OpenRV will not start if the build is fixed.

Fixing the build: edit src/build/make_python.py, find the openssl_libs variable for Linux (platform == "Linux") and remove the OPENSSL_OUTPUT_DIR and change it to 'usr' and change 'lib' to 'lib64'.

### Install the python requirements

Some of the RV build scripts requires extra python packages. They can be installed using the requirements.txt at the root of the repository.

```bash
python3 -m pip install -r requirements.txt
```

## Install CMake

You need CMake version 3.24+ to build RV. The dnf-installable version is not quite recent enough, you'll to build and install CMake from sources.

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0.tar.gz
tar -zxvf cmake-3.24.0.tar.gz
cd cmake-3.24.0
./bootstrap --parallel=32  # 32 or whatever your machine allows
make -j 32  # 32 or whatever your machine allows
sudo make install

cmake --version  # confirm the version of your newly installed version of CMake
cmake version 3.24.0
```

## Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.

WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.
