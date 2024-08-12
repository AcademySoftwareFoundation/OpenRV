#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_ATOMIC_OPS"
)

SET(_version
    "7.7.0"
)

# Download a recent version that includes the feature we need (--disable-gpl) which hasn't been released nor tagged yet.
SET(_download_url
    "https://github.com/ivmai/libatomic_ops/archive/044573903530c4a8e8318e20a830d4a0531b2035.zip"
)

SET(_download_hash
    cc7fad1e71b3064abe1ea821ae9a9a6e
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(_lib_dir
    ${_install_dir}/lib
)

IF(RV_TARGET_WINDOWS)
  SET(_atomic_ops_lib_name
      libatomic_ops.a
  )
ELSE()
  SET(_atomic_ops_lib_name
      ${CMAKE_STATIC_LIBRARY_PREFIX}atomic_ops${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_atomic_ops_lib
    ${_lib_dir}/${_atomic_ops_lib_name}
)

SET(_make_command
    make
)
SET(_configure_command
    sh ./configure
)
SET(_autogen_command
    sh ./autogen.sh
)

# Make sure NOT to enable GPL
SET(_configure_args
    "--disable-gpl"
)
LIST(APPEND _configure_args "--prefix=${_install_dir}")

EXTERNALPROJECT_ADD(
  ${_target}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  CONFIGURE_COMMAND ${_autogen_command} && ${_configure_command} ${_configure_args}
  BUILD_COMMAND ${_make_command} -j${_cpu_count}
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_atomic_ops_lib}
  USES_TERMINAL_BUILD TRUE
)

ADD_LIBRARY(atomic_ops::atomic_ops STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(atomic_ops::atomic_ops ${_target})
SET_PROPERTY(
  TARGET atomic_ops::atomic_ops
  PROPERTY IMPORTED_LOCATION ${_atomic_ops_lib}
)

SET(_include_dir
    ${_install_dir}/include
)
FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  atomic_ops::atomic_ops
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST atomic_ops::atomic_ops)

ADD_CUSTOM_COMMAND(
  COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
  OUTPUT ${RV_STAGE_LIB_DIR}/${_atomic_ops_lib_name}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  DEPENDS ${_target}
)

ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${RV_STAGE_LIB_DIR}/${_atomic_ops_lib_name}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_ATOMIC_OPS_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
