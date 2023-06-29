# How to build

1) Create a build folder next to CMakeLists.txt, it can be named 'build'.
2) cd (Change Directory) to this new build folder.
3) call CMake to Configure the project, you may need to pass a Generator. The command is: 'cmake <-G generator> -B . -S .. -DCMAKE_BUILD_TYPE=<Debug|Release>
4) Once configure is done: it's time to build which depends on your generator. Examples:
    a) for -G Unix Makefiles (the default is -G is not passed): 'make -j8 VERBOSE=1'
    b) for -G Ninja: 'ninja -v'