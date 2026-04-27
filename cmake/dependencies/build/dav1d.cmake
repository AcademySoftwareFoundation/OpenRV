#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build dav1d from source via ExternalProject_Add. Included by cmake/dependencies/dav1d.cmake when no installed package is found.
#
# Expects these variables from the caller (set by RV_CREATE_STANDARD_DEPS_VARIABLES): _target, _version, _source_dir, _install_dir, _lib_dir, _bin_dir,
# _include_dir, _configure_command, _make_command And these dep-specific variables from the caller: _dav1d_lib_name, _dav1d_lib
#

SET(_download_url
    "https://github.com/videolan/dav1d/archive/refs/tags/${_version}.zip"
)
SET(_download_hash
    ${RV_DEPS_DAV1D_DOWNLOAD_HASH}
)

# _lib_dir_name is needed for the meson --libdir option
IF(RHEL_VERBOSE)
  SET(_lib_dir_name
      lib64
  )
ELSE()
  SET(_lib_dir_name
      lib
  )
ENDIF()

IF(APPLE)
  # Cross-file must be specified because if Rosetta is used to compile for x86_64 from ARM64, Meson still detects ARM64 as the default architecture.
  IF(RV_TARGET_APPLE_X86_64)
    SET(_meson_cross_file
        "${PROJECT_SOURCE_DIR}/src/build/meson_arch_x86_64.txt"
    )
  ELSEIF(RV_TARGET_APPLE_ARM64)
    SET(_meson_cross_file
        "${PROJECT_SOURCE_DIR}/src/build/meson_arch_arm64.txt"
    )
  ENDIF()

  SET(_configure_command
      ${_configure_command} "--cross-file" ${_meson_cross_file}
  )
ENDIF()

SET(_default_library
    shared
)

EXTERNALPROJECT_ADD(
  ${_target}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${_configure_command} ./_build --libdir=${_lib_dir_name} --default-library=${_default_library} --prefix=${_install_dir} -Denable_tests=false
                    -Denable_tools=false
  BUILD_COMMAND ${_make_command} -C _build
  INSTALL_COMMAND ${_make_command} -C _build install
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_dav1d_lib}
  USES_TERMINAL_BUILD TRUE
)

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
    ADD_TO_DEPS_LIST
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
    ${_dav1d_lib_name}
    INCLUDE_DIRS
    ${_include_dir}
    DEPENDS
    ${_target}
    ADD_TO_DEPS_LIST
  )
ENDIF()
