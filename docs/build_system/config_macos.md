# Preparing Open RV on macOS

Open RV 2025 can be built for macOS using the [VFX reference platform](https://vfxplatform.com/).  Dependencies can be viewed in the `cmake/defaults/` folder.  eg [cmake/defaults/CY2026.cmake](https://github.com/AcademySoftwareFoundation/OpenRV/tree/main/cmake/defaults)

Select your VFX reference platform by clicking on the appropriate tab. Install instructions follows.

* NOTE: CY2025 and CY2026 are experimental.  Noteably CY2026 for RV is still on Qt 6.5.3 pending changes necessary for 6.8.3

````{tabs}
```{code-tab} bash VFX-CY2026
Boost               : 1.88.0
Cmake               : 3.31.X+
Imath               : 3.2.2
NumPy               : 2.3.0
OCIO                : 2.5.0
OpenEXR             : 3.4.3
OpenSSL             : 3.6.0
Python              : 3.13.9
Qt                  : 6.8.3 ** RV Still on 6.5.3 pending work
Xcode               : See XCode section

```
```{code-tab} bash VFX-CY2025
Boost               : 1.85.0
Cmake               : 3.31.X+
Imath               : 3.1.12
NumPy               : 1.26.4
OCIO                : 2.4.2
OpenEXR             : 3.3.6
OpenSSL             : 3.4.0
Python              : 3.11.14
Qt                  : 6.5.3
Xcode               : See XCode section

```
```{code-tab} bash VFX-CY2024
Boost               : 1.82.0
Cmake               : 3.31.X+
Imath               : 3.1.12
NumPy               : 1.24.4
OCIO                : 2.3.2
OpenEXR             : 3.2.5
OpenSSL             : 3.4.0
Python              : 3.11.14
Qt                  : 6.5.3
Xcode               : See XCode section

```
```{code-tab} bash VFX-CY2023
Boost               : 1.80
Cmake               : 3.31.7
Imath               : 3.1.12
NumPy               : 1.23.5
OCIO                : 2.2.1
OpenEXR             : 3.1.13
OpenSSL             : 1.1.1u
Python              : 3.10.18
Qt                  : 5.15.18
Xcode               : See XCode section

```
````

All other dependencies are shared across variations.


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
Open RV can be built for *x86_64* by changing the architecture of the terminal to *x86_64* using the following command:
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

**Heads Up:**
If you are using >= Xcode 26 you will need to patch Qt 6.5.3 and 6.8.3 to implement the fix for [QTBUG-137687](https://bugreports.qt.io/browse/QTBUG-137687).

This patch is run during the sourcing of rvcmds.sh.  You can also run it directly via `sh apply_qt_fix.sh` 

Alternately you can use Xcode 16.4 on the latest macOS Tahoe 26 to build Open RV.

From the App Store, download Xcode 16.4. Make sure that it is the source of the active developer directory.

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

````{tabs}
```{code-tab} bash VFX-CY2024
brew install ninja readline sqlite3 xz zlib tcl-tk@8 autoconf automake libtool python@3.11 yasm clang-format black meson nasm pkg-config glew rust
```
```{code-tab} bash VFX-CY2023
brew install ninja readline sqlite3 xz zlib tcl-tk@8 autoconf automake libtool python@3.10 yasm clang-format black meson nasm pkg-config glew rust
```
````

````{warning}
Rust version **1.92 or later** is required to build certain Python dependencies (such as cryptography) that contain Rust components.
Homebrew will install the latest stable version of Rust.
````

Make sure `xcode-select -p` still returns `/Applications/Xcode.app/Contents/Developer`. If that is not the case, run `sudo xcode-select -s /Applications/Xcode.app`

(install_qt)=
## Install Qt

Download the Qt version corresponding to your chosen VFX reference platform using the online installer on the [Qt page](https://www.qt.io/download-open-source). Qt logs, Android, iOS, and WebAssembly are not required to build Open RV.

````{tabs}
```{code-tab} bash VFX-CY2024
Download Qt 6.5.3 using the online installer.
This version should be available in the regular Qt installer without needing archives.
The CI build agents are currently using Qt 6.5.3.
```
```{code-tab} bash VFX-CY2023
Download Qt 5.15.2 from the Qt archives.
You will need to enable archive packages in the Qt installer to access Qt 5.15.2.
```
````

**WARNING**: If you fetch Qt from another source, make sure it is built with SSL support, contains everything required to build PySide6 (for VFX-CY2024) or PySide2 (for VFX-CY2023), and that the file structure is similar to the official package.

**Note**: Qt from homebrew is known to not work well with Open RV.

(build_openrv)=
## Build Open RV

Once the platform-specific installation process is complete, building Open RV follows the same process for all platforms. Please refer to the [Common Build Instructions](config_common_build.md) for the complete build process.

### macOS-Specific Build Notes

#### Executable Location

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
