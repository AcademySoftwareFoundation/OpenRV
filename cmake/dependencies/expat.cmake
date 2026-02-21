#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official source repository https://github.com/libexpat/libexpat
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_EXPAT" "${RV_DEPS_EXPAT_VERSION}" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

STRING(REPLACE "." "_" _version_underscored ${_version})
SET(_download_url
    "https://github.com/libexpat/libexpat/archive/refs/tags/R_${_version_underscored}.tar.gz"
)

SET(_download_hash
    ${RV_DEPS_EXPAT_DOWNLOAD_HASH}
)

SET(_libexpat_lib_version
    ${RV_DEPS_EXPAT_VERSION}
)

RV_MAKE_STANDARD_LIB_NAME("libexpat" "" "SHARED" "d")

# Remove the -S argument from _configure_options, and adjust the path for Expat.
LIST(REMOVE_ITEM _configure_options "-S ${_source_dir}")
# Expat source is under expat folder and not directly under repository root.
LIST(APPEND _configure_options "-S ${_source_dir}/expat")

LIST(APPEND _configure_options "-DEXPAT_BUILD_DOCS=OFF")
LIST(APPEND _configure_options "-DEXPAT_BUILD_EXAMPLES=OFF")
LIST(APPEND _configure_options "-DEXPAT_BUILD_TESTS=OFF")
LIST(APPEND _configure_options "-DEXPAT_BUILD_TOOLS=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

RV_ADD_IMPORTED_LIBRARY(
  NAME EXPAT::EXPAT
  TYPE SHARED
  LOCATION ${_libpath}
  IMPLIB ${_implibpath}
  INCLUDE_DIRS ${_include_dir}
  DEPENDS ${_target}
  ADD_TO_DEPS_LIST
)
