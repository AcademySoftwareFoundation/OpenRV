#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_GLEW" "${RV_DEPS_GLEW_VERSION}" "make" "")

SET(_download_url
    "https://github.com/nigels-com/glew/archive/${_version}.zip"
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
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  # Patch to fix the build issue with OpenGL-Registry Pinning the OpenGL-Registry version to a specific commit https://github.com/nigels-com/glew/issues/449
  # Also clone the required glfixes repository
  PATCH_COMMAND
    cd auto && git clone https://github.com/KhronosGroup/OpenGL-Registry.git || true && cd OpenGL-Registry && git checkout
    a77f5b6ffd0b0b74904f755ae977fa278eac4e95 && cd .. && git clone --depth=1 --branch glew https://github.com/nigels-com/glfixes glfixes || true && touch
    OpenGL-Registry/.dummy && cd ..
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

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_glew_lib_name})

