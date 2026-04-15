#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# libdeflate is a transitive runtime dependency of OpenEXR when built with deflate compression support (the default). It is only needed when provided by a
# package manager (Conan, Homebrew); the build-from-source OpenEXR path uses zlib's deflate internally and does not require a separate libdeflate.

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_LIBDEFLATE" "${RV_DEPS_LIBDEFLATE_VERSION}" "" "")

RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  libdeflate
  VERSION
  ${_version}
  DEPS_LIST_TARGETS
  libdeflate::libdeflate
)

IF(${_target}_FOUND)
  IF(NOT TARGET libdeflate::libdeflate)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      libdeflate::libdeflate
      TYPE
      SHARED
      LOCATION
      ${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}deflate${CMAKE_SHARED_LIBRARY_SUFFIX}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
    )
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS libdeflate::libdeflate)
ENDIF()
