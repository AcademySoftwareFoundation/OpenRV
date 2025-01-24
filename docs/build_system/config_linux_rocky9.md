# Building Open RV on Rocky 9

## Summary

1. [Install Basics](#install-basics)
2. [Install tools and build dependencies](#install-tools-and-build-dependencies)
3. [Install CMake](#install-cmake)
4. [Install Qt5](#install-qt)
5. [Build Open RV](build_rocky9_openrv)
    1. [Building from command line](building_rocky9_from_command_line)
    2. [Building with Docker](building_rocky9_with_docker)

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
sudo dnf install alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel bison bzip2-devel cmake-gui curl-devel flex gcc gcc-c++ libXcomposite libXi-devel libaio-devel libffi-devel nasm ncurses-devel nss libtool libxkbcommon libXcomposite libXdamage libXrandr libXtst libXcursor mesa-libOSMesa mesa-libOSMesa-devel meson ninja-build openssl-devel patch perl-FindBin pulseaudio-libs pulseaudio-libs-glib2 ocl-icd ocl-icd-devel opencl-headers python3 python3-devel qt5-qtbase-devel readline-devel sqlite-devel systemd-devel tcl-devel tcsh tk-devel yasm zip zlib-devel 
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

## Install CMake

You need CMake version 3.27+ to build RV. The dnf-installable version is not quite recent enough, you'll need to build and install CMake from source.

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3.tar.gz
tar -zxvf cmake-3.30.3.tar.gz
cd cmake-3.30.3
./bootstrap --parallel=32  # 32 or whatever your machine allows
make -j 32  # 32 or whatever your machine allows
sudo make install

cmake --version  # confirm the version of your newly installed version of CMake
cmake version3.30.3
```

## Install Qt

Download the last version of Qt 5.15.x that you can get using the online installer on the [Qt page](https://www.qt.io/download-open-source). During Qt Setup's Select Components phase, check the "Archive" box on the right side of the window then click on "Filter" to see Qt 5.15.x options. Logs, Android, iOS and WebAssembly are not required to build OpenRV. Make sure to note the destination of the Qt install, as you will have to set the `QT_HOME` environment variable to this location's build dir.

WARNING: If you fetch Qt from another source, make sure to build it with SSL support, that it contains everything required to build PySide2, and that the file structure is similar to the official package.


(build_rocky9_openrv)=
## Build Open RV

(building_rocky9_from_command_line)=
### Building from command line

(build_rocky9_openrv1)=
#### Before executing any commands

To maximize your chances of successfully building Open RV, you must:
- Fully update your code base to the latest version (or the version you want to use) with a command like `git pull`.
- Fix all conflicts due to updating the code.
- Revisit all modified files to ensure they aren't using old code that changed during the update such as when the Visual Studio version changes.

(build_rocky9_openrv2)=
#### Get Open RV source code

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

(build_rocky9_openrv3)=
#### Load aliases for Open RV

From the Open RV directory:
```shell
source rvcmds.sh
```

(build_rocky9_openrv4)=
#### Install Python dependencies

````{note}
This section needs to be done only one time when a fresh Open RV repository is cloned. 
The first time the `rvsetup` is executed, it will create a Python virtual environment in the current directory under `.venv`.
````

From the Open RV directory, the following command will download and install the Python dependencies.
```shell
rvsetup
```

(build_rocky9_openrv5)=
#### Configure the project

From the Open RV directory, the following command will configure CMake for the build:

````{tabs}
```{code-tab} bash Release
rvcfg
```
```{code-tab} bash Debug
rvcfgd
```
````

(build_rocky9_openrv6)=
#### Build the dependencies

From the Open RV directory, the following command will build the dependencies:

````{tabs}
```{code-tab} bash Release
rvbuildt dependencies
```
```{code-tab} bash Debug
rvbuildtd dependencies
```
````

(build_rocky9_openrv7)=
#### Build the main executable

From the Open RV directory, the following command will build the main executable:

````{tabs}
```{code-tab} bash Release
rvbuildt main_executable
```
```{code-tab} bash Debug
rvbuildtd main_executable
```
````

(build_rocky9_openrv8)=
#### Opening Open RV executable

````{tabs}
```{tab} Release
Once the build is completed, the Open RV application can be found in the Open RV directory under `_build/stage/app/bin/rv`.
```
```{tab} Debug
Once the build is completed, the Open RV application can be found in the Open RV directory under `_build_debug/stage/app/bin/rv``.
```
````

(building_rocky9_with_docker)=
### Building with Docker (Optional)

To build Open RV using Docker, utilize the provided Dockerfile, which includes all required dependencies. 

(build_rocky9_image)=
#### Build the image
```bash
cd dockerfiles
docker build -t openrv-rocky9 -f Dockerfile.Linux-Rocky9 .
```

(run_rocky9_image)=
#### Create the container
```bash
docker run -d openrv-rocky9 /bin/bash -c "sleep infinity"
```

(go_into_the_rocky9_container)=
#### Go into the container
```bash
# Lookup the container id for openrv-rocky9
docker container ls

# Use the container id to go into it.
# e.g.
# CONTAINER ID   IMAGE           COMMAND                  CREATED          STATUS          PORTS     NAMES
# 1f6a1104a1f4   openrv-rocky9   "/bin/bash -c 'sleep…"   25 minutes ago   Up 25 minutes             busy_sanderson
# In this example, the <id> would be 1f6a1104a1f4.
docker container exec -it <id> /bin/bash
```

Once you are into the container, you can follow the [standard process](build_rocky9_openrv2).

#### Copy the stage folder outside of the container

If you are on a host that is compatible with Rocky Linux 9, you can copy the stage folder outside of the container and 
execute Open RV.

Container id is the same as the one used in the step [Go into the container](go_into_the_rocky9_container).

```bash
docker cp <container id>:/home/rv/OpenRV/_build/stage ./openrv_stage
```
