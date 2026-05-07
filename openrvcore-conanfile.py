import os
import sys
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, cmake_layout, CMakeToolchain
from conan.tools.gnu import PkgConfigDeps
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
        # Versions and options are read from conan/packages.yml, which is the
        # single source of truth for the Conan build path. To bump a dep:
        # edit packages.yml, then run conan/scripts/gen_profile_common.py.
        #
        # libtiff is intentionally absent: it is force-built from source in
        # CMake (private headers needed) and excluded from Conan because
        # libtiff 4.6.0 doesn't build against libjpeg-turbo's 12-bit API.
        # openjph/openexr pull it in transitively; disabled via
        # openjph:with_tiff=False in packages.yml.
        import yaml

        packages_yml = os.path.join(self.recipe_folder, "conan", "packages.yml")
        pkgs = yaml.safe_load(open(packages_yml))

        for dep in pkgs["deps"]:
            # Mirror of conan/scripts/_common.py:is_active_dep. Conan's
            # recipe sandbox makes importing helper modules fragile, so
            # the rule is duplicated here. Update both sites together.
            if dep.get("windows_only") and self.settings.os != "Windows":
                continue
            # Strip the "name/*:" pattern prefix from option keys to get bare names.
            opts = {}
            for k, v in (dep.get("options") or {}).items():
                bare = k.split(":")[-1]
                # YAML booleans are already Python bools; strings stay strings.
                opts[bare] = v
            self.requires(f"{dep['name']}/{dep['version']}", force=True, options=opts)

    def generate(self):
        buildenv = VirtualBuildEnv(self)
        buildenv.generate()

        if self.settings.os == "Windows":
            ms = VCVars(self)
            ms.generate()

        deps = CMakeDeps(self)
        # OIIO 3.x links openjph via $<TARGET_NAME_IF_EXISTS:openjph> which
        # requires a non-namespaced target. CMakeDeps defaults to openjph::openjph.
        deps.set_property("openjph", "cmake_target_name", "openjph")
        deps.generate()

        # Generate pkg-config .pc files so that ExternalProject builds (FFmpeg)
        # can discover Conan-provided shared libraries like dav1d via pkg-config.
        pc = PkgConfigDeps(self)
        pc.generate()

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

        # VFX Platform: controlled by the vfx_platform option in the main conanfile.py.
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
            "LIBDEFLATE",
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
