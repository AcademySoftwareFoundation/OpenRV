# Open RV
Open RV is an image and sequence viewer for VFX and animation artists.
Open RV is high-performant, hardware accelerated, and pipeline-friendly.

## Documentation
[RV Documentation](doc/rv-manuals.md)

## Build Instructions

You can build Open RV for Linux, Windows, and macOS. You build it from scratch using the instructions specific to each OS:

* [Linux](doc/build_system/config_linux_centos7.md)
* [Windows](doc/build_system/config_windows.md)
* [macOS](doc/build_system/config_macos.md)

### Building Tips

#### Build Aliases

You can use `source rvmds.sh` to add common build aliases into your shell. After the first download following the installation of the required dependencies, use `rvbootstrap` to set up, configure, and build Open RV with the default options.

After the setup, you can use `rvmk` to build.

#### Compile in Parallel

Start the compilation in parallel by adding `--parallel[=threadCound]` to the build command. This is the same as using `-j`.

#### 3rd Parties Outside Of Repository

If you desire to keep your third-party builds between build cleanups, set `-DRV_DEPS_BASE_DIR=/path/to/third/party`.

#### Expert Mode

You can always go to the build directory and call the generator directly.

## Running Open RV

Once the build ends, you can execute (or debug!) Open RV from the cmake-build directory.

The path to the build is `cmake-build/stage/app`. The Open RV cmake options set up the environment so you can start the build without RPATH issues.

## Install

The recommended method to install Open RV is to invoke the install build step tool using cmake.

The build system allows you to prepackage Open RV using cmake's install command and a prefix.

Then, it's up to you to either sign or package the result, or to do both. It should contain the minimum required to have a full Open RV.

```shell
cmake --install cmake-build --prefix /Absolute/path/to/a/destination/folder
```

## Run Tests

You invoke Open RV tests with the following command:

```shell
ctest --test-dir cmake-build --extra-verbose
```

### Tests Tips

#### Run The Tests In Parallel

You can run the tests in parallel by specifying `--parallel X`, where X is the number of tests to run in parallel.

#### Run A Subset Of The Tests

You can apply a filter with the `-R` flag to specify a regex.

#### Run The Tests Verbose

You can run the tests with extra verbosity with the flag `--extra-verbose`. 

> **Important:** You cannot use `--extra-verbose` with `--parallel`. It's one or the other, not both.

## Contributing to Open RV

This repository uses pre-commit to have formatting executed before a commit. To install the hooks:

```shell
pre-commit install
```

> **Important:** When the hooks reformat a file, you need to re-add them to git to have your `git commit` command executed.
> Also, you can skip the hook execution by using `git commit -n`.
