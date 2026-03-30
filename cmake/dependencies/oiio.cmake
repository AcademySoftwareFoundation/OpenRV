#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [OIIO -- Sources](https://github.com/OpenImageIO/oiio)
#
# [OIIO -- Documentation](https://openimageio.readthedocs.io/en/v${VERSION)NUMBER}/)
#
# [OIIO -- Build instructions](https://github.com/OpenImageIO/oiio/blob/master/INSTALL.md)
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OIIO" "${RV_DEPS_OIIO_VERSION}" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/OpenImageIO/archive/refs/tags/v${_version}.tar.gz"
)
SET(_download_hash
    "${RV_DEPS_OIIO_DOWNLOAD_HASH}"
)

RV_MAKE_STANDARD_LIB_NAME("OpenImageIO_Util" "${_oiio_ver_major_minor}" "SHARED" "${RV_DEBUG_POSTFIX}")
SET(_byprojects_copy
    ${_byproducts}
)
SET(_oiio_utils_libname
    ${_libname}
)
SET(_oiio_utils_libpath
    ${_libpath}
)
SET(_oiio_utils_implibpath
    ${_implibpath}
)

# Strips that last group from the version number and stores it in _oiio_ver_major_minor eg. 1.2.3.4 -> 1.2.3
STRING(
  REGEX
  REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+)\\..*" "\\1" _oiio_ver_major_minor ${RV_DEPS_OIIO_VERSION}
)

RV_MAKE_STANDARD_LIB_NAME("OpenImageIO" "${_oiio_ver_major_minor}" "SHARED" "${RV_DEBUG_POSTFIX}")
LIST(APPEND _byproducts ${_byprojects_copy})

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
LIST(APPEND _configure_options "-DBUILD_TESTING=OFF")
LIST(APPEND _configure_options "-DUSE_PYTHON=0") # this on would requireextra pybind11 package
LIST(APPEND _configure_options "-DUSE_OCIO=ON")
LIST(APPEND _configure_options "-DOpenColorIO_ROOT=${RV_DEPS_OCIO_ROOT_DIR}")
LIST(APPEND _configure_options "-DUSE_FREETYPE=0")
LIST(APPEND _configure_options "-DUSE_GIF=OFF")

# Write an initial-cache script so CMAKE_PREFIX_PATH (a semicolon-separated list) survives ExternalProject's double expansion of CONFIGURE_COMMAND args. Uses
# RV_DEPS_CMAKE_PREFIX_PATH (snapshot before Qt6 additions) to avoid passing ~150 Qt component dirs. Backslashes are converted to forward slashes to prevent
# escape issues in the generated CMake script.
SET(_oiio_initial_cache
    "${_build_dir}/_rv_initial_cache.cmake"
)
SET(_oiio_cache_content
    ""
)
IF(RV_DEPS_CMAKE_PREFIX_PATH)
  STRING(REPLACE "\\" "/" _oiio_clean_prefix "${RV_DEPS_CMAKE_PREFIX_PATH}")
  STRING(APPEND _oiio_cache_content "set(CMAKE_PREFIX_PATH \"${_oiio_clean_prefix}\" CACHE STRING \"\" FORCE)\n")
ENDIF()
# When deps come from a package manager (Conan), block find_package from searching the Homebrew shared prefix. OIIO's find_package(Boost) would otherwise find
# Homebrew's Boost headers (newer version), causing ABI mismatches with Conan's Boost library. CMAKE_IGNORE_PREFIX_PATH only affects find_package, not
# find_program (so Ninja and compilers still resolve). With Homebrew-only builds, all Boost is from the same source so no contamination occurs.
IF(RV_CONAN_CMAKE_PREFIX_PATH
   AND APPLE
)
  EXECUTE_PROCESS(
    COMMAND brew --prefix
    OUTPUT_VARIABLE _oiio_brew_prefix
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    RESULT_VARIABLE _oiio_brew_rc
  )
  IF(_oiio_brew_rc EQUAL 0
     AND _oiio_brew_prefix
  )
    STRING(APPEND _oiio_cache_content "set(CMAKE_IGNORE_PREFIX_PATH \"${_oiio_brew_prefix}\" CACHE STRING \"\" FORCE)\n")
  ENDIF()
