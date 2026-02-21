#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_SPDLOG" "${RV_DEPS_SPDLOG_VERSION}" "" "")

SET(_download_url
    "https://github.com/gabime/spdlog/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    ${RV_DEPS_SPDLOG_DOWNLOAD_HASH}
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(_include_dir
    ${_install_dir}/include
)

IF(RHEL_VERBOSE)
  SET(_lib_dir
      ${_install_dir}/lib64
  )
ELSE()
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
  SET(RV_SPDLOG_DEBUG_POSTFIX
      "d"
  )
ENDIF()

SET(_spdlog_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}spdlog${RV_SPDLOG_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
)

SET(_spdlog_lib
    ${_lib_dir}/${_spdlog_lib_name}
)

IF(RV_TARGET_WINDOWS)
  # MSYS2/CMake defaults to Ninja
  SET(_make_command
      ninja
  )
ELSE()
  SET(_make_command
      make
  )
ENDIF()

LIST(APPEND _configure_options "-DSPDLOG_BUILD_EXAMPLE=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  PATCH_COMMAND patch -N -u -b include/spdlog/tweakme.h -i "${PROJECT_SOURCE_DIR}/cmake/patches/spdlog_tweakme.h.patch" || true
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_spdlog_lib}
  USES_TERMINAL_BUILD TRUE
)

ADD_LIBRARY(spdlog::spdlog STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(spdlog::spdlog ${_target})
SET_PROPERTY(
  TARGET spdlog::spdlog
  PROPERTY IMPORTED_LOCATION ${_spdlog_lib}
)
SET_PROPERTY(
  TARGET spdlog::spdlog
  PROPERTY IMPORTED_SONAME ${_spdlog_lib_name}
)

FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  spdlog::spdlog
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST spdlog::spdlog)

IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_spdlog_lib_name})
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_spdlog_lib_name})
ENDIF()

SET(RV_DEPS_SPDLOG_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
