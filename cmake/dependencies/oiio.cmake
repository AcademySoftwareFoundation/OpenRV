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

# Propagate CMAKE_PREFIX_PATH and CMAKE_IGNORE_PREFIX_PATH so the sub-build can find transitive dependencies (e.g. libdeflate for OpenEXR) and avoids
# contaminating prefixes (e.g. msys64/mingw64).
IF(CMAKE_PREFIX_PATH)
  LIST(APPEND _configure_options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
ENDIF()
IF(RV_DEPS_IGNORE_PREFIXES)
  LIST(APPEND _configure_options "-DCMAKE_IGNORE_PREFIX_PATH=${RV_DEPS_IGNORE_PREFIXES}")
ENDIF()

# Use explicit *_DIR variables pointing directly to config file directories for more precise package resolution. Use RV_DEPS_*_CMAKE_DIR which accounts for lib
# vs lib64 (RHEL) rather than hardcoding lib/.
LIST(APPEND _configure_options "-DBoost_DIR=${RV_DEPS_BOOST_ROOT_DIR}/lib/cmake/Boost-${RV_DEPS_BOOST_VERSION}")
LIST(APPEND _configure_options "-DOpenEXR_ROOT=${RV_DEPS_OPENEXR_ROOT_DIR}")
LIST(APPEND _configure_options "-DImath_DIR=${RV_DEPS_IMATH_CMAKE_DIR}")

# Use RV_RESOLVE_IMPORTED_LINKER_FILE to get the file the linker needs: IMPORTED_IMPLIB (.lib) on Windows, IMPORTED_LOCATION elsewhere. This avoids passing DLLs
# to -D<Pkg>_LIBRARY= which causes LNK1107 on MSVC. Also handles config-specific variants (e.g. IMPORTED_LOCATION_RELEASE).
RV_RESOLVE_IMPORTED_LINKER_FILE(PNG::PNG _png_library)
GET_TARGET_PROPERTY(_png_include_dir PNG::PNG INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
# INTERFACE_INCLUDE_DIRECTORIES may be a list; take only the first element for the cmake -D flag.
LIST(GET _png_include_dir 0 _png_include_dir_first)
LIST(APPEND _configure_options "-DPNG_PNG_INCLUDE_DIR=${_png_include_dir_first}")

RV_RESOLVE_IMPORTED_LINKER_FILE(libjpeg-turbo::jpeg _jpeg_library)
GET_TARGET_PROPERTY(_jpeg_include_dir libjpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEG_LIBRARY=${_jpeg_library}")
LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${_jpeg_include_dir}")

RV_RESOLVE_IMPORTED_LINKER_FILE(libjpeg-turbo::turbojpeg _jpegturbo_library)
GET_TARGET_PROPERTY(_jpegturbo_include_dir libjpeg-turbo::turbojpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEGTURBO_LIBRARY=${_jpegturbo_library}")
LIST(APPEND _configure_options "-DJPEGTURBO_INCLUDE_DIR=${_jpegturbo_include_dir}")

LIST(APPEND _configure_options "-DOpenJPEG_ROOT=${RV_DEPS_OPENJPEG_ROOT_DIR}")
LIST(APPEND _configure_options "-DOPENJPEG_VERSION=${RV_DEPS_OPENJPEG_VERSION}")
# Use openjp2 target (the actual IMPORTED library). OpenJpeg::OpenJpeg is an INTERFACE wrapper when found via CONFIG.
RV_RESOLVE_IMPORTED_LINKER_FILE(openjp2 _openjpeg_library)
IF(NOT _openjpeg_library)
  # Build-from-source path: openjp2 target doesn't exist, use OpenJpeg::OpenJpeg
  RV_RESOLVE_IMPORTED_LINKER_FILE(OpenJpeg::OpenJpeg _openjpeg_library)
ENDIF()
IF(TARGET openjp2)
  GET_TARGET_PROPERTY(_openjpeg_include_dir openjp2 INTERFACE_INCLUDE_DIRECTORIES)
ELSE()
  GET_TARGET_PROPERTY(_openjpeg_include_dir OpenJpeg::OpenJpeg INTERFACE_INCLUDE_DIRECTORIES)
ENDIF()
LIST(APPEND _configure_options "-DOPENJPEG_OPENJP2_LIBRARY=${_openjpeg_library}")
LIST(APPEND _configure_options "-DOPENJPEG_INCLUDE_DIR=${_openjpeg_include_dir}")

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
  GET_TARGET_PROPERTY(_freetype_library freetype IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_freetype_include_dir freetype INTERFACE_INCLUDE_DIRECTORIES)
  LIST(APPEND _configure_options "-DFREETYPE_LIBRARY=${_freetype_library}")
  LIST(APPEND _configure_options "-DFREETYPE_INCLUDE_DIR=${_freetype_include_dir}")
  MESSAGE(DEBUG "OIIO: _freetype_library='${_freetype_library}'")
  MESSAGE(DEBUG "OIIO: _freetype_include_dir='${_freetype_include_dir}'")
ENDIF()

LIST(APPEND _configure_options "-DLibRaw_ROOT=${RV_DEPS_RAW_ROOT_DIR}")

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
