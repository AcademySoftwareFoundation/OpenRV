#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# The patch comes from the ZLIB port in Vcpkg repository. The name of the patch is kept as is. See https://github.com/microsoft/vcpkg/tree/master/ports/zlib
# Description: Fix unistd.h being incorrectly required when imported from a project defining HAVE_UNISTD_H=0
SET(_patch_command
    patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/zconf.h.cmakein_prevent_invalid_inclusions.patch && patch -p1 <
    ${CMAKE_CURRENT_SOURCE_DIR}/patch/zconf.h.in_prevent_invalid_inclusions.patch
)

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  PATCH_COMMAND ${_patch_command}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_zlib_byproducts}
  USES_TERMINAL_BUILD TRUE
)

IF(RV_TARGET_WINDOWS)
  # FFmpeg expects "zlib" in Release and Debug.
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Renaming the ZLIB import debug lib to the name FFmpeg is expecting (release name)"
      COMMAND ${CMAKE_COMMAND} -E copy ${_implibpath} ${_lib_dir}/zlib.lib
    )
  ENDIF()
ENDIF()

RV_ADD_IMPORTED_LIBRARY(
  NAME
  ZLIB::ZLIB
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
