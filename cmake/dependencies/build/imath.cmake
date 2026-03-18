#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build Imath from source via ExternalProject_Add. Included by cmake/dependencies/imath.cmake when no installed package is found.
#
# Expects these variables from the caller (set by RV_CREATE_STANDARD_DEPS_VARIABLES): _target, _version, _install_dir, _configure_options, _cmake_build_command,
# _cmake_install_command And these dep-specific variables from the caller: _libname, _libpath, _implibpath (Windows only)
#

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    "${RV_DEPS_IMATH_DOWNLOAD_HASH}"
)

# Override include dir to Imath subdirectory for build-from-source
SET(_include_dir
    ${_install_dir}/include/Imath
)

LIST(APPEND _imath_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  LIST(APPEND _imath_byproducts ${_implibpath})
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_imath_byproducts}
  USES_TERMINAL_BUILD TRUE
)

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
  ADD_TO_DEPS_LIST
)
