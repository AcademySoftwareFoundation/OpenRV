#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_GLEW" "e1a80a9f12d7def202d394f46e44cfced1104bfb" "make" "")

SET(_download_url
    "https://github.com/nigels-com/glew/archive/${_version}.zip"
)

SET(_download_hash
    9bfc689dabeb4e305ce80b5b6f28bcf9
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
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

IF(RV_TARGET_DARWIN)
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}GLEW.2.2.0${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}GLEW${CMAKE_SHARED_LIBRARY_SUFFIX}.2.2.0
  )
ENDIF()
SET(_glew_lib
    ${_lib_dir}/${_glew_lib_name}
)

EXTERNALPROJECT_ADD(
  ${_target}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  CONFIGURE_COMMAND cd auto && ${_make_command} && cd .. && ${_make_command}
  BUILD_COMMAND ${_make_command} -j${_cpu_count} GLEW_DEST=${_install_dir}
  INSTALL_COMMAND ${_make_command} install LIBDIR=${_lib_dir} GLEW_DEST=${_install_dir}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_glew_lib}
  USES_TERMINAL_BUILD TRUE
)

ADD_LIBRARY(GLEW::GLEW STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(GLEW::GLEW ${_target})
SET_PROPERTY(
  TARGET GLEW::GLEW
  PROPERTY IMPORTED_LOCATION ${_glew_lib}
)

FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  GLEW::GLEW
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST GLEW::GLEW)

ADD_CUSTOM_COMMAND(
  COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
  OUTPUT ${RV_STAGE_LIB_DIR}/${_glew_lib_name}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  DEPENDS ${_target}
)

ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${RV_STAGE_LIB_DIR}/${_glew_lib_name}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_GLEW_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
