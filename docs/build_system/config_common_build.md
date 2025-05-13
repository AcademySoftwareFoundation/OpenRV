# Building Open RV

Once the platform-specific installation process is done, building RV is essentially the same process for all platforms.

### 1. Building OpenRV for the first time

#### 1.1 Clone the RV repository using HTTPS or SSH key

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

If you cloned the repo without setting the `--recursive` flag, you can initialize the submodule in another step with the following command:

```bash
git submodule update --init --recursive
```

**Note: If you plan to contribute and submit pull requests for review** you should create a GitHub account and follow the standard GitHub fork model for submitting PRs. This involves creating your own github account, forking the Academy Software Foundation's OpenRV repository, making your changes in a branch of your forked version, and then submitting pull requests from your forked repository (don't forget to sync your fork regularly!). In that case, substitute the above repository URL above with the URL of your own fork.


#### 1.2 Configuring Git to use the ignore file with `git blame`

A `.git-blame-ignore-revs` file lists commits to ignore when running `git blame`, such as formatting commits. This allows you to use `git blame` without these commits cluttering the Git history. To configure Git to use this file when running `git blame`, use the following command:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

#### 1.3 Load command aliases to build OpenRV

Command-line aliases are provided to simplify the process of setting up the environment and to build OpenRV. Once in your OpenRV directory:

```bash
source rvcmds.sh
```

#### 1.4 First-time build only: rvbootstrap

This step only needs to be done on a freshly cloned git repository. Under the hood, his command will create an initial setup environment, will fetch source dependencies, install other required elements, and will create a Python virtual environment in the current directory under the `.venv` directory.

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


### 2. Building OpenRV after the first time

Once you've built OpenRV for the first time, there is no need to run "rvboostrap" again.

#### 2.1 Build OpenRV (incrementally) using rvmk

To build OpenRV after the first time, "rvmk" will correctly configure the environment variables and launch the incremental build process. 

```bash
# Produces incremental optimized build in ~/OpenRV/_build_debug
rvmk
```
```bash
# Produces incremental unoptimized debug build in ~/OpenRV/_build_debug
rvmkd
```

Note that, under the hood, rvmk/rvmkd is literally the equivalent to these two commands:
```bash
rvcfg/rvcfgd      # sets environment variables
rvbuild/rvbuildd  # launches the build process
```


#### 2.2 Rebuilding the dependencies

Building the source dependencies is done automatically the first time we build OpenRV with "rvbootstrap/d" so you typically never need to rebuild them. In the rare event you would need to fix a bug or update one such third-party source dependency, dependencies can be rebuild this way:


```bash
# Rebuild dependencies for default optimized build
rvbuildt dependencies
```
```bash
# Rebuild dependencies for debug build
rvbuildtd dependencies
```


### 3. Starting the OpenRV executable

Once OpenRV is finished building, its executable binary can be found here:

```bash
# For the default optimized build:
`~/OpenRV/_build/stage/app/bin/rv`.
```
```bash
# For the debug build
`~/OpenRV/_build_debug/stage/app/bin/rv`.
```


On macOS, if you want to build for a different deployment target than your current operating system, make sure to define
the MACOSX_DEPLOYMENT_TARGET environment variable.

### 4. Contributing to OpenRV 

Before you can submit any code for a pull request, this repository uses the `pre-commit` tool to perform basic checks and to execute formatting hooks before a commit. To install the pre-commit hooks, run the following command:

```bash
pre-commit install
```

### 5. Clang-tidy

