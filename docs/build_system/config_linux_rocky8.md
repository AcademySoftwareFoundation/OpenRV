# Building OpenRV on Rocky Linux 8.7

## Manual Build

1. [Install Dependencies](#install-dependencies)
2. [Install Qt5](#install-qt5)
3. [Build and Install OpenRV](#build-and-install-openrv)

### Install Dependencies

Make sure we have some basic tools available on the workstation:

```shell
sudo dnf install wget git cmake
```

Some of the build dependencies come from outside the main AppStream repo. So first we will enable those and then install our dependencies:

```shell
sudo dnf install epel-release
sudo dnf config-manager --set-enabled powertools
sudo dnf install alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison bzip2-devel cmake-gui curl-devel flex gcc gcc-c++ libXcomposite libXi-devel libaio-devel libffi-devel nasm ncurses-devel nss libtool libxkbcommon libXcomposite libXdamage libXrandr libXtst libXcursor mesa-libOSMesa mesa-libOSMesa-devel meson ninja-build openssl-devel patch pulseaudio-libs pulseaudio-libs-glib2 ocl-icd ocl-icd-devel opencl-headers python3 python3-devel qt5-qtbase-devel readline-devel sqlite-devel systemd-devel tcl-devel tcsh tk-devel yasm zip zlib-devel
```

You may or may not have libGLU on your system depending on your graphics driver setup.  To know you can check if you have the lib /usr/lib64/libGLU.so.1.  If it's there you can skip this step, there's nothing further to do in this section.  If not, you can install the graphics drivers for your system. As as last resort, you can install a software (mesa) libGLU version, but you may not have ideal performance with this option.  To do so:

```shell
sudo dnf install mesa-libGLU mesa-libGLU-devel
```

Ensure that your `python3` packages are installed and up to date:
```shell
dnf install -y python3-setuptools
python3 -m pip install --upgrade pip setuptools wheel
```

Ensure that you have a symlink to `python3` at `/usr/bin/python`.
```shell
ln -sf /usr/bin/python3 /usr/bin/python
```

### Install Qt5

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). Logs, Android, iOS and WebAssembly are not required to build OpenRV.

WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.

```shell
./qt-unified-linux-x64-4.6.1-online.run --email "<email>" --password "<password>" --platform minimal --confirm-command --accept-licenses --accept-obligations --accept-messages install qt.qt5.5152.qtpdf qt.qt5.5152.qtpurchasing qt.qt5.5152.qtvirtualkeyboard  qt.qt5.5152.qtquicktimeline qt.qt5.5152.qtlottie qt.qt5.5152.debug_info  qt.qt5.5152.qtscript qt.qt5.5152.qtcharts qt.qt5.5152.qtwebengine  qt.qt5.5152.qtwebglplugin qt.qt5.5152.qtnetworkauth  qt.qt5.5152.qtwaylandcompositor qt.qt5.5152.qtdatavis3d qt.qt5.5152.logs  qt.qt5.5152 qt.qt5.5152.src qt.qt5.5152.gcc_64 qt.qt5.5152.qtquick3d
```

### Build and Install OpenRV

Get the repository:
```shell
git clone --recursive https://github.com/AcademySoftwareFoundation/OpenRV.git
```

Some of the RV build scripts requires extra python packages.
They can be installed using the requirements.txt at the root of the repository.
```shell
python3 -m pip install -r requirements.txt
```

Set the QT_HOME to the install location, build and install:
```shell
export QT_HOME=/opt/Qt/5.15.2/gcc_64
source rvcmds.sh
rvbootstrap
rvinst
```

## Docker Build

OpenRV can also be built inside a Docker Container.

1. [Build the Image](#build-the-image)
2. [Extract the Installation Files](#extract-the-installation-files)
3. [Cleanup](#cleanup)

### Build the Image

Here's a `Dockerfile` that installs all dependencies, builds and installs OpenRV from source.
> Note: You will need an email and password for Qt and you might have to update the url to the Qt5 online installer.

```dockerfile
FROM rockylinux:8.7

# Install dependencies
RUN dnf install -y epel-release
RUN dnf config-manager --set-enabled powertools
RUN dnf install -y alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison \
    bzip2-devel cmake-gui curl-devel flex gcc gcc-c++ libXcomposite libXi-devel \
    libaio-devel libffi-devel nasm ncurses-devel nss libtool libxkbcommon libXcomposite \
    libXdamage libXrandr libXtst libXcursor mesa-libOSMesa mesa-libOSMesa-devel meson \
    ninja-build openssl-devel patch pulseaudio-libs pulseaudio-libs-glib2 ocl-icd \
    ocl-icd-devel opencl-headers qt5-qtbase-devel readline-devel \
    sqlite-devel tcl-devel tcsh tk-devel yasm zip zlib-devel
RUN dnf install -y mesa-libGLU mesa-libGLU-devel
RUN dnf install -y cmake git wget

# Note: /usr/bin/python needs to exist for OpenRV to build correctly.
RUN dnf install -y python3-setuptools && \
    python3 -m pip install --upgrade pip setuptools wheel && \
    ln -sf /usr/bin/python3 /usr/bin/python

# Install Qt5
ARG QT_EMAIL
ARG QT_PASSWORD
RUN wget https://d13lb3tujbc8s0.cloudfront.net/onlineinstallers/qt-unified-linux-x64-4.6.1-online.run && \
    chmod a+x qt-unified-linux-x64-4.6.1-online.run && \
    ./qt-unified-linux-x64-4.6.1-online.run \
    --email "$QT_EMAIL" --password "$QT_PASSWORD" --platform minimal \
    --confirm-command --accept-licenses --accept-obligations --accept-messages \
    install qt.qt5.5152.qtpdf qt.qt5.5152.qtpurchasing qt.qt5.5152.qtvirtualkeyboard  \
    qt.qt5.5152.qtquicktimeline qt.qt5.5152.qtlottie qt.qt5.5152.debug_info  \
    qt.qt5.5152.qtscript qt.qt5.5152.qtcharts qt.qt5.5152.qtwebengine  \
    qt.qt5.5152.qtwebglplugin qt.qt5.5152.qtnetworkauth  \
    qt.qt5.5152.qtwaylandcompositor qt.qt5.5152.qtdatavis3d qt.qt5.5152.logs  \
    qt.qt5.5152 qt.qt5.5152.src qt.qt5.5152.gcc_64 qt.qt5.5152.qtquick3d

# Get OpenRV Git Repo
RUN git clone --recursive https://github.com/AcademySoftwareFoundation/OpenRV.git
RUN python3 -m pip install -r /OpenRV/requirements.txt

# Build and Install OpenRV
WORKDIR /OpenRV
# Note: Aliases only work in interactive shells. Use bash script with expand_aliases to
#       use commands set in rvcmds.sh
RUN echo "#!/bin/bash" > install.sh && \
    echo "set -e" >> install.sh && \
    echo "shopt -s expand_aliases" >> install.sh && \
    echo "export QT_HOME=/opt/Qt/5.15.2/gcc_64" >> install.sh && \
    echo "source /OpenRV/rvcmds.sh" >> install.sh && \
    echo "rvbootstrap" >> install.sh && \
    echo "rvinst" >> install.sh
RUN bash ./install.sh
```

### Extract the Installation Files

Next mount the image and copy the `_install` directory from it:
```shell
docker cp $(docker create --name temp_container openrv):/OpenRV/_install install && docker rm temp_container
```

### Cleanup

To clean up, simply remove the image:
```bash
docker image rm openrv
```
