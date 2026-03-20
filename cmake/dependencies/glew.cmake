#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_GLEW" "${RV_DEPS_GLEW_VERSION}" "make" "")

# --- Try to find installed package ---
# GLEW's CMake config does not ship a version file, so omit VERSION to avoid find_package failure.
RV_FIND_DEPENDENCY(TARGET ${_target} PACKAGE GLEW DEPS_LIST_TARGETS GLEW::GLEW)

# --- Library naming (shared between find and build paths) ---
IF(RV_TARGET_DARWIN)
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}GLEW.${RV_DEPS_GLEW_VERSION_LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}GLEW${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_GLEW_VERSION_LIB}
  )
ENDIF()
SET(_glew_lib
    ${_lib_dir}/${_glew_lib_name}
)

# --- Build from source if not found ---
IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/glew.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_glew_lib_name})
ELSE()
  # CONFIG found — GLEW::GLEW target already exists (created by glew-config.cmake from GLEW::glew).
  IF(NOT TARGET GLEW::GLEW)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      GLEW::GLEW
      TYPE
      SHARED
      LOCATION
      ${_glew_lib}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ENDIF()

  # Found path: use TARGET_LIBS to resolve actual library path at build time
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS GLEW::GLEW)
ENDIF()
