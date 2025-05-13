# Building Open RV on macOS

(summary)=
## Summary

1. [Select macOS build architecture](select_target_architecture)
1. [Allow Terminal to update or delete other applications](allow_terminal)
2. [Install Xcode](install_xcode)
3. [Install Homebrew](install_homebrew)
4. [Install pyenv and Python](install_python)
5. [Install tools and build dependencies](install_tools_and_build_dependencies)
6. [Install Qt](install_qt)
7. [Build Open RV](build_openrv)



(select_target_architecture)=
### 1. Select macOS build target architecture

#### 1.1 If building for Apple ARM Silicon (Mac M-series CPUs)

Unfortunately, building for Apple Silicon requires a commercial Qt account, simply because since the ARM macOS binaries for Qt are not included in the open-source Qt online installer (but they are included in the commercial Qt online installer)

If you don't want to use a commercial Qt license, or if you're actually targetting macOS on Intel processors, you will need to setup an x86_84 terminal environment. 

#### 1.2 If building for x86_64 Intel CPUs

OpenRV can be built for *x86_64* by changing the architecture of the terminal to *x86_64* using the following command:
```bash
arch -x86_64 $SHELL
```

For all subsequent steps, it will be important to use that *x86_64* terminal window. 


(allow_terminal)=
### 2. Allow Terminal to update or delete other applications

From macOS System Settings > Privacy & Security > App Management, allow Terminal to update or delete other applications.

(install_xcode)=
### 3. Install Xcode

From the App Store, download Xcode. Make sure that it is the source of the active developer directory.

`xcode-select -p` should return `/Applications/Xcode.app/Contents/Developer`. If that is not the case, run `sudo xcode-select -s /Applications/Xcode.app`

(install_homebrew)=
### 4. Install Homebrew

Homebrew is the one-stop shop providing all the build requirements. You can install it by following the instructions on the [Homebrew page](https://brew.sh).

Make sure Homebrew's binary directory is in your PATH and that `brew` can be resolved from your terminal.

(install_tools_and_build_dependencies)=
### 5. Install tools and build dependencies

Most of the build requirements can be installed by running the following brew install command:

```bash
brew install cmake ninja readline sqlite3 xz zlib tcl-tk@8 autoconf automake libtool python yasm clang-format black meson nasm pkg-config glew
```

Make sure `python` resolves in your terminal. In some cases, depending on how the python formula is built, there is no `python` symbolic link.
In that case, you can create one with this command `ln -s python3 $(dirname $(which python3))/python`.


### 6. Install Pyenv and Python

Install pyenv to be able to install the specific version of python associated the VFX reference platform

#### 6.1 Install Pyenv
```bash
curl https://pyenv.run | bash
export PYENV_ROOT="$HOME/.pyenv"
```

#### 6.2 Install the Python version associated with the VFX reference platform
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


(install_qt)=
### 7. Install Qt

Download the latest open-source [Qt installer](https://www.qt.io/download-open-source). We do not recommend that you install Qt from other installable sources as it may introduce build issues. In particular, the Qt6 from homebrew is known to not work well with OpenRV.

Remember, if you're building for macOS on Apple ARM Silicon, you need to log into the Qt installer with a commercial Qt account, since the required ARM versions of Qt are not present in the open-source Qt installer.

```bash
- Start the Qt installer
- Installation path: ~/Qt
- Click "Archive" in the hard-to-find dropbown box in the top right side of the window
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

Note 1: If you install Qt at a different installation path, you will need to manually export the environment variable "QT_HOME" to that path in your ~/.zshrc file such that the build scripts will be able to find it.

Note 2: Qt modules for Logs, Android, iOS and WebAssembly are not required to build OpenRV. 


(build_openrv)=
### 8. Build Open RV

*On macOS, in the unlikely event you want to build for a different deployment target than your current operating system, make sure to define the MACOSX_DEPLOYMENT_TARGET environment variable in your ~/.zshrc file before starting any build process.


Finally, simply go through the [common build process](config_common_build.md) to build OpenRV.

