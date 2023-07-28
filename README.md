# Open RV
---
[![Open RV](docs/images/OpenRV_icon.png)](https://github.com/AcademySoftwareFoundation/OpenRV.git)
---

![Supported Versions](https://img.shields.io/badge/python-3.9-blue)
[![Supported VFX Platform Versions](https://img.shields.io/badge/vfx%20platform-2023-lightgrey.svg)](http://www.vfxplatform.com/)
[![docs](https://readthedocs.org/projects/aswf-openrv/badge/?version=latest)](https://aswf-openrv.readthedocs.io/en/latest)

## Overview

Open RV is an image and sequence viewer for VFX and animation artists.
Open RV is high-performant, hardware accelerated, and pipeline-friendly.

[Open RV Documentation on Read the Docs](https://aswf-openrv.readthedocs.io/en/latest/)

## Cloning the repository

OpenRV uses submodules to pull some dependencies. When cloning the repository, make sure to do it recursively by using the following command:

```bash
git clone --recursive https://github.com/AcademySoftwareFoundation/OpenRV.git
```

If you cloned the repo without setting the `--recursive` flag, you can initialize the submodule in another step with the following command:

```bash
git submodule update --init --recursive
```

## Building the workstation

Open RV is currently supported on the following operating systems:

* [Windows 10 and 11](docs/build_system/config_windows.md)
* [macOS Big Sur, Monterey and Ventura](docs/build_system/config_macos.md)
* [Linux Centos 7](docs/build_system/config_linux_centos7.md)

Support for other operating systems is on a best effort basis.


## Building RV

You can use `source rvcmds.sh` to add common build aliases into your shell. After the first download following the installation of the required dependencies, use `rvbootstrap` to set up, configure, and build Open RV with the default options.

After the setup, you can use `rvmk` (the common build alias) to configure and build Open RV. You can also use `rvmkd` to configure and build in Debug.

### Contributor setup

This repository uses a pre-commit to execute formatting before a commit. To install the pre-commit hooks:

```shell
pre-commit install
```



### Cleanup

To clean your build directory and restart from a clean slate, use the `rvclean` common build alias, or delete the `_build` folder.



### Bootstrap

Before first your first Open RV build, you must install some python dependencies.

#### Common build alias

Use the `rvsetup` common build alias to run the bootstrap step.

#### Manually

```bash
python3 -m pip install --user --upgrade -r requirements.txt
```



### Configure

The project uses CMake and requires a `configure` step before building. It is during the configure step that you provide your Qt package.

From the root of the repository, execute `cmake` and specify the path to an arbitrary build folder and the path to your QT5 package.

#### Common build alias

Use the `rvcfg` (the common build alias) to run the configuration step. You can also use `rvcfgd` to configure in Debug.

#### Manually

##### Windows

On Windows, you must specify the path to Strawberry perl for the OpenSSL build.

```bash
cmake -B _build -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DRV_DEPS_WIN_PERL_ROOT=/c/Strawberry/perl/bin -DRV_DEPS_QT5_LOCATION=/c/path/to/your/Qt/Root
```

##### Linux and MacOS

```bash
cmake -B _build -DCMAKE_BUILD_TYPE=Release -DRV_DEPS_QT5_LOCATION=/Path/To/Your/Qt5/Root
```

#### Tips

##### 3rd Parties Outside Of Repository

To keep your third-party builds between build cleanups, set `-DRV_DEPS_BASE_DIR=/path/to/third/party`.


### Build

Invoke the previously specified generator tool using cmake to run the `build` step (recommended).

#### Common build alias

Use the `rvbuild` (the common build alias) to run the build step. You can also use `rvbuildd` to build in Debug.

#### Manually

```bash
cmake --build _build --config Release -v --parallel=8 --target main_executable
```



### Test

Invoke the tests using ctest.

#### Common build alias

Use the `rvtest` common build alias to start the tests.

#### Manually

```shell
ctest --test-dir _build
```

#### Tips

##### Run The Tests In Parallel

You can run the `test` in parallel by specifying `--parallel X`, where X is the number of tests to run in parallel.

##### Run A Subset Of The Tests

You can apply a filter with the `-R` flag to specify a regex.

##### Run The Tests Verbose

You can run the tests with extra verbosity with the flag `--extra-verbose`.

> **Important:** You cannot use `--extra-verbose` with `--parallel`. It's one or the other, not both.



### Run

Once the build ends, you can execute (or debug!) Open RV from the _build directory.

The path to the build is `_build/stage/app`. It contains everything required to have the proper debug symbols.



### Install

Invoke the `install` step using cmake. The install step prepares Open RV for packaging by building a copy of Open RV in the `_install` folder.

The build system allows you to prepackage Open RV using cmake's `install` command. It will strip debug symbols if required.

Then, it's up to you to either sign or package the result, or to do both. It should contain the minimum required to have a functional Open RV.

#### Common build alias

Use the `rvinst` common build alias to install OpenRV.

#### Manually

```shell
cmake --install _build --prefix _install
```
