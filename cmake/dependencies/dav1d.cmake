#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_DAV1D" "${RV_DEPS_DAV1D_VERSION}" "ninja" "meson")
RV_SHOW_STANDARD_DEPS_VARIABLES()

# --- Library naming (shared on all platforms, same filenames for find and build) ---
IF(RV_TARGET_DARWIN)
  SET(_david_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}dav1d.${RV_DEPS_DAV1D_VERSION_LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_david_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}dav1d${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_DAV1D_VERSION_LIB}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_david_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}dav1d${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

# --- Try to find installed package ---
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  dav1d
  VERSION
  ${RV_DEPS_DAV1D_VERSION}
  PKG_CONFIG_NAME
  dav1d
  DEPS_LIST_TARGETS
  dav1d::dav1d
)

# Compute full library paths AFTER RV_FIND_DEPENDENCY (which may override _lib_dir, _bin_dir)
IF(RV_TARGET_WINDOWS)
  SET(_dav1d_lib
      ${_bin_dir}/${_david_lib_name}
  )
  SET(_dav1d_implib_name
      dav1d${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_dav1d_implib
      ${_lib_dir}/${_dav1d_implib_name}
  )
ELSE()
  SET(_dav1d_lib
      ${_lib_dir}/${_david_lib_name}
  )
ENDIF()

# --- Build from source if not found ---
IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/dav1d.cmake)
ELSE()
  # Found via find_package or pkg-config — create proper imported target with LOCATION
  IF(NOT TARGET dav1d::dav1d)
    IF(RV_TARGET_WINDOWS)
      RV_ADD_IMPORTED_LIBRARY(
        NAME
        dav1d::dav1d
        TYPE
        SHARED
        LOCATION
        ${_dav1d_lib}
        IMPLIB
        ${_dav1d_implib}
        INCLUDE_DIRS
        ${_include_dir}
        DEPENDS
        ${_target}
      )
    ELSE()
      RV_ADD_IMPORTED_LIBRARY(
        NAME
        dav1d::dav1d
        TYPE
        SHARED
        LOCATION
        ${_dav1d_lib}
        SONAME
        ${_david_lib_name}
        INCLUDE_DIRS
        ${_include_dir}
        DEPENDS
        ${_target}
      )
    ENDIF()
  ENDIF()
ENDIF()

# --- Staging ---
IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_bin_dir} USE_FLAG_FILE)
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} USE_FLAG_FILE)
ENDIF()

# --- FFmpeg customization adding dav1d codec support ---
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_DEPENDS" RV_DEPS_DAV1D
)
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_EXTRA_C_OPTIONS" "--extra-cflags=-I${_include_dir}"
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS" "--extra-ldflags=-LIBPATH:${_lib_dir}"
  )
ELSE()
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS" "--extra-ldflags=-L${_lib_dir}"
  )
ENDIF()
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_EXTERNAL_LIBS" "--enable-libdav1d"
)
SET(RV_DEPS_DAVID_LIB_DIR
    ${_lib_dir}
    CACHE INTERNAL ""
)
