#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [libtiff 3.6 -- Old libtiff Webpage](http://www.libtiff.org/)
#
# [libtiff 3.9-4.5](https://download.osgeo.org/libtiff/)
#
# [libtiff 4.4](https://conan.io/center/libtiff)
#
# [libtiff 4.5](http://www.simplesystems.org/libtiff)
#

# OpenImageIO required >= 3.9, using latest 4.0
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_TIFF" "${RV_DEPS_TIFF_VERSION}" "" "")

# Force build from source: OpenRV uses private TIFF headers (tiffiop.h, tif_dir.h, tif_hash_set.h) that are not installed by system packages.
SET(RV_DEPS_TIFF_FORCE_BUILD
    ON
    CACHE BOOL "libtiff must be built from source (private headers required)" FORCE
)

# libtiff ships CMake CONFIG files when built from source. CMake also provides a FindTIFF module. Fall back to pkg-config.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  TIFF
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  libtiff-4
  ALLOW_MODULE
  DEPS_LIST_TARGETS
  TIFF::TIFF
)

SET(_download_url
    "https://gitlab.com/libtiff/libtiff/-/archive/v${_version}/libtiff-v${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_TIFF_DOWNLOAD_HASH}
)

# Shared naming logic (used by both build and found paths)
IF(NOT RV_TARGET_WINDOWS)
  IF(RV_TARGET_DARWIN)
    RV_MAKE_STANDARD_LIB_NAME("tiff" "${RV_DEPS_TIFF_VERSION_LIB}" "SHARED" "")
  ELSE()
    RV_MAKE_STANDARD_LIB_NAME("tiff" "" "SHARED" "")
  ENDIF()
ELSE()
  # Windows: TIFF produces tiff.dll/tiffd.dll (not libtiff.dll). Debug uses "d" postfix.
  RV_MAKE_STANDARD_LIB_NAME("tiff" "${_version}" "SHARED" "d")
ENDIF()

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/tiff.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

  IF(RV_TARGET_WINDOWS)
    IF(${CMAKE_BUILD_TYPE} STREQUAL "Release")
      SET(_tiff_lib_name
          "tiff.lib"
      )
    ELSEIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      SET(_tiff_lib_name
          "tiffd.lib"
      )
    ENDIF()
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      TIFF::TIFF
      TYPE
      SHARED
      LOCATION
      ${_libpath}
      IMPLIB
      ${_lib_dir}/${_tiff_lib_name}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ELSE()
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      TIFF::TIFF
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
  ENDIF()

  TARGET_LINK_LIBRARIES(
    TIFF::TIFF
    INTERFACE ZLIB::ZLIB libjpeg-turbo::jpeg
  )
ELSE()
  # FindTIFF.cmake (MODULE) creates TIFF::TIFF as INTERFACE wrapping TIFF::tiff. CONFIG creates TIFF::TIFF directly. For staging, use TIFF::tiff if it exists
  # (the actual IMPORTED library with LOCATION), otherwise TIFF::TIFF.
  IF(TARGET TIFF::tiff)
    SET(_tiff_stage_target
        TIFF::tiff
    )
    # TIFF::tiff needs to be in RV_DEPS_LIST for install_name_tool -change to work (it has LOCATION, TIFF::TIFF is INTERFACE).
    LIST(APPEND RV_DEPS_LIST TIFF::tiff)
    RV_RESOLVE_DARWIN_INSTALL_NAME(TIFF::tiff)
  ELSE()
    SET(_tiff_stage_target
        TIFF::TIFF
    )
  ENDIF()

  IF(NOT TARGET TIFF::TIFF)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      TIFF::TIFF
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

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS ${_tiff_stage_target})
ENDIF()
