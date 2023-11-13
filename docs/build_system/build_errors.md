# Build Errors and Solutions

### Mu::Parse(char const*, Mu::NodeAssembler*)

```text
Undefined symbols for architecture x86_64:
  "Mu::Parse(char const*, Mu::NodeAssembler*)", referenced from:
      Mu::MuLangContext::evalText(char const*, char const*, Mu::Process*, std::__1::vector<Mu::Module const*, gc_allocator<Mu::Module const*> > const&) in libMuLang.a(MuLangContext.cpp.o)
      Mu::MuLangContext::evalFile(char const*, Mu::Process*, std::__1::vector<Mu::Module const*, gc_allocator<Mu::Module const*> > const&) in libMuLang.a(MuLangContext.cpp.o)
      Mu::MuLangContext::parseType(char const*, Mu::Process*) in libMuLang.a(MuLangContext.cpp.o)
      Mu::MuLangContext::parseStream(Mu::Process*, std::__1::basic_istream<char, std::__1::char_traits<char> >&, char const*) in libMuLang.a(MuLangContext.cpp.o)
ld: symbol(s) not found for architecture x86_64
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```

### Numerous undeclared identifier 'YYMUFlexLexer' Errors

```text
FlexLexer.cpp:2429:10: error: use of undeclared identifier 'YYMUFlexLexer'
    void yyFlexLexer::yy_load_buffer_state()
         ^
```

**Solution:** Most likely, the YY prefix wasn't set properly or at all

### (macOS) from ranlib tool: XYZ has no symbols

```text
[ 85%] Building CXX object src/lib/files/Gto/CMakeFiles/Gto.dir/Parser.cpp.o
[ 85%] Linking CXX static library libGto.a
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ranlib: file: libGto.a(FlexLexer.cpp.o) has no symbols
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ranlib: file: libGto.a(FlexLexer.cpp.o) has no symbols
[100%] Built target Gto
```

**Solution:** Most likely an empty source file was compiled.

### Linker Error, Undefined symbols "Mu::Parse(char const*, Mu::NodeAssembler*)"

```text
Undefined symbols for architecture x86_64:
  "Mu::Parse(char const*, Mu::NodeAssembler*)", referenced from:
      Mu::MuLangContext::evalText(char const*, char const*, Mu::Process*, std::__1::vector<Mu::Module const*, gc_allocator<Mu::Module const*> > const&) in libMuLang.a(MuLangContext.cpp.o)
      Mu::MuLangContext::evalFile(char const*, Mu::Process*, std::__1::vector<Mu::Module const*, gc_allocator<Mu::Module const*> > const&) in libMuLang.a(MuLangContext.cpp.o)
      Mu::MuLangContext::parseType(char const*, Mu::Process*) in libMuLang.a(MuLangContext.cpp.o)
      Mu::MuLangContext::parseStream(Mu::Process*, std::__1::basic_istream<char, std::__1::char_traits<char> >&, char const*) in libMuLang.a(MuLangContext.cpp.o)
ld: symbol(s) not found for architecture x86_64

```

**Solution:** Check that Lexer/Parser compiled source aren't empty! Compiling an empty source file won't generate an error but would endup causing linker to complain du to missing code. This situation did occurred with `sed` command being to write-back to same file. The mitigation solution was to `sed` to a temporary file then rename the temporary file to the expected output filename.

### Multiple 'error: unknown type name 'string' reported in system files

```text
In file included from /OpenRV/src/lib/graphics/TwkGLF/GLVideoDevice.cpp:5:
In file included from /OpenRV/src/lib/graphics/TwkGLF/GLVideoDevice.h:7:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/string:519:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/__debug:14:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/iosfwd:98:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/__mbstate_t.h:29:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/wchar.h:123:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/wchar.h:91:
In file included from /OpenRV/src/lib/base/TwkMath/time.h:7:
In file included from /OpenRV/src/lib/base/TwkMath/math.h:12:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/algorithm:653:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/functional:500:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/__functional/function.h:20:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/__memory/shared_ptr.h:22:
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/__memory/allocator.h:18:
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/stdexcept:83:32: error: unknown type name 'string'
    explicit logic_error(const string&);
                               ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/stdexcept:106:34: error: unknown type name 'string'
    explicit runtime_error(const string&);
                                 ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/stdexcept:126:59: error: unknown type name 'string'
    _LIBCPP_INLINE_VISIBILITY explicit domain_error(const string& __s) : logic_error(__s) {}
                                                          ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/c++/v1/stdexcept:139:63: error: unknown type name 'string'
    _LIBCPP_INLINE_VISIBILITY explicit invalid_argument(const string& __s) : logic_error(__s) {}
```

### (Linux) /usr/bin/env: ‘python’: No such file or directory

```text
env: 'python': No such file or directory
CMake Error at cmake/macros/rv_quote_file.cmake:103 (message):
  Couldn't create file from 'rv_about.html'
Call Stack (most recent call first):
  src/lib/app/RvCommon/CMakeLists.txt:58 (quote_file2)
```

