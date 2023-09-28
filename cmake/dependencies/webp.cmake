#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# WebP official Web page:  -- https://www.webmproject.org
#
# WebP official sources:   -- https://chromium.googlesource.com/webm/libwebp
#
# WebP build from sources: -- https://github.com/webmproject/libwebp/blob/main/doc/building.md
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

# OpenImageIO was tested up to 1.2.1
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_WEBP" "1.2.1" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/webmproject/libwebp/archive/refs/tags/v${_version}.tar.gz"
)

SET(_download_hash
    "ef5ac6de4b800afaebeb10df9ef189b2"
)

IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  # WebP lib on Darwin&Linux is built without a version number.
  RV_MAKE_STANDARD_LIB_NAME("webp" "" "STATIC" "d")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("webp" "${_version}" "STATIC" "d")
ENDIF()

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
GET_TARGET_PROPERTY(_zlib_library ZLIB::ZLIB IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DZLIB_LIBRARY=${_zlib_library}")
LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${_zlib_include_dir}")

GET_TARGET_PROPERTY(_png_library PNG::PNG IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_png_include_dir PNG::PNG INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
LIST(APPEND _configure_options "-DPNG_INCLUDE_DIR=${_png_include_dir}")

IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(_jpeg_library jpeg-turbo::jpeg IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(_jpeg_library jpeg-turbo::jpeg IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(_jpeg_include_dir jpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEG_LIBRARY=${_jpeg_library}")
LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${_jpeg_include_dir}")

IF(RV_TARGET_WINDOWS)
  # WebP is static so we can use the Static lib from Tiff
  GET_TARGET_PROPERTY(_tiff_library Tiff::Tiff IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(_tiff_library Tiff::Tiff IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(_tiff_include_dir Tiff::Tiff INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DTIFF_LIBRARY=${_tiff_library}")
LIST(APPEND _configure_options "-DTIFF_INCLUDE_DIR=${_tiff_include_dir}")

LIST(APPEND _configure_options "-DWEBP_BUILD_GIF2WEBP=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_ANIM_UTILS=OFF")

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
  DEPENDS ZLIB::ZLIB jpeg-turbo::jpeg Tiff::Tiff PNG::PNG
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_make_command} -j${_cpu_count} -v
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(Webp::Webp STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(Webp::Webp ${_target})
SET_PROPERTY(
  TARGET Webp::Webp
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET Webp::Webp
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET Webp::Webp
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  Webp::Webp
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST Webp::Webp)
