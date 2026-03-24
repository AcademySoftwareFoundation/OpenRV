#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMATH" "${RV_DEPS_IMATH_VERSION}" "" "")

# --- Library naming (shared on all platforms, same filenames for find and build) ---
IF(RV_TARGET_DARWIN)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}.${RV_DEPS_IMATH_LIB_VER}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_IMATH_LIB_VER}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

# --- Try to find installed package ---
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  Imath
  VERSION
  ${RV_DEPS_IMATH_VERSION}
  DEPS_LIST_TARGETS
  Imath::Imath
)

# Compute paths AFTER RV_FIND_DEPENDENCY (which may override _lib_dir)
SET(_libpath
    ${_lib_dir}/${_libname}
)

# When found via CONFIG, use the authoritative ${Imath_DIR} set by find_package (works for any layout: vcpkg share/, Homebrew lib/cmake/, etc.). For
# build-from-source, fall back to the conventional lib/cmake/Imath path.
IF(${_target}_FOUND
   AND DEFINED Imath_DIR
)
  SET(RV_DEPS_IMATH_CMAKE_DIR
      "${Imath_DIR}"
      CACHE STRING "Path to Imath CMake files ${_target}"
  )
  SET(RV_DEPS_IMATH_CMAKE_DIR
      "${Imath_DIR}"
  )
ELSE()
  SET(RV_DEPS_IMATH_CMAKE_DIR
      ${_lib_dir}/cmake/Imath
      CACHE STRING "Path to Imath CMake files ${_target}"
  )
  SET(RV_DEPS_IMATH_CMAKE_DIR
      ${_lib_dir}/cmake/Imath
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_implibpath
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
ENDIF()

# --- Build from source if not found ---
IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/imath.cmake)
ELSE()
  # CONFIG found — Imath::Imath target exists with proper LOCATION. For pkg-config (unlikely), create proper imported target.
  IF(NOT TARGET Imath::Imath)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      Imath::Imath
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
    )
  ENDIF()
ENDIF()

# --- Staging (shared — same library name for both paths) ---
RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})
