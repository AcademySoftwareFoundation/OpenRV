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

# OpenImageIO was tested up to 1.2.1
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_WEBP" "${RV_DEPS_WEBP_VERSION}" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

IF(RV_USE_BREW_DEPS)
  FIND_PACKAGE(WebP CONFIG)
  IF(WebP_FOUND)
    MESSAGE(STATUS "Using Homebrew WebP")
    IF(TARGET WebP::webp)
      IF(NOT TARGET Webp::Webp)
        ADD_LIBRARY(Webp::Webp INTERFACE IMPORTED GLOBAL)
        TARGET_LINK_LIBRARIES(
          Webp::Webp
          INTERFACE WebP::webp
        )
      ENDIF()
    ELSE()
      FIND_LIBRARY(
        WEBP_LIBRARY
        NAMES webp
      )
      IF(WEBP_LIBRARY)
        ADD_LIBRARY(Webp::Webp UNKNOWN IMPORTED GLOBAL)
        SET_PROPERTY(
          TARGET Webp::Webp
          PROPERTY IMPORTED_LOCATION "${WEBP_LIBRARY}"
        )
        TARGET_INCLUDE_DIRECTORIES(
          Webp::Webp
          INTERFACE "${WebP_INCLUDE_DIRS}"
        )
      ENDIF()
    ENDIF()
    LIST(APPEND RV_DEPS_LIST Webp::Webp)
    GET_FILENAME_COMPONENT(_webp_root "${WebP_DIR}/../../.." ABSOLUTE)
    SET(RV_DEPS_WEBP_ROOT_DIR
        "${_webp_root}"
        CACHE INTERNAL "" FORCE
    )
    RETURN()
  ENDIF()
ENDIF()

SET(_download_url
    "https://github.com/webmproject/libwebp/archive/refs/tags/v${_version}.tar.gz"
)

SET(_download_hash
    ${RV_DEPS_WEBP_DOWNLOAD_HASH}
)

IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  # WebP lib on Darwin&Linux is built without a version number.
  RV_MAKE_STANDARD_LIB_NAME("webp" "" "STATIC" "")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("webp" "${_version}" "STATIC" "d")
ENDIF()

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(zlib_library ZLIB::ZLIB IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(zlib_library ZLIB::ZLIB IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${zlib_include_dir}")
LIST(APPEND _configure_options "-DZLIB_LIBRARY=${zlib_library}")

GET_TARGET_PROPERTY(_png_library PNG::PNG IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_png_include_dir PNG::PNG INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
LIST(APPEND _configure_options "-DPNG_PNG_INCLUDE_DIR=${_png_include_dir}")

IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(_jpeg_library libjpeg-turbo::jpeg IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(_jpeg_library libjpeg-turbo::jpeg IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(_jpeg_include_dir libjpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEG_LIBRARY=${_jpeg_library}")
LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${_jpeg_include_dir}")

IF(RV_TARGET_WINDOWS)
  # WebP is static so we can use the Static lib from Tiff
  GET_TARGET_PROPERTY(_tiff_library TIFF::TIFF IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(_tiff_library TIFF::TIFF IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(_tiff_include_dir TIFF::TIFF INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DTIFF_LIBRARY=${_tiff_library}")
LIST(APPEND _configure_options "-DTIFF_INCLUDE_DIR=${_tiff_include_dir}")

LIST(APPEND _configure_options "-DWEBP_BUILD_ANIM_UTILS=OFF")

# Do no build Webp tools.
LIST(APPEND _configure_options "-DWEBP_BUILD_GIF2WEBP=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_CWEBP=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_DWEBP=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_IMG2WEBP=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_VWEBP=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_WEBPINFO=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_WEBPMUX=OFF")
LIST(APPEND _configure_options "-DWEBP_BUILD_EXTRAS=OFF")

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
  DEPENDS ZLIB::ZLIB libjpeg-turbo::jpeg TIFF::TIFF PNG::PNG
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname})

RV_ADD_IMPORTED_LIBRARY(
  NAME
  WebP::webp
  TYPE
  STATIC
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
