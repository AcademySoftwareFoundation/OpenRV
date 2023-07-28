#   NOTE ON PATENT LICENSING:
#
#   Legal Note: Some of the codecs available in ffmpeg contain proprietary
#   algorithms that are protected under intellectual property rights. Please
#   check with your legal department whether you have the proper licenses and
#   rights to use these codecs. TWEAK is not responsible for any unlicensed use
#   of these codecs.
#
#   Upshot: You should not enable codecs you do not have a license for.

# How to build

## Mac & Linux

1) Create a build folder next to CMakeLists.txt, it can be named 'build'.
2) cd (Change Directory) to this new build folder.
3) call CMake to Configure the project, you may need to pass a Generator. The command is: 'cmake <-G generator> -B . -S .. -DCMAKE_BUILD_TYPE=<Debug|Release>
4) Once configure is done: it's time to build which depends on your generator. Examples:
    a) for -G Unix Makefiles (the default is -G is not passed): 'make -j8 VERBOSE=1'
    b) for -G Ninja: 'ninja -v'

## Windows

1) Create a build folder next to CMakeLists.txt, it can be named 'build'.
2) cd (Change Directory) to this new build folder.
3) call CMake to Configure the project. The command is: 'cmake -G "Visual Studio 16 2019" -B . -S ..'
4) Once configure is done: you can build in the same folder. Note that the Release or Debug config flag is passed in this command for Windows: cmake --build . --config Release
5) Use --config Debug for Debug. Passing the --parallel flag is possible for building.
