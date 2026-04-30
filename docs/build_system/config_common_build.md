# Building UTV

Once the platform-specific installation process is done, building RV is essentially the same process for all platforms.

### 1. Building UTV for the first time

#### 1.1 Clone the UTV repository using HTTPS or SSH key

Clone the UTV repository. Typically, this will create an "UTV" directory from your current location.

```bash
# If using a password-protected SSH key:
git clone --recursive git@github.com:AcademySoftwareFoundation/UTV.git
cd UTV
```

```bash
# Or if using the web URL:
git clone --recursive https://github.com/AcademySoftwareFoundation/UTV.git
cd UTV
```

If you cloned the repo without setting the `--recursive` flag, you can initialize the submodule in another step with the following command:

```bash
git submodule update --init --recursive
```

**Note: If you plan to contribute and submit pull requests for review** you should create a GitHub account and follow the standard GitHub fork model for submitting PRs. This involves creating your own github account, forking the Academy Software Foundation's UTV repository, making your changes in a branch of your forked version, and then submitting pull requests from your forked repository (don't forget to sync your fork regularly!). In that case, substitute the above repository URL above with the URL of your own fork.

#### 1.2 Configuring Git to use the ignore file with `git blame`

A `.git-blame-ignore-revs` file lists commits to ignore when running `git blame`, such as formatting commits. This allows you to use `git blame` without these commits cluttering the Git history. To configure Git to use this file when running `git blame`, use the following command:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

#### 1.3 Build UTV using build.sh

We provide a standard `build.sh` script to simplify the process of setting up the environment and compiling UTV. Once in your UTV directory:

```bash
./build.sh
```

By default, the script configures and builds a Release version. You can switch between debug and release configurations by passing arguments to the script.

#### 1.4 First-time build

This script handles everything automatically: it will create a Python virtual environment (using `uv` if available for maximum speed), fetch source dependencies if necessary, and compile the engine.

After the setup stage is done, a build is started and should produce a valid "utv" executable binary.

````{tabs}
```{code-tab} bash Release
# Produces default optimized build in UTV/_build
./build.sh --release
```
```{code-tab} bash Debug
# Produces unoptimized debug build in UTV/_build_debug
./build.sh --debug
```
````

Note 1: launch the default optimized build unless you have a reason to want the unoptimized debug build.

Note 2: It's possible that after boostrapping the build fails. If this happens, building again often fixes the problem. From the command line, call `./build.sh` to complete the build.

### 2. Building UTV after the first time

To build UTV after the first time, simply run `./build.sh` again. It will launch an incremental build.

````{tabs}
```{code-tab} bash Release
# Produces incremental optimized build
./build.sh --release
```
```{code-tab} bash Debug
# Produces incremental unoptimized debug build
./build.sh --debug
```
````

### 3. Starting the UTV executable

Once UTV is finished building, its executable binary can be found here:

For Windows and Linux:

````{tabs}
```{code-tab} bash Release
_build/stage/app/bin/utv
```
```{code-tab} bash Debug
_build/stage/app/bin/utv
```
````

For macOS:

````{tabs}
```{code-tab} bash Release
_build/stage/app/UTV.app/Contents/MacOS/utv
```
```{code-tab} bash Debug
_build/stage/app/UTV.app/Contents/MacOS/utv
```
````

### 4. Contributing to UTV

Before you can submit any code for a pull request, this repository uses the `pre-commit` tool to perform basic checks and to execute formatting hooks before a commit. To install the pre-commit hooks, run the following command:

```bash
pre-commit install
```

### 5. Clang-tidy

