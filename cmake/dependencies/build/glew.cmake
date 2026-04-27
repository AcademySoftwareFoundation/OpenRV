#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build GLEW from source via ExternalProject_Add (Makefile-based build). Included by cmake/dependencies/glew.cmake when no installed package is found.
#
# Expects these variables from the caller (set by RV_CREATE_STANDARD_DEPS_VARIABLES): _target, _version, _install_dir, _lib_dir, _include_dir, _make_command,
# _cpu_count And these dep-specific variables from the caller: _glew_lib, _glew_lib_name

SET(_download_url
    "https://github.com/nigels-com/glew/archive/refs/tags/glew-${_version}.tar.gz"
)

SET(_download_hash
    ${RV_DEPS_GLEW_DOWNLOAD_HASH}
)

EXTERNALPROJECT_ADD(
  ${_target}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME glew-glew-${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  CONFIGURE_COMMAND cd auto && ${_make_command} && cd .. && ${_make_command}
  BUILD_COMMAND ${_make_command} -j${_cpu_count} GLEW_DEST=${_install_dir}
  INSTALL_COMMAND ${_make_command} install LIBDIR=${_lib_dir} GLEW_DEST=${_install_dir}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_glew_lib}
  USES_TERMINAL_BUILD TRUE
)

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
