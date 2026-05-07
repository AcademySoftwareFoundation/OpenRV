#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_ZLIB" "${RV_DEPS_ZLIB_VERSION}" "" "" FORCE_LIB)

# zlib has no CMake CONFIG files, so ALLOW_MODULE lets the built-in FindZLIB.cmake create ZLIB::ZLIB.  Also falls back to pkg-config (useful if ZLIB_ROOT points
# to a keg-only Homebrew cellar).
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  ZLIB
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  zlib
  ALLOW_MODULE
  DEPS_LIST_TARGETS
  ZLIB::ZLIB
)

SET(_download_url
    "https://github.com/madler/zlib/archive/refs/tags/v${_version}.zip"
)
SET(_download_hash
    ${RV_DEPS_ZLIB_DOWNLOAD_HASH}
)

# Shared naming logic (used by both build and found paths for staging)
IF(RV_TARGET_WINDOWS)
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_zlibname
        "zlibd"
    )
  ELSE()
    SET(_zlibname
        "zlib"
    )
  ENDIF()
ELSE()
  SET(_zlibname
      "z"
  )
ENDIF()

SET(_libname
    ${CMAKE_SHARED_LIBRARY_PREFIX}${_zlibname}${CMAKE_SHARED_LIBRARY_SUFFIX}
)
SET(_libpath
    ${_lib_dir}/${_libname}
)

IF(RV_TARGET_WINDOWS)
  SET(_libpath
      ${_bin_dir}/${_libname}
  )
ENDIF()

LIST(APPEND _zlib_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibname
      ${CMAKE_IMPORT_LIBRARY_PREFIX}${_zlibname}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_implibpath
      ${_lib_dir}/${_implibname}
  )
  LIST(APPEND _zlib_byproducts ${_implibpath})
ENDIF()

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/zlib.cmake)

  IF(RV_TARGET_WINDOWS)
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_bin_dir} OUTPUTS ${RV_STAGE_BIN_DIR}/${_libname})
  ELSEIF(RV_TARGET_DARWIN)
    RV_STAGE_DEPENDENCY_LIBS(
      TARGET
      ${_target}
      OUTPUTS
      ${RV_STAGE_LIB_DIR}/${_libname}
      PRE_COMMANDS
      COMMAND
      ${CMAKE_INSTALL_NAME_TOOL}
      -id
      "@rpath/${_libname}"
      "${_lib_dir}/${_libname}"
    )
  ELSE()
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname})
  ENDIF()
ELSE()
  IF(NOT TARGET ZLIB::ZLIB)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      ZLIB::ZLIB
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

  IF(RV_TARGET_WINDOWS)
    # Conan's zlib names the DLL zlib1.dll but the import lib zdll.lib. conan_package_library_targets can't match them, creating an UNKNOWN target whose
    # IMPORTED_LOCATION is the .lib.  TARGET_LIBS-based staging would copy the .lib instead of the DLL.  Fall back to BIN_DIR staging.
    FILE(GLOB _found_zlib_dlls "${_bin_dir}/zlib*.dll")
    IF(_found_zlib_dlls)
      LIST(GET _found_zlib_dlls 0 _found_zlib_dll)
      GET_FILENAME_COMPONENT(_found_zlib_dll_name "${_found_zlib_dll}" NAME)
      RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_bin_dir} OUTPUTS ${RV_STAGE_BIN_DIR}/${_found_zlib_dll_name})
    ELSE()
      RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS ZLIB::ZLIB)
    ENDIF()
  ELSE()
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS ZLIB::ZLIB)
  ENDIF()
ENDIF()

SET(RV_DEPS_ZLIB_INCLUDE_DIR
    ${_include_dir}
    CACHE STRING "Path to installed includes for ${_target}"
)

# FFmpeg customization
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_DEPENDS" RV_DEPS_ZLIB
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