**Solution:** We need a `python` command available.

The newer Ubuntu, Centos & Rocky system usually won't have `python` out of the box, they have a `python3` interpreter.

One solution is to instyall `pipenv` and start a local virtual environment. That will give you a `python` command.

### (Linux)  pyconfig.h: No such file or directory

Should be in `/home/nmontmarquette/local/cache/RV_build/RV_DEPS_DESKTOP_PACKAGE/src/Python3/Release/include/python3.9/pyconfig.h`

/home/nmontmarquette/local/cache/RV_build/RV_DEPS_DESKTOP_PACKAGE/src/Python3/Release/bin/python3.9

### (Linux) error: libaio.h: No such file or directory

```text
/OpenRV/src/lib/base/TwkUtil/FileStream.cpp:170:10: fatal error: libaio.h: No such file or directory
 #include <libaio.h>
          ^~~~~~~~~~
```

**Solution:**

```bash
sudo dnf install libaio-devel
```

### (Rocky Linux)

```text
/usr/bin/ld: ../../../lib/mu/Mu/libMu.a(Thread.cpp.o): undefined reference to symbol 'pthread_attr_getstacksize@@GLIBC_2.2.5'
//usr/lib64/libpthread.so.0: error adding symbols: DSO missing from command line
collect2: error: ld returned 1 exit status
make[2]: *** [src/bin/apps/rv/CMakeFiles/rv.dir/build.make:228: src/bin/apps/rv/rv] Error 1
make[1]: *** [CMakeFiles/Makefile2:2525: src/bin/apps/rv/CMakeFiles/rv.dir/all] Error 2
make: *** [Makefile:91: all] Error 2
```

### (Linux any Distro)
```text
/opt/rh/devtoolset-8/root/usr/libexec/gcc/x86_64-redhat-linux/8/ld: ../../../lib/graphics/TwkGLF/libTwkGLF.a(GLFBO.cpp.o): undefined reference to symbol 'glBindFramebufferEXT'
//lib64/libGL.so.1: error adding symbols: DSO missing from command line
collect2: error: ld returned 1 exit status
make[3]: *** [src/bin/apps/rv/CMakeFiles/rv.dir/build.make:236: src/bin/apps/rv/rv] Error 1
make[2]: *** [CMakeFiles/Makefile2:2497: src/bin/apps/rv/CMakeFiles/rv.dir/all] Error 2
make[1]: *** [CMakeFiles/Makefile2:2504: src/bin/apps/rv/CMakeFiles/rv.dir/rule] Error 2
make: *** [Makefile:254: rv] Error 2
[nmontmarquette@CentOS79 rv_build]$
```

**Solution:**

Ref.: [Resolve "DSO missing from command line" error](https://zhangboyi.gitlab.io/post/2020-09-14-resolve-dso-missing-from-command-line-error/)
Ref.: [error adding symbols: DSO missing from command line](https://stackoverflow.com/questions/19901934/libpthread-so-0-error-adding-symbols-dso-missing-from-command-line)


### Windows 10 & Windows 11 (Windows 10+) : PIP REQUIREMENTS: py7zr fails to install

```
  [end of output]

        note: This error originates from a subprocess, and is likely not a problem with pip.

      See above for output.

      note: This is an issue with the package mentioned above, not pip.
      hint: See above for details.
      [end of output]

  note: This error originates from a subprocess, and is likely not a problem with pip.
error: subprocess-exited-with-error

× pip subprocess to install build dependencies did not run successfully.
```
Note: There's an issue with the latest version of MSYS2 and Python3.xx versions. Enter the following command from the same MSYS2-MinGW64 shell if you encounter an error when installing the **py7zr** python requirement:

```shell
SETUPTOOLS_USE_DISTUTILS=stdlib pip install py7zr
```

### Windows : System-wide vcpkg

Another installation of vcpkg can conflict with the one installed by the build system. Currently,
there are only two possibles solutions:

  - Option A: Remove the automatic integration of the system-wide vcpkg into cmake using :
    ```bash 
    vcpkg integrate remove
    ```
  - Option B: Make sure that the right dependencies (and version) are installed in the system-wide vcpkg.

### macOS, Linux, Windows
```text
[ 99%] Linking CXX executable rv
CMakeFiles/rv.dir/main.cpp.o: In function `utf8Main(int, char**)':
/OpenRV/src/bin/apps/rv/main.cpp:419: undefined reference to `qInitResources_rv()'
collect2: error: ld returned 1 exit status
make[2]: *** [src/bin/apps/rv/rv] Error 1
make[1]: *** [src/bin/apps/rv/CMakeFiles/rv.dir/all] Error 2
make: *** [all] Error 2
```

**Solution:**

Most likely missing a `qt5_add_resources` CMake statement.
Ref.: https://forum.qt.io/topic/88959/undefined-reference-to-qinitresources/2
