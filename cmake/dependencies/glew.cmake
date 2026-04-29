#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project.
# Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

IF(RV_USE_SYSTEM_DEPS)
  FIND_PACKAGE(GLEW REQUIRED)
  
    IF(DEFINED GLEW_VERSION)
      SET(RV_DEPS_GLEW_VERSION "${GLEW_VERSION}" CACHE STRING "" FORCE)
    ELSEIF(DEFINED GLEW_VERSION)
      SET(RV_DEPS_GLEW_VERSION "${GLEW_VERSION}" CACHE STRING "" FORCE)
    ELSEIF(DEFINED GLEW_VERSION_STRING)
      SET(RV_DEPS_GLEW_VERSION "${GLEW_VERSION_STRING}" CACHE STRING "" FORCE)
    ELSEIF(DEFINED GLEW_VERSION_STRING)
      SET(RV_DEPS_GLEW_VERSION "${GLEW_VERSION_STRING}" CACHE STRING "" FORCE)
    ENDIF()
    RETURN()
ENDIF()

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_GLEW" "${RV_DEPS_GLEW_VERSION}" "make" "")

SET(_download_url
    "https://github.com/nigels-com/glew/archive/refs/tags/glew-${_version}.tar.gz"
)

SET(_download_hash
    ${RV_DEPS_GLEW_DOWNLOAD_HASH}
)

IF(RV_TARGET_DARWIN)
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}GLEW.${RV_DEPS_GLEW_VERSION_LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}GLEW${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_GLEW_VERSION_LIB}
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
  STATIC
  LOCATION
  ${_glew_lib}
  INCLUDE_DIRS
  ${_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_glew_lib_name})
