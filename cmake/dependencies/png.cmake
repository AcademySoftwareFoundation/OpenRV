#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official source repository https://libpng.sourceforge.io/
#
# Some clone on GitHub https://github.com/glennrp/libpng
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_PNG" "${RV_DEPS_PNG_VERSION}" "" "")

# libpng ships CMake CONFIG files when built from source. CMake also provides a FindPNG module. Fall back to pkg-config.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  PNG
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  libpng16
  ALLOW_MODULE
  DEPS_LIST_TARGETS
  PNG::PNG
)

SET(_download_url
    "https://github.com/glennrp/libpng/archive/refs/tags/v${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_PNG_DOWNLOAD_HASH}
)

# Shared naming logic (used by both build and found paths)
SET(_libpng_lib_version
    "${_version_major}${_version_minor}.${_version_patch}.0"
)
IF(NOT RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("png${_version_major}${_version_minor}" "${_libpng_lib_version}" "SHARED" "d")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("libpng${_version_major}${_version_minor}" "${_libpng_lib_version}" "SHARED" "d")
ENDIF()

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/png.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

  IF(NOT RV_TARGET_WINDOWS)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      PNG::PNG
      TYPE
      SHARED
      LOCATION
      ${_libpath}
      SONAME
      ${_libname}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ELSE()
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      PNG::PNG
      TYPE
      SHARED
      LOCATION
      ${_implibpath}
      IMPLIB
      ${_implibpath}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ENDIF()
ELSE()
  IF(NOT TARGET PNG::PNG)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      PNG::PNG
      TYPE
      SHARED
      LOCATION
      ${_lib_dir}/${_libname}
      SONAME
      ${_libname}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
    )
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS PNG::PNG)
ENDIF()