ENDIF()
# NOTE: CMAKE_IGNORE_PREFIX_PATH is NOT set for Homebrew-only builds (no Conan). Although RV_DEPS_IGNORE_PREFIXES contains /opt/homebrew, blocking it would
# prevent OIIO from finding transitive deps (libdeflate, etc.) that only exist at the Homebrew prefix. The CMAKE_CXX_FLAGS -I workaround below handles Boost
# header contamination for the Homebrew path.
FILE(MAKE_DIRECTORY "${_build_dir}")
FILE(
  WRITE "${_oiio_initial_cache}"
  "${_oiio_cache_content}"
)
LIST(APPEND _configure_options "-C" "${_oiio_initial_cache}")

# Use explicit *_DIR variables pointing directly to config file directories for more precise package resolution. Use RV_DEPS_*_CMAKE_DIR which accounts for lib
# vs lib64 (RHEL) rather than hardcoding lib/.
LIST(APPEND _configure_options "-DBoost_ROOT=${RV_DEPS_BOOST_ROOT_DIR}")
LIST(APPEND _configure_options "-DOpenEXR_ROOT=${RV_DEPS_OPENEXR_ROOT_DIR}")
LIST(APPEND _configure_options "-DImath_DIR=${RV_DEPS_IMATH_CMAKE_DIR}")

# Use RV_RESOLVE_IMPORTED_LINKER_FILE to get the file the linker needs: IMPORTED_IMPLIB (.lib) on Windows, IMPORTED_LOCATION elsewhere. This avoids passing DLLs
# to -D<Pkg>_LIBRARY= which causes LNK1107 on MSVC. Also handles config-specific variants (e.g. IMPORTED_LOCATION_RELEASE). PNG::PNG may be an INTERFACE wrapper
# (vcpkg) where INTERFACE_INCLUDE_DIRECTORIES is on the underlying target (PNG::png_shared). Resolve through the chain for both library and include dirs. Guard
# all -D flags: passing -NOTFOUND values poisons OIIO's own FindPNG search. CMAKE_PREFIX_PATH (initial-cache) provides fallback discovery.
RV_RESOLVE_IMPORTED_LINKER_FILE(PNG::PNG _png_library)
RV_RESOLVE_IMPORTED_INCLUDE_DIR(PNG::PNG _png_include_dir)
IF(_png_library)
  LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
ENDIF()
IF(_png_include_dir)
  # INTERFACE_INCLUDE_DIRECTORIES may be a list; take only the first element for the cmake -D flag.
  LIST(GET _png_include_dir 0 _png_include_dir_first)
  LIST(APPEND _configure_options "-DPNG_PNG_INCLUDE_DIR=${_png_include_dir_first}")
ENDIF()

RV_RESOLVE_IMPORTED_LINKER_FILE(libjpeg-turbo::jpeg _jpeg_library)
RV_RESOLVE_IMPORTED_INCLUDE_DIR(libjpeg-turbo::jpeg _jpeg_include_dir)
IF(_jpeg_library)
  LIST(APPEND _configure_options "-DJPEG_LIBRARY=${_jpeg_library}")
ENDIF()
IF(_jpeg_include_dir)
  LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${_jpeg_include_dir}")
ENDIF()

RV_RESOLVE_IMPORTED_LINKER_FILE(libjpeg-turbo::turbojpeg _jpegturbo_library)
RV_RESOLVE_IMPORTED_INCLUDE_DIR(libjpeg-turbo::turbojpeg _jpegturbo_include_dir)
IF(_jpegturbo_library)
  LIST(APPEND _configure_options "-DJPEGTURBO_LIBRARY=${_jpegturbo_library}")
