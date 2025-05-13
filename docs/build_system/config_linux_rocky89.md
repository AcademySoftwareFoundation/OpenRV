# Building Open RV on Rocky 8 and 9

In general, setting up for Rocky 8 or 9 is quite similar, with only minor differences.

## Summary

OpenRV 2025 can be built for Rocky 8 and Rocky 9, using VFX reference platform CY2023 or CY2024.

Here's a table of what versions are needed for each:

|                      | Rocky 8<br>CY2023 | Rocky 8<br>CY2024 |Rocky 9<br>CY2023 | Rocky 9<br>CY2024 |
|----------------------|------------|------------|-----------|---------|
| Config manager repo  | powertools | powertools | crb       | crb     |
| Qt                   | 5.12.2     | 6.5.3      | 5.12.2    | 6.5.3   |
| Cmake                | 3.31.6     | 3.31.6     | 3.31.6    | 3.31.6  |
| Python               | 3.10.0     | 3.11.8     | 3.10.0    | 3.11.8  |
| Perl-CPAN            | --         | --         | 2.36+     | 2.36+   |

All other dependencies are shared across variations.

1. [Install tools and build dependencies](#1.-install-tools-and-build-dependencies)
2. [Install Pyenv / Python](#2.-Install-pyenv-and-python)
3. [Install CMake](#3.-install-cmake)
4. [Install Qt](#4.-install-qt)
5. [Building OpenRV for the first time](#5.-building-openrv-for-the-first-time)
6. [Building OpenRV after the first time](#6.-building-openrv-after-the-first-time)
7. [Starting the OpenRV executable](#7.starting-the-openrv-executable)
8. [Building with Docker (Optional)](8.building-with-docker-(optional))

### 1. Install tools and build dependencies

#### 1.1. Set the config manager
```bash
# Rocky 8
sudo dnf config-manager --set-enabled powertools devel
```

```bash
# Rocky 9
sudo dnf config-manager --set-enabled crb devel
dnf install -y perl-CPAN
cpan FindBin
```

#### 1.2. Install other dependencies
```bash
sudo dnf install wget git epel-release
```
```bash
sudo dnf groupinstall "Development Tools" -y
```
```bash
sudo dnf install -y alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison bzip2-devel cmake-gui curl-devel flex gcc gcc-c++ git libXcomposite libXi-devel libaio-devel libffi-devel nasm ncurses-devel nss libtool libxkbcommon libXcomposite libXdamage libXrandr libXtst libXcursor mesa-libOSMesa mesa-libOSMesa-devel meson openssl-devel patch pulseaudio-libs pulseaudio-libs-glib2 ocl-icd ocl-icd-devel opencl-headers qt5-qtbase-devel readline-devel sqlite-devel systemd-devel tcl-devel tcsh tk-devel yasm zip zlib-devel wget patchelf pcsc-lite libxkbfile perl-IPC-Cmd
```
```bash
sudo dnf install -y libX11-devel libXext-devel libXrender-devel libXrandr-devel libXcursor-devel libXi-devel libXxf86vm-devel libxkbcommon-devel
```
```bash
sudo dnf install -y xz-devel mesa-libGLU mesa-libGLU-devel
```
```bash
sudo dnf clean all
```

#### 1.4. Disable the devel repo afterwards since dnf will warn about it:
```bash
sudo dnf config-manager --set-disabled devel
```

### 2. Install Pyenv and Python

Install pyenv to be able to install the specific version of python associated the VFX reference platform

#### 2.1 Install Pyenv
```bash
curl https://pyenv.run | bash
export PYENV_ROOT="$HOME/.pyenv"
```

#### 2.2 Install the Python version associated with the VFX reference platform
```bash
# For VFX platform CY2023
pyenv install 3.10
pyenv global 3.10
```

```bash
# For VFX platform CY2024
pyenv install 3.11.8
pyenv global 3.11.8
```

#### 2.3 Install the required additional python packages. 

```bash
# install requirements.txt from the root of the git repository
python3 -m pip install -r requirements.txt
```

### 3. Install CMake

Since the dnf-installable version is not quite recent enough, you'll need to build and install CMake from source.

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6.tar.gz
tar -zxvf cmake-3.31.6.tar.gz
cd cmake-3.31.6
./bootstrap --parallel=32  # 32 or whatever your machine allows
make -j 32  # 32 or whatever your machine allows
sudo make install

cmake --version  # confirm the version of your newly installed version of CMake
cmake version 3.31.6
```

### 4. Install Qt

Download the latest open-source [Qt installer](https://www.qt.io/download-open-source). We do not recommend that you install Qt from other installable sources as it may introduce build issues.

```bash
- Start the Qt installer
- Installation path: ~/Qt
- Click "Archive" box on the right side of the window
- Click "Filter" to see installable options
```


```bash
# For VFX platform CY2023
Select Qt 5.12.2
# Any 5.12.2+ should work, but Autodesk's RV is build against 5.12.2)
```

```bash
# For VFX platform CY2024
Select Qt 6.5.3
# Any 6.5.3+ should work, but Autodesk's RV is build against 6.5.3)
```

Note 1: If you install Qt at a different installation path, you will need to manually export the environment variable "QT_HOME" to that path in your ~/.bashrc file such that the build scripts will be able to find it.

Note 2: Qt modules for Logs, Android, iOS and WebAssembly are not required to build OpenRV. 

### 5. Building with Docker (Optional)

To build OpenRV using Docker, use the provided Dockerfile found in this repository, which includes all required dependencies. 


#### 5.1. Build the image and run

```bash
# For Rocky 8
cd dockerfiles
docker build -t openrv-rocky8 -f Dockerfile.Linux-Rocky8 .
docker run -d openrv-rocky8 /bin/bash -c "sleep infinity"
```

```bash
# For Rocky 9
cd dockerfiles
docker build -t openrv-rocky9 -f Dockerfile.Linux-Rocky9 .
docker run -d openrv-rocky9 /bin/bash -c "sleep infinity"
```

#### 5.2. Create and run the container
```bash
# Lookup the container id for openrv-rocky{8/9}
docker container ls

# It will produce an output that looks like this:
# CONTAINER ID   IMAGE               COMMAND                  {and more}
# 1f6a1104a1f4   openrv-rocky{8/9}   "/bin/bash -c 'sleep…"   {and more}
```
```bash
# Use the "CONTAINER ID" value (1f6a1104a1f4 in this example) in the following command:
docker container exec -it <id> /bin/bash  # replace 'id' with your value
```

#### 5.3. Build OpenRV in the container

Once you are into the container, you can follow the [common build process](config_common_build.md) to build OpenRV.

#### 5.4. Copy the stage folder outside of the container

If you are on a host that is the same as, or compatible with, your version of Rocky Linux, you can copy the stage folder outside of the container and execute Open RV.

```bash
# Container id is the same as the one used in the step above
docker cp <container id>:/home/rv/OpenRV/_build/stage ./openrv_stage
```
