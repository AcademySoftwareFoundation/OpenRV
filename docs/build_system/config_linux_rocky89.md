# Building Open RV on Rocky 8 and 9

The build process is largely the same, with some small exceptions, discussed below as appropriate.

## Summary

OpenRV 2025 can be built for Rocky 8 and Rocky 9, using VFX reference platform CY2023 or CY2024.

Here's a table of what's versions are needed for each:

|                      | Rocky 8<br>CY2023 | Rocky 8<br>CY2024 |Rocky 9<br>CY2023 | Rocky 9<br>CY2024 |
|----------------------|------------|------------|-----------|---------|
| Config manager repo  | powertools | powertools | crb       | crb     |
| Qt                   | 5.12.2     | 5.12.2     | 6.5.3     | 6.5.3   |
| Cmake                | 3.31.6     | 3.31.6     | 3.31.6    | 3.31.6  |
| Python               | 3.10.0     | 3.11.8     | 3.10.0    | 3.11.8  |
| Perl-CPAN            | --         | 2.36+      | --        | 2.36+   |

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

#### 1.1. Set the config manager depending Rocky's version:
```bash
# Rocky 8
sudo dnf config-manager --set-enabled crb devel
```

```bash
# Rocky 9
sudo dnf config-manager --set-enabled powertools devel
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

Download the latest open-source [Qt installer](https://www.qt.io/download-open-source). We do not recomment that you install Qt from other installable sources as it may introduce build isues.

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

Note 1: If you select a different installation path, you will need to manually export the environment variable "QT_HOME" to this path in your ~/.bashrc file.

Note 2: Qt modules for Logs, Android, iOS and WebAssembly are not required to build OpenRV. 


### 5. Building OpenRV for the first time

In general, to maximize your chances of successfully building and contributing to OpenRV, you should:

- Be familiar with git and its usage.
- Create a branch from 'main' before modifying code
- Always update your branch (git pull, git rebase) with the latest from github
- Fix conflicts prior to creating a pull request

#### 5.1 Clone the RV repository using HTTPS or SSH key

Clone the Open RV repository. Typically, this will create an "OpenRV" folder from your current location.

```bash
# If using a password-protected SSH key:
git clone --recursive git@github.com:AcademySoftwareFoundation/OpenRV.git
cd OpenRV
```

```bash
# Or if using the web URL:
git clone --recursive https://github.com/AcademySoftwareFoundation/OpenRV.git
cd OpenRV
```

**Note: If you plan to contribute and submit pull requests for review** you should create a GitHub account and follow the standard GitHub fork model for submitting PRs. This involves creating your own github account, forking the Academy Software Foundation's OpenRV repository, making your changes in a branch of your forked version, and then submitting pull requests from your forked repository (don't forget to sync your fork regularly!). In that case, substitute the above repository URL above with the URL of your own fork.


#### 5.2 Load command aliases to build OpenRV

Command-line aliases are provided to simplify the process of setting up the environment and to build OpenRV.

```bash
cd OpenRV
source rvcmds.sh
```

#### 5.3 - First-time build only: rvbootstrap

This step only needs to be done on a freshly cloned git repository. Under the hood, his command will create an initial setup environment, will fetch source dependencies, install other required elements, and will create a Python virtual environment in the current directory under `.venv`

After the setup stage is done, a build is started and should produce a valid "rv" executable binary.

```bash
# Produces default optimized build in ~/OpenRV/_build
rvbootstrap
```
```bash
# Produces unoptimized debug build in ~/OpenRV/_build_debug
rvbootstrapd
```

Note: launch the default optimized build unless you have a reason to want the unoptimized debug build.


### 6. Building OpenRV after the first time

Once you've built OpenRV for the first time, there is no need to run "rvboostrap" again.

#### 6.1 Build OpenRV (incrementally) using rvmk

To build OpenRV after the first time, "rvmk" will correctly configure the environment variables and launch the incremental build process. 

```bash
# Produces incremental optimized build in ~/OpenRV/_build_debug
rvmk
```
```bash
# Produces incremental unoptimized debug build in ~/OpenRV/_build_debug
rvmkd
```

Note that, under the hood, rvmk/rvmkd is litterally the equivalent to these two commands:
```bash
rvcfg/rvcfgd      # sets environment variables
rvbuild/rvbuildd  # launches the build process
```


#### 6.2 Rebuilding the dependencies

Building the source dependencies is done automatically the first time we build OpenRV with "rvbootstrap/d" so you typically never need to rebuild them. In the rare event you would need to fix a bug or update one such third-party source dependency, dependencies can be rebuild this way:


```bash
# Rebuild dependencies for default optimized build
rvbuildt dependencies
```
```bash
# Rebuild dependencies for debug build
rvbuildtd dependencies
```


### 7. Starting the OpenRV executable

Once OpenRV is finished building, its executable binary can be found here:

```bash
# For the default optimized build:
`~/OpenRV/_build/stage/app/bin/rv`.
```
```bash
# For the debug build
`~/OpenRV/_build_debug/stage/app/bin/rv`.
```


### 8. Building with Docker (Optional)

To build OpenRV using Docker, use the provided Dockerfile found in this repository, which includes all required dependencies. 

#### 8.1. Build the image and run

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

#### 8.2. Create and run the container
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

Once you are into the container, you can follow the [standard process](5.-building-openrv-for-the-first-time).

#### 8.3. Copy the stage folder outside of the container

If you are on a host that is the same as, or compatible with, your version of Rocky Linux, you can copy the stage folder outside of the container and execute Open RV.

```bash
# Container id is the same as the one used in the step above
docker cp <container id>:/home/rv/OpenRV/_build/stage ./openrv_stage
```