ENDIF()
IF(_jpegturbo_include_dir)
  LIST(APPEND _configure_options "-DJPEGTURBO_INCLUDE_DIR=${_jpegturbo_include_dir}")
ENDIF()

LIST(APPEND _configure_options "-DOpenJPEG_ROOT=${RV_DEPS_OPENJPEG_ROOT_DIR}")
LIST(APPEND _configure_options "-DOPENJPEG_VERSION=${RV_DEPS_OPENJPEG_VERSION}")
# Use openjp2 target (the actual IMPORTED library). OpenJpeg::OpenJpeg is an INTERFACE wrapper when found via CONFIG.
RV_RESOLVE_IMPORTED_LINKER_FILE(openjp2 _openjpeg_library)
IF(NOT _openjpeg_library)
  # Build-from-source path: openjp2 target doesn't exist, use OpenJpeg::OpenJpeg
  RV_RESOLVE_IMPORTED_LINKER_FILE(OpenJpeg::OpenJpeg _openjpeg_library)
ENDIF()
IF(TARGET openjp2)
  RV_RESOLVE_IMPORTED_INCLUDE_DIR(openjp2 _openjpeg_include_dir)
ELSE()
  RV_RESOLVE_IMPORTED_INCLUDE_DIR(OpenJpeg::OpenJpeg _openjpeg_include_dir)
ENDIF()
IF(_openjpeg_library)
  LIST(APPEND _configure_options "-DOPENJPEG_OPENJP2_LIBRARY=${_openjpeg_library}")
ENDIF()
IF(_openjpeg_include_dir)
  LIST(APPEND _configure_options "-DOPENJPEG_INCLUDE_DIR=${_openjpeg_include_dir}")
ENDIF()

LIST(APPEND _configure_options "-DTIFF_ROOT=${RV_DEPS_TIFF_ROOT_DIR}")

LIST(APPEND _configure_options "-DUSE_FFMPEG=0")

# When Boost is built from source but other deps come from a shared prefix (e.g. Homebrew), their transitive -isystem includes can pull in a newer system
# Boost's headers, causing ABI mismatches at link time. Adding Boost's include as a non-system -I flag ensures it takes precedence over any -isystem paths,
# since compilers (GCC/Clang) always search -I before -isystem.
IF(RV_DEPS_IGNORE_PREFIXES)
  LIST(APPEND _configure_options "-DCMAKE_CXX_FLAGS=-I${RV_DEPS_BOOST_ROOT_DIR}/include")
ENDIF()

IF(RV_TARGET_LINUX)
  MESSAGE(STATUS "Building OpenImageIO using system's freetype library.")
  SET(_depends_freetype
      ""
  )
ELSE()
  SET(_depends_freetype
      freetype
  )
  RV_RESOLVE_IMPORTED_LINKER_FILE(freetype _freetype_library)
  RV_RESOLVE_IMPORTED_INCLUDE_DIR(freetype _freetype_include_dir)
  IF(_freetype_library)
    LIST(APPEND _configure_options "-DFREETYPE_LIBRARY=${_freetype_library}")
  ENDIF()
  IF(_freetype_include_dir)
    LIST(APPEND _configure_options "-DFREETYPE_INCLUDE_DIR=${_freetype_include_dir}")
  ENDIF()
  MESSAGE(DEBUG "OIIO: _freetype_library='${_freetype_library}'")
  MESSAGE(DEBUG "OIIO: _freetype_include_dir='${_freetype_include_dir}'")
ENDIF()

# LibRaw_ROOT alone is insufficient when vcpkg puts the import library in lib/manual-link/. OIIO's FindLibRaw.cmake uses LIBRAW_LIBDIR_HINT and
# LIBRAW_INCLUDEDIR_HINT as find_library/find_path HINTS. Pass the directory containing the library file and the include parent directory.
LIST(APPEND _configure_options "-DLibRaw_ROOT=${RV_DEPS_RAW_ROOT_DIR}")
RV_RESOLVE_IMPORTED_LINKER_FILE(libraw::raw _raw_library)
RV_RESOLVE_IMPORTED_INCLUDE_DIR(libraw::raw _raw_include_dir)
IF(_raw_library)
  GET_FILENAME_COMPONENT(_raw_lib_dir "${_raw_library}" DIRECTORY)
  LIST(APPEND _configure_options "-DLIBRAW_LIBDIR_HINT=${_raw_lib_dir}")