Although not strictly enforced, it is highly suggested to enable clang-tidy locally to lint the C++ code you plan to contribute to the project. A `.clang-tidy` configuration file is present at the root of the project to help standardize linting rules. While it is recommended to use clangd (which integrates clang-tidy), you can refer to the list of other well-known clang-tidy integrations [here](https://clang.llvm.org/extra/clang-tidy/Integrations.html). For more details on how to install everything you need for your IDE, please follow the steps outlined [here](https://clangd.llvm.org/installation).

1. Install clangd with your package manager

2. Install the clangd extension in your IDE
    > **Note:** For VSCode users, after installing [vscode-clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd), you need to disable [Microsoft C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) IntelliSense by adding `"C_Cpp.intelliSenseEngine": "disabled"` to your `settings.json`.

3. Generate a `compile_commands.json` file with CMake during the environment configuration step

     ```shell
    rvcfg -DCMAKE_EXPORT_COMPILE_COMMANDS=1
    ```

4. Create a symlink to the `compile_commands.json` file in the root directory of the project so that `clangd` can locate it:

    ```shell
    ln -s _build/compile_commands.json compile_commands.json
    ```

### 6. Cleaning up your build directory

To clean your build directory and restart from a clean slate, use the `rvclean` alias, or delete the `_build` folder.

To keep your third-party builds between build cleanups, set `-DRV_DEPS_BASE_DIR=/path/to/third/party`.

### 7. Installing Blackmagicdesign&reg; Video Output Support (Optional)

Download the Blackmagicdesign&reg; SDK to add Blackmagicdesign&reg; output capability to Open RV (optional): https://www.blackmagicdesign.com/desktopvideo_sdk<br>
Then set RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH to the path of the downloaded zip file on the rvcfg line.<br>
Example:
```bash
rvcfg -DRV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH='<downloads_path>/Blackmagic_DeckLink_SDK_14.1.zip'
```

### 8. NDI&reg; Video Output Support (Optional)

Download and install the NDI&reg; SDK to add NDI&reg; output capability to Open RV (optional): https://ndi.video/<br>

Installing the NDI SDK must be done before building OpenRV for the first time (if you add it later, it's easiest to just delete your build folder, and execute the first-time build procedure again)

### 9. How to enable non-free FFmpeg codecs

Legal Notice: Non-free FFmpeg codecs are disabled by default. Please check with your legal department whether you have the proper licenses and rights to use these codecs. 
ASWF is not responsible for any unlicensed use of these codecs.

The RV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE and RV_FFMPEG_NON_FREE_ENCODERS_TO_ENABLE can optionally be specified at configure time to enable non free FFmpeg decoders and encoders respectively.

For example:

```bash
rvcfg -DRV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE="aac;hevc" -DRV_FFMPEG_NON_FREE_ENCODERS_TO_ENABLE="aac"
```



### 10. Apple ProRes on Apple Silicon

On Apple Silicon machines, Open RV supports hardware decoding through Apple's VideoToolbox framework. This feature is enabled by default but can be controlled using the `-DRV_FFMPEG_USE_VIDEOTOOLBOX` option. Set this option to `ON` to enable or `OFF` to disable VideoToolbox hardware decoding.

To enable decoding of ProRes media files, you must also specify the following option during the configuration step:

```bash
-DRV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE="prores"
```

Note that you should always have `-DRV_FFMPEG_USE_VIDEOTOOLBOX` enabled when decoding Apple ProRes videos on Apple Silicon machines. Failure to do so will result in performance issues and is not compliant with Apple's licensing requirements.

**Important:** Before enabling ProRes decoding, you are required to obtain a proper license agreement from Apple by contacting [ProRes@apple.com](mailto:ProRes@apple.com).



### 11. Running the automated tests

OpenRV uses ctest to run its automated tests.

To run all tests automatically:

```shell
rvtest
```

To run tests manually, 

```shell
ctest --test-dir _build
```

#### 11.1 Running The Tests In Parallel

You can run the `test` in parallel by specifying `--parallel X`, where X is the number of tests to run in parallel.

#### 11.2 Running A Subset Of The Tests

You can apply a filter with the `-R` flag to specify a regex.

#### 11.3 Running The Tests Verbose

You can run the tests with extra verbosity with the flag `--extra-verbose`.

> **Important:** You cannot use `--extra-verbose` with `--parallel`. It's one or the other, not both.




### 12. Creating the installation package

To create the installation package, invoke the `install` step using cmake. The install step prepares OpenRV for packaging by building a copy of Open RV in the `_install` folder. This step will strip debug symbols from the executable if required.

Afterwards, it's up to you to either sign or package the result, or to do both. The result should contain the minimum required to have a functional OpenRV.

#### 12.1 Creating the installation package automatically

```shell
rvinst
```

#### 12.2 Creating the installation package manually

```shell
cmake --install _build --prefix _install
```


### 13. Building with Docker

Building with Docker is currently only supported on Rocky Linux. Please refer to the Rocky Linux setup instructions for details.


