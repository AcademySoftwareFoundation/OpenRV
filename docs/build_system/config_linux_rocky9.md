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
sudo dnf install alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison bzip2-devel cmake-gui curl-devel flex gcc gcc-c++ libXcomposite libXi-devel libaio-devel libffi-devel nasm ncurses-devel nss libtool libxkbcommon libXcomposite libXdamage libXrandr libXtst libXcursor mesa-libOSMesa mesa-libOSMesa-devel meson ninja-build openssl-devel patch perl-FindBin pulseaudio-libs pulseaudio-libs-glib2 ocl-icd ocl-icd-devel opencl-headers python3 python3-devel qt5-qtbase-devel readline-devel sqlite-devel tcl-devel tcsh tk-devel yasm zip zlib-devel
```

You can disable the devel repo afterwards since dnf will warn about it:
```bash
sudo dnf config-manager --set-disabled devel
```

### GLU

You may or may not have libGLU on your system depending on your graphics driver setup. 

Check for the existance of the following files:

* `/usr/lib64/libGLU.so.1`
* `/usr/lib64/libGLU.so`
* `/usr/include/GL/glu.h`

If `libGLU.so.1` is present but `libGLU.so` is not, create a symlink between them with `sudo ln -s /usr/lib64/libGLU.so.1 /usr/lib64/libGLU.so`. If `/usr/include/GL/glu.h` is missing, install `mesa-libGLU-devel` to get this required header file. If libGLU is missing entirely, you can install the graphics drivers for your system. As as last resort, you can install a software (mesa) libGLU version, but you may not have ideal performance with this option:

```bash
sudo dnf install mesa-libGLU mesa-libGLU-devel
```

### Install the python requirements

Some of the RV build scripts requires extra python packages. They can be installed using the requirements.txt at the root of the repository.

```bash
python3 -m pip install -r requirements.txt
```

## Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). During Qt Setup's Select Components phase, check the "Archive" box on the right side of the window then click on "Filter" to see Qt 5.15.x options. Logs, Android, iOS and WebAssembly are not required to build OpenRV. Make sure to note the destination of the Qt install, as you will have to set the `QT_HOME` environment variable to this location's build dir.

WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.