ENDIF()
IF(_raw_include_dir)
  # OIIO's find_path uses PATH_SUFFIXES libraw, so pass the parent of the libraw/ include dir.
  STRING(
    REGEX
    REPLACE "/libraw$" "" _raw_include_dir_parent "${_raw_include_dir}"
  )
  LIST(APPEND _configure_options "-DLIBRAW_INCLUDEDIR_HINT=${_raw_include_dir_parent}")
ENDIF()

IF(NOT RV_TARGET_LINUX)
  LIST(APPEND _configure_options "-DWebP_ROOT=${RV_DEPS_WEBP_ROOT_DIR}")
  # Linux has a Link error related to relocation; WebP appears not built with -fPIC. Hence OIIO will build WebP itself.
ENDIF()
LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")

# OIIO tools are not needed.
LIST(APPEND _configure_options "-DOIIO_BUILD_TOOLS=OFF" "-DOIIO_BUILD_TESTS=OFF")

LIST(PREPEND _configure_options "-G ${CMAKE_GENERATOR}")
IF(RV_TARGET_WINDOWS)
  LIST(APPEND _configure_options "-DCMAKE_CXX_FLAGS=/utf-8")
ENDIF()

IF(NOT RV_TARGET_WINDOWS)
  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    BINARY_DIR ${_build_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS ${_depends_freetype}
            libjpeg-turbo::jpeg
            TIFF::TIFF
            OpenEXR::OpenEXR
            OpenJpeg::OpenJpeg
            libjpeg-turbo::turbojpeg
            PNG::PNG
            Boost::headers
            Boost::thread
            Boost::filesystem
            Imath::Imath
            WebP::webp
            LibRaw::raw
            ZLIB::ZLIB
            OpenColorIO::OpenColorIO
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${_cmake_build_command}
    INSTALL_COMMAND ${_cmake_install_command}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ELSE()
  LIST(
    APPEND
    _oiio_build_options
    "--build"
    "${_build_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}"
    "--parallel"
    "${_cpu_count}"
  )
  LIST(
    APPEND
    _oiio_install_options
    "--install"
    "${_build_dir}"
    "--prefix"
    "${_install_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}" # for --config
  )

  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    BINARY_DIR ${_build_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS ${_depends_freetype}
            libjpeg-turbo::jpeg
            TIFF::TIFF
            OpenEXR::OpenEXR
            OpenJpeg::OpenJpeg
            libjpeg-turbo::turbojpeg
            PNG::PNG
            Boost::headers
            Boost::thread
            Boost::filesystem
            Imath::Imath
            WebP::webp
            LibRaw::raw
            ZLIB::ZLIB
            OpenColorIO::OpenColorIO
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${CMAKE_COMMAND} ${_oiio_build_options}
    INSTALL_COMMAND ${CMAKE_COMMAND} ${_oiio_install_options}
    BUILD_IN_SOURCE FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ENDIF()

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

RV_ADD_IMPORTED_LIBRARY(
  NAME
  OpenImageIO::OpenImageIO
  TYPE
  SHARED
  LOCATION
  ${_libpath}
  SONAME
  ${_libname}
  IMPLIB
  ${_implibpath}
  INCLUDE_DIRS
  ${_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)

RV_ADD_IMPORTED_LIBRARY(
  NAME
  OpenImageIO::OpenImageIO_Util
  TYPE
  SHARED
  LOCATION
  ${_oiio_utils_libpath}
  SONAME
  ${_oiio_utils_libname}
  IMPLIB
  ${_oiio_utils_implibpath}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)