Although not strictly enforced, it is highly suggested to enable clang-tidy locally to lint the C++ code you plan to contribute to the project. A `.clang-tidy` configuration file is present at the root of the project to help standardize linting rules. While it is recommended to use clangd (which integrates clang-tidy), you can refer to the list of other well-known [clang-tidy IDE integrations](https://clang.llvm.org/extra/clang-tidy/Integrations.html). For more details on how to install everything you need for your IDE, please follow the [clangd installation guide](https://clangd.llvm.org/installation).

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

### 6. Ruff

Ruff is used in this project for both Python linting and formatting, and its checks are enforced through a pre-commit hook and our Github actions CI. You can run Ruff through the `pre-commit` tool with the following commands:

```bash
pre-commit run ruff-check --fix --all-files # Lint and fix issues found on all files
pre-commit run ruff-format --all-files # Format all files
```

For more details on the Ruff commands you can run, please take a look at the [Ruff documentation](https://docs.astral.sh/ruff/).

It is highly recommended to also add Ruff to your IDE. For VSCode users, you can activate Ruff by installing the [Ruff extension](https://marketplace.visualstudio.com/items?itemName=charliermarsh.ruff). For other IDEs, please refer to the [Editor Integration page](https://docs.astral.sh/ruff/editors/setup/).

### 7. Cleaning up your build directory

To clean your build directory and restart from a clean slate, use the `--clean` argument, or manually delete the `_build` directory:

```bash
./build.sh --clean
```

To keep your third-party build between cleanups, set: `-DRV_DEPS_BASE_DIR=/path/to/third/party`.

### 8. Installing Blackmagicdesign&reg; Video Output Support (Optional)

Download the Blackmagicdesign&reg; SDK to add Blackmagicdesign&reg; output capability to UTV (optional): <https://www.blackmagicdesign.com/desktopvideo_sdk><br>
Then set RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH to the path of the downloaded zip file on the rvcfg line.<br>
Example:

```bash
rvcfg -DRV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH='<downloads_path>/Blackmagic_DeckLink_SDK_14.1.zip'
```

### 9. NDI&reg; Video Output Support (Optional)

Download and install the NDI&reg; SDK to add NDI&reg; output capability to UTV (optional): <https://ndi.video/><br>

Installing the NDI SDK must be done before building UTV for the first time (if you add it later, it's easiest to just delete your build folder, and execute the first-time build procedure again)

### 10. How to enable non-free FFmpeg codecs

Legal Notice: Non-free FFmpeg codecs are disabled by default. Please check with your legal department whether you have the proper licenses and rights to use these codecs.
ASWF is not responsible for any unlicensed use of these codecs.

The RV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE and RV_FFMPEG_NON_FREE_ENCODERS_TO_ENABLE can optionally be specified at configure time to enable non free FFmpeg decoders and encoders respectively.

For example:

```bash
rvcfg -DRV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE="aac;hevc" -DRV_FFMPEG_NON_FREE_ENCODERS_TO_ENABLE="aac"
```

### 11. Apple ProRes

If you have Apple's ProRes SDK for Windows or Linux (you can obtain freely by contacting [ProRes@apple.com](mailto:ProRes@apple.com)),
then set ```RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH``` to the path of the downloaded zip file on the rvcfg line.<br>
Example:

```bash
-DRV_DEPS_APPLE_PRORES_SDK_ZIP_PATH='<downloads_path>/ProResDecoder_Linux_x86_64-15B54.zip'
```

By default, the ProRes decode via the SDK will use all available system threads.  To use a fixed maximum number of threads, set the
environment variable `RV_PREF_GLOBAL_PRORES_DECODER_THREADS` to a positive value.

On Apple Silicon machines, UTV supports hardware decoding through Apple's VideoToolbox framework. This feature is enabled by default
but can be controlled using the `-DRV_FFMPEG_USE_VIDEOTOOLBOX` option. Set this option to `ON` to enable or `OFF` to disable VideoToolbox
hardware decoding.

To enable decoding of ProRes media files, you must also specify the following option during the configuration step:

```bash
-DRV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE="prores"
```

Note that you should always have `-DRV_FFMPEG_USE_VIDEOTOOLBOX` enabled when decoding Apple ProRes videos on Apple Silicon machines.
Failure to do so will result in performance issues and is not compliant with Apple's licensing requirements.

**Important:** Before enabling ProRes decoding on Linux/Windows, you are required to obtain a proper license agreement and the SDK from
Apple by contacting [ProRes@apple.com](mailto:ProRes@apple.com).

### 12. Running the automated tests

UTV uses ctest to run its automated tests.

To run all tests automatically:

```shell
rvtest
```

To run tests manually,

```shell
ctest --test-dir _build
```

#### 12.1 Running The Tests In Parallel

You can run the `test` in parallel by specifying `--parallel X`, where X is the number of tests to run in parallel.

#### 12.2 Running A Subset Of The Tests

You can apply a filter with the `-R` flag to specify a regex.

#### 12.3 Running The Tests Verbose

You can run the tests with extra verbosity with the flag `--extra-verbose`.

> **Important:** You cannot use `--extra-verbose` with `--parallel`. It's one or the other, not both.

### 13. Creating the installation package

To create the installation package, invoke the `install` step using cmake. The install step prepares UTV for packaging by building a copy of UTV in the `_install` folder. This step will strip debug symbols from the executable if required.

Afterwards, it's up to you to either sign or package the result, or to do both. The result should contain the minimum required to have a functional UTV.

#### 13.1 Creating the installation package automatically

```shell
rvinst
```

#### 13.2 Creating the installation package manually

```shell
cmake --install _build --prefix _install
```

### 14. Building with Docker

Building with Docker is currently only supported on Rocky Linux. Please refer to the Rocky Linux setup instructions for details.
