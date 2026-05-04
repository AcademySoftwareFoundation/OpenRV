#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official sources: https://github.com/uclouvain/openjpeg
#
# Build instructions: https://github.com/uclouvain/openjpeg/blob/master/INSTALL.md
#

# version 2+ requires changes to IOjp2 project
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OPENJPEG" "${RV_DEPS_OPENJPEG_VERSION}" "make" "")

IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' since this CMake-based project isn't using lib64
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

# OpenJPEG ships CMake CONFIG files (OpenJPEGConfig.cmake). CONFIG creates target `openjp2` (not namespaced). Fall back to pkg-config.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  OpenJPEG
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  libopenjp2
  DEPS_LIST_TARGETS
  openjp2
)

SET(_download_url
    "https://github.com/uclouvain/openjpeg/archive/refs/tags/v${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_OPENJPEG_DOWNLOAD_HASH}
)

# Shared naming logic (used by both build and found paths)
IF(RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("openjp2" "" "SHARED" "")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("openjp2" "${RV_DEPS_OPENJPEG_VERSION}" "SHARED" "")
ENDIF()

SET(_openjpeg_include_dir
    "${_include_dir}/openjpeg-${_version_major}.${_version_minor}"
)

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/openjpeg.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

  IF(RV_TARGET_WINDOWS)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      OpenJpeg::OpenJpeg
      TYPE
      SHARED
      LOCATION
      ${_libpath}
      SONAME
      ${_libname}
      IMPLIB
      ${_bin_dir}/${_implibname}
      INCLUDE_DIRS
      ${_openjpeg_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ELSE()
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      OpenJpeg::OpenJpeg
      TYPE
      SHARED
      LOCATION
      ${_libpath}
      SONAME
      ${_libname}
      INCLUDE_DIRS
      ${_openjpeg_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ENDIF()
ELSE()
  # CONFIG creates `openjp2` target. Create `OpenJpeg::OpenJpeg` as an INTERFACE wrapper for backward compatibility (used by oiio.cmake). Only `openjp2` goes in
  # RV_DEPS_LIST (has LOCATION for install_name_tool -change); OpenJpeg::OpenJpeg is INTERFACE (no LOCATION).
  IF(NOT TARGET OpenJpeg::OpenJpeg)
    ADD_LIBRARY(OpenJpeg::OpenJpeg INTERFACE IMPORTED GLOBAL)
    TARGET_LINK_LIBRARIES(
      OpenJpeg::OpenJpeg
      INTERFACE openjp2
    )
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS openjp2)
ENDIF()
