# Building Open RV on macOS

(summary)=
## Summary

- [Summary](summary)
- [Allow Terminal to update or delete other applications](allow_terminal)
- [Install Xcode](install_xcode)
- [Install CMake](install_cmake)
- [Install Homebrew](install_homebrew)
- [Install tools and build dependencies](install_tools_and_build_dependencies)
- [Install Qt](install_qt)
- [Build Open RV](build_openrv)
- [Setting up debugging in VSCode](debugging_openrv)

````{note}
OpenRV can be built for *x86_64* by changing the architecture of the terminal to *x86_64* using the following command:
```bash
arch -x86_64 $SHELL
```
**It is important to use that *x86_64* terminal for all the subsequent steps.**
````

(allow_terminal)=
## Allow Terminal to update or delete other applications

From macOS System Settings > Privacy & Security > App Management, allow Terminal to update or delete other applications.

(install_xcode)=
## Install Xcode

From the App Store, download Xcode. Make sure that it is the source of the active developer directory.

`xcode-select -p` should return `/Applications/Xcode.app/Contents/Developer`. If that is not the case, run `sudo xcode-select -s /Applications/Xcode.app`

(install_cmake)=
## Install CMake

Homebrew's CMake could previously be used to build Open RV on macOS, but now it installs CMake version 4, which is too recent and causes dependency issues. An earlier version of CMake must be installed separately:
[cmake-3.31.7-macos-universal.dmg](https://github.com/Kitware/CMake/releases/download/v3.31.7/cmake-3.31.7-macos-universal.dmg).

Add the CMake tool to the PATH using the following command:
```bash
sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install=/usr/local/bin
````

(install_homebrew)=
## Install Homebrew

Homebrew is the one-stop shop providing all the build requirements. You can install it by following the instructions on the [Homebrew page](https://brew.sh).

Make sure Homebrew's binary directory is in your PATH and that `brew` can be resolved from your terminal.

(install_tools_and_build_dependencies)=
## Install tools and build dependencies

Most of the build requirements can be installed by running the following brew install command:

```bash
brew install ninja readline sqlite3 xz zlib tcl-tk@8 autoconf automake libtool python@3.11 yasm clang-format black meson nasm pkg-config glew
```

Make sure `xcode-select -p` still returns `/Applications/Xcode.app/Contents/Developer`. If that is not the case, run `sudo xcode-select -s /Applications/Xcode.app`

(install_qt)=
## Install Qt

Download the last version of Qt 6.5.x using the online installer on the [Qt page](https://www.qt.io/download-open-source). Qt logs, Android, iOS, and WebAssembly are not required to build OpenRV.


WARNING: If you fetch Qt from another source, make sure it is built with SSL support, contains everything required to build PySide6, and that the file structure is similar to the official package.

Note: Qt6 from homebrew is known to not work well with OpenRV.
Note: The CI build agents are currently using Qt 6.5.3

(build_openrv)=
## Build Open RV

(build_openrv1)=
### Before executing any commands

To maximize your chances of successfully building Open RV, you must:
- Fully update your code base to the latest version (or the version you want to use) with a command like `git pull`.
- Fix all conflicts due to updating the code.
- Revisit all modified files to ensure they aren't using old code that changed during the update such as when the Visual Studio version changes.

(build_openrv2)=
### Get Open RV source code

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

(build_openrv3)=
### Load aliases for Open RV

From the Open RV directory:
```shell
source rvcmds.sh
```

(build_openrv4)=
### Install Python dependencies

````{note}
This section needs to be done only once when a fresh Open RV repository is cloned. 
The first time `rvsetup` is executed, it will create a Python virtual environment in the current directory under `.venv`.
````

From the Open RV directory, the following command will download and install the Python dependencies.
```shell
rvsetup
```

(build_openrv5)=
### Configure the project

From the Open RV directory, the following command will configure CMake for the build:

````{tabs}
```{code-tab} bash Release
rvcfg
```
```{code-tab} bash Debug
rvcfgd
```
````

(build_openrv6)=
### Build Open RV

From the Open RV directory, the following command will build the main executable:

````{tabs}
```{code-tab} bash Release
rvbuild
```
```{code-tab} bash Debug
rvbuildd
```
````

(build_openrv7)=
### Opening Open RV executable

````{tabs}
```{tab} Release
Once the build is completed, the Open RV application can be found in the Open RV directory under `_build/stage/app/RV.app/Contents/MacOS/RV`.
```
```{tab} Debug
Once the build is completed, the Open RV application can be found in the Open RV directory under `_build_debug/stage/app/RV.app/Contents/MacOS/RV`.
```
````

(debugging_openrv)=
## 9. Setting up debugging in VSCode

For a general understanding on how to debug C++ code in VSCode, please refer to the [Microsoft documentation](https://code.visualstudio.com/docs/cpp/launch-json-reference).

To set up the C++ debugger in VSCode for Open RV, use the following steps:

1. **Install the [CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb) extension**
2. **Configure the debugger**  
    Create a `.vscode` folder at the root of the project if it doesn't already exist. Inside this folder, create a `launch.json` file to configure the debugger with the following content:

    ```json
    {
        "version": "0.2.0",
        "configurations": [
            {
            "type": "lldb",
            "request": "launch",
            "name": "Debug Open RV (Debug Build)",
            "program": "${workspaceFolder}/_build_debug/stage/app/RV.app/Contents/MacOS/RV",
            "args": [],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "build"
            }
        ]
    }
    ```

    **NOTE:** `program` should point to the build of the Open RV executable you want to debug. `preLaunchTask` is only necessary if you decide to follow step 3.

3. **Set up automatic rebuild (Optional)**  
    If you want to automatically rebuild Open RV before starting the debugger, add a `tasks.json` file in the `.vscode` folder created in the previous step:

    ```json
    {
      "version": "2.0.0",
      "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "_build_debug",
                "--target",
                "main_executable"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always"
            }
        }
      ]
    }
    ```
