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
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname})
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

  IF(RV_TARGET_DARWIN)
    # Found zlib may carry a versioned install name (e.g. libz.1.dylib from Homebrew, libz.1.3.1.dylib from MacPorts). Stage with the actual SONAME and create an
    # unversioned symlink when needed. The install name rewrite to @rpath is deferred to install time (remove_absolute_rpath.py).
    #
    # Resolve the actual library path, then fall back to ZLIB_LIBRARIES (always set by FindZLIB.cmake).
    RV_RESOLVE_IMPORTED_LOCATION(ZLIB::ZLIB _zlib_realpath)
    IF(NOT _zlib_realpath
       AND ZLIB_LIBRARIES
    )
      SET(_zlib_realpath
          "${ZLIB_LIBRARIES}"
      )
    ENDIF()

    # Resolve symlinks to get the real file, then extract SONAME from install name
    GET_FILENAME_COMPONENT(_zlib_realpath "${_zlib_realpath}" REALPATH)
    EXECUTE_PROCESS(
      COMMAND otool -D "${_zlib_realpath}"
      OUTPUT_VARIABLE _zlib_otool_out
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # otool -D output: first line is the file path, second line is the install name
    STRING(REGEX MATCH "[^\n]+$" _zlib_install_name "${_zlib_otool_out}")
    GET_FILENAME_COMPONENT(_zlib_soname "${_zlib_install_name}" NAME)

    # Only create the unversioned symlink if the SONAME is versioned (e.g. libz.1.dylib != libz.dylib). When they match (e.g. vcpkg/Conan providing libz.dylib
    # directly), skip to avoid a self-referencing symlink.
    SET(_zlib_symlink_cmd
        ""
    )
    IF(NOT "${_zlib_soname}" STREQUAL "${_libname}")
      SET(_zlib_symlink_cmd
          COMMAND ${CMAKE_COMMAND} -E create_symlink ${_zlib_soname} ${RV_STAGE_LIB_DIR}/${_libname}
      )
    ENDIF()

    ADD_CUSTOM_COMMAND(
      COMMENT "Staging ${_target}: ${_zlib_soname} -> ${RV_STAGE_LIB_DIR}"
      OUTPUT ${RV_STAGE_LIB_DIR}/${_zlib_soname}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${RV_STAGE_LIB_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_zlib_realpath}" "${RV_STAGE_LIB_DIR}/${_zlib_soname}" ${_zlib_symlink_cmd}
      DEPENDS ${_target}
    )
    ADD_CUSTOM_TARGET(
      ${_target}-stage-target ALL
      DEPENDS ${RV_STAGE_LIB_DIR}/${_zlib_soname}
    )
    ADD_DEPENDENCIES(dependencies ${_target}-stage-target)
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
