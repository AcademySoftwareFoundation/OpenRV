# Preparing Open RV on Rocky 8 and 9

Open RV 2025 can be built for Rocky 8 and Rocky 9, using the VFX reference platform CY2023 or CY2024, with only minor differences for the config manager repo and the requirement for Perl-CPAN.

Select your VFX reference platform by clicking on the appropriate tab. Install instructions follows.

````{tabs}
```{code-tab} bash VFX-CY2023
Qt                  : 5.12.2
Python              : 3.10
Cmake               : 3.31.6

Config manager repo : powertools  (Rocky 8)
Config manager repo : crb         (Rocky 9)
Perl-CPAN           : 2.36+       (Rocky 9)

```
```{code-tab} bash VFX-CY2024
Qt                  : 6.5.3
Python              : 3.11.8
Cmake               : 3.31.6

Config manager repo : powertools  (Rocky 8)
Config manager repo : crb         (Rocky 9)
Perl-CPAN           : 2.36+       (Rocky 9)

```
````



All other dependencies are shared across variations.

1. [Install tools and build dependencies](rocky_install_tools_and_dependencies)
2. [Install Pyenv / Python](rocky_install_pyenv_and_python)
3. [Install CMake](rocky_install_cmake)
4. [Install Qt](rocky_install_qt)
5. [Building with Docker (Optional)](rocky_building_with_docker)


(rocky_install_tools_and_dependencies)=
### 1. Install tools and build dependencies

#### 1.1. Set the config manager

````{tabs}
```{code-tab} bash Rocky 8
sudo dnf config-manager --set-enabled powertools devel
```
```{code-tab} bash Rocky 9
sudo dnf config-manager --set-enabled crb devel
dnf install -y perl-CPAN
cpan FindBin
```
````

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

(rocky_install_pyenv_and_python)=
### 2. Install Pyenv and Python

Install pyenv to be able to install the specific version of python associated the VFX reference platform

#### 2.1 Install Pyenv

Install pyenv, and add its installation path to your ~/.bashrc and apply the changes to your current terminal window:

```bash
curl https://pyenv.run | bash
echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
source ~/.bashrc
```

#### 2.2 Install the Python version associated with the VFX reference platform
````{tabs}
```{code-tab} bash VFX-CY2023
pyenv install 3.10
pyenv global 3.10
```
```{code-tab} bash VFX-CY2024
pyenv install 3.11.8
pyenv global 3.11.8
```
````


(rocky_install_cmake)=
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

(rocky_install_qt)=
### 4. Install Qt

Download the latest open-source [Qt installer](https://www.qt.io/download-open-source). We do not recommend that you install Qt from other installable sources as it may introduce build issues.

````{tabs}
```{code-tab} bash VFX-CY2023
- Start the Qt installer
- Installation path: ~/Qt
- Click "Archive" in the hard-to-find dropbown box in the top right side of the window
- Select Qt 5.12.2
# Any 5.12.2+ should work, but Autodesk's RV is build against 5.12.2
```
```{code-tab} bash VFX-CY2024
- Start the Qt installer
- Installation path: ~/Qt
- Click "Archive" in the hard-to-find dropbown box in the top right side of the window
- Select Qt 6.5.3
# Any 6.5.3+ should work, but Autodesk's RV is build against 6.5.3
```
````



Note 1: If you install Qt at a different installation path, you will need to manually export the environment variable "QT_HOME" to that path in your ~/.bashrc file such that the build scripts will be able to find it.

Note 2: Qt modules for Logs, Android, iOS and WebAssembly are not required to build Open RV. 

(rocky_building_with_docker)=
### 5. Building with Docker (Optional)

To build Open RV using Docker, use the provided Dockerfile found in this repository, which should already contain all required dependencies. 

Please go through the cloning procedure found in the [common build process](config_common_build.md). Once cloned, get back here to build the docker image, run the container, and build Open RV within the docker container.


#### 5.1. Build the image and run

````{tabs}
```{code-tab} bash Rocky 8
cd dockerfiles
docker build -t openrv-rocky8 -f Dockerfile.Linux-Rocky8 .
docker run -d openrv-rocky8 /bin/bash -c "sleep infinity"
```

```{code-tab} bash Rocky 9
cd dockerfiles
docker build -t openrv-rocky9 -f Dockerfile.Linux-Rocky9 .
docker run -d openrv-rocky9 /bin/bash -c "sleep infinity"
```
````



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

#### 5.3. Build Open RV in the container

Once you are into the container, you can follow the [common build process](config_common_build.md) to build Open RV.

#### 5.4. Copy the stage folder outside of the container

If you are on a host that is the same as, or compatible with, your version of Rocky Linux, you can copy the stage folder outside of the container and execute Open RV.

```bash
# Container id is the same as the one used in the step above
docker cp <container id>:/home/rv/OpenRV/_build/stage ./openrv_stage
```
