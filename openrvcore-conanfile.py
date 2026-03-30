import os
import sys
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, cmake_layout, CMakeToolchain
from conan.tools.microsoft import VCVars
from conan.tools.env import VirtualBuildEnv
from conan.errors import ConanException

# The comment below is relevant for WINDOWS only because of the usage of MSYS2.

# The Python path should be pre-appended to the PATH environment variables in the
# buildenv section of the profile used to build the recipe.
# It will ensure that the Python path is the first path in the environment variable.
# This is a special case because of the way that MSYS2 merges the PATH environment variables.
# (MSYS2 PATH = MSYS2 PATH + CMD PATH)
#
# Here's an example of a buildenv section (also, see example under conan/profiles):
#
# [buildenv]
# PATH=+(path)/c/Program Files/Python310
#


class OpenRVBase:
    setuptools_use_distutils = ""

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        if self.settings.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                pkgs = [
                    "mingw-w64-x86_64-autotools",
                    "mingw-w64-x86_64-cmake",
                    "mingw-w64-x86_64-cmake-cmcldeps",
                    "mingw-w64-x86_64-glew",
                    "mingw-w64-x86_64-libarchive",
                    "mingw-w64-x86_64-make",
                    "mingw-w64-x86_64-meson",
                    "mingw-w64-x86_64-python-pip",
                    "mingw-w64-x86_64-python-psutil",
                    "mingw-w64-x86_64-toolchain",
                    "autoconf",
                    "automake",
                    "bison",
                    "flex",
                    "git",
                    "libtool",
                    "nasm",
                    "p7zip",
                    "patch",
                ]
                pkgsStr = " ".join(pkgs)
                self.tool_requires("msys2/cci.latest", options={"additional_packages": pkgsStr})
        else:
            self.tool_requires("ninja/1.11.1")

        self.tool_requires("cmake/3.31.8")

    def requirements(self):
        self.requires("zlib/1.3.1", force=True, options={"shared": True})

        self.requires("libatomic_ops/7.10.0", options={"shared": False})

        # Version conflict: ffmpeg/4.4.3->libwebp/1.3.2, ->libwebp/1.2.1
        # Webp >=1.3.0 depends on sharpyuv and it causes issues with OIIO.
        self.requires("libwebp/1.2.1", force=True, options={"shared": False})

        self.requires("dav1d/1.5.3", force=True, options={"shared": True})

        self.requires(
            "libjpeg-turbo/2.1.4",
            force=True,
            options={
                "shared": True,
            },
        )

        # Override openssl version for ffmpeg, but do not use it for OpenRV right now.
        self.requires("openssl/3.5.0", force=True, options={"shared": True, "no_zlib": True})

        # Override boost version for other dependencies.
        self.requires(
            "boost/1.82.0#7053477b271b8c39e046f784405b402f",
            force=True,
            options={"shared": True, "extra_b2_flags": "-d+0 -s NO_LZMA=1"},
        )

        # PCRE2 for Windows (boost regex used on other platforms)
        if self.settings.os == "Windows":
            self.requires(
                "pcre2/10.44",
                force=True,
                options={
                    "shared": True,
                    "with_zlib": False,
                    "with_bzip2": False,
                    "support_jit": False,
                    "build_pcre2_16": False,
                    "build_pcre2_32": False,
                },
            )

        # Override imath version for other dependencies.
        self.requires("imath/3.2.1", force=True, options={"shared": True})

        self.requires("openexr/3.2.5", force=True, options={"shared": True})

        self.requires("libpng/1.6.55", force=True, options={"shared": True})

        # libtiff is force-built from source in CMake (private headers needed).
        # Excluded from Conan: libtiff 4.6.0 doesn't build against libjpeg-turbo (12-bit API).
        # openjph/openexr pull it in transitively — disabled via openjph:with_tiff=False.

        self.requires("openjpeg/2.5.4", force=True, options={"shared": True})

        self.requires(
            "libraw/0.21.1",
            force=True,
            options={"shared": True, "with_jpeg": "libjpeg-turbo", "with_jasper": False},
        )

        self.requires("openjph/0.26.3", force=True, options={"shared": True})

    def generate(self):
        buildenv = VirtualBuildEnv(self)
        buildenv.generate()

        if self.settings.os == "Windows":
            ms = VCVars(self)
            ms.generate()

        deps = CMakeDeps(self)
        deps.generate()

        # Setup CMakeToolchain generator.
        if not self.settings.os == "Windows":
            tc = CMakeToolchain(self, generator="Ninja")
        else:
            tc = CMakeToolchain(self)

        # Qt location: use QT_HOME env var, fall back to conventional paths.
        qt_version = os.getenv("QT_VERSION", "6.5.3")
        qt_home = os.getenv("QT_HOME")

        if not qt_home:
            # Build platform-specific default path
            if self.settings.os == "Windows":
                qt_home = f"c:/Qt/{qt_version}/msvc2019_64"
            else:
                home = os.getenv("HOME", "")
                qt_dir = os.path.join(home, "Qt6.5.3", qt_version)
                if self.settings.os == "Linux":
                    qt_home = os.path.join(qt_dir, "gcc_64")
                elif self.settings.os == "Macos":
                    # Recent Qt versions use "macos", older ones use "clang_64"
                    macos_path = os.path.join(qt_dir, "macos")
                    clang_path = os.path.join(qt_dir, "clang_64")
                    qt_home = macos_path if os.path.isdir(macos_path) else clang_path

            self.output.warning(
                f"QT_HOME not set, using default: {qt_home}. "
                "Set the QT_HOME environment variable to point to your Qt installation."
            )

        if not os.path.isdir(qt_home):
            raise ConanException(
                f"Qt directory not found: {qt_home}. "
                "Set the QT_HOME environment variable to a valid Qt installation path."
            )

        if self.settings.os == "Windows":
            win_perl_location = os.getenv("WIN_PERL", "c:/Strawberry/perl/bin")
            if not os.path.isdir(win_perl_location):
                self.output.warning(
                    f"Perl directory not found: {win_perl_location}. "
                    "Set the WIN_PERL environment variable to your Perl installation."
                )
            self.setuptools_use_distutils = "stdlib"

        # VFX Platform — controlled by the vfx_platform option in the main conanfile.py.
        tc.cache_variables["RV_VFX_PLATFORM"] = str(self.options.vfx_platform)

        # Set up CMake's cached variables.
        tc.cache_variables["RV_DEPS_QT_LOCATION"] = qt_home
        if self.settings.os == "Windows":
            tc.cache_variables["RV_DEPS_WIN_PERL_ROOT"] = win_perl_location

        tc.cache_variables["CMAKE_BUILD_PARALLEL_LEVEL"] = str(os.cpu_count())

        # Make sure that CMAKE_BUILD_TYPE is specified in the cmake command that Conan use.
        # It seems needed on Windows for MS Visual Studio (multi-config generator).
        tc.cache_variables["CMAKE_BUILD_TYPE"] = str(self.settings.build_type)

        tc.cache_variables["CMAKE_FIND_PACKAGE_TARGETS_GLOBAL"] = "ON"

        # Enable find-first resolution: use Conan-provided packages via find_package
        # before falling back to building from source.
        tc.cache_variables["RV_DEPS_PREFER_INSTALLED"] = "ON"

        # Allow MINIMUM version matching for all converted dependencies so that
        # Conan-supplied versions (which may be newer than the pinned RV_DEPS_*
        # versions) are accepted by RV_FIND_DEPENDENCY.
        for dep in [
            "BOOST",
            "DAV1D",
            "OPENEXR",
            "IMATH",
            "ZLIB",
            "OPENSSL",
            "PNG",
            "TIFF",
            "JPEGTURBO",
            "RAW",
            "OPENJPEG",
            "OPENJPH",
        ]:
            tc.cache_variables[f"RV_DEPS_{dep}_VERSION_MATCH"] = "MINIMUM"

        # Set CMAKE_PREFIX_PATH to point to the generators folder
        # This allows ExternalProject_Add builds to find generated *Config.cmake files
        generators_folder = os.path.join(self.build_folder, "generators")
        tc.cache_variables["RV_CONAN_CMAKE_PREFIX_PATH"] = generators_folder

        # Generate the CMake's toolchain and preset files.
        tc.generate()

    def build(self):
        # NOTE: `pip install --user` fails inside virtualenvs:
        # "User site-packages are not visible in this virtualenv."
        in_venv = bool(os.getenv("VIRTUAL_ENV")) or (hasattr(sys, "base_prefix") and sys.prefix != sys.base_prefix)
        user_flag = [] if in_venv else ["--user"]
        command = " ".join(["python3", "-m", "pip", "install", *user_flag, "--upgrade", "-r", "requirements.txt"])
        if self.settings.os == "Windows":
            command = f"SETUPTOOLS_USE_DISTUTILS={self.setuptools_use_distutils} {command}"
        self.run(command, cwd=self.source_folder)

        cmake = CMake(self)
        cmake.configure()
        cmake.build(target="main_executable")


class PyReq(ConanFile):
    name = "openrvcore"
    version = "1.0.0"
    package_type = "python-require"
