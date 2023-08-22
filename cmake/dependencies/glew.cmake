#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_GLEW"
)

SET(_version
    "2.2.0"
)

SET(_download_url
    "https://github.com/nigels-com/glew/archive/refs/tags/glew-${_version}.zip"
)

SET(_download_hash
    f150f61074d049ff0423b09b18cd1ef6
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

IF(RV_TARGET_LINUX)
  SET(_lib_dir
      ${_install_dir}/lib64
  )
ELSE()
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

SET(_glew_name
    "GLEW"
)

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(_glew_name
      "${_glew_name}d"
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}${_glew_name}.${_version}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_glew_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}${_glew_name}${CMAKE_SHARED_LIBRARY_SUFFIX}.${_version}
  )
ENDIF()

SET(_glew_lib
    ${_lib_dir}/${_glew_lib_name}
)

SET(_make_command
    make
)

EXTERNALPROJECT_ADD(
  ${_target}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  CONFIGURE_COMMAND make -C auto
  COMMAND
    cd build/cmake && ${CMAKE_COMMAND} -B ${RV_DEPS_BASE_DIR}/${_target}/build -DCMAKE_INSTALL_PREFIX=${_install_dir}
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  BUILD_COMMAND ${CMAKE_COMMAND} --build ${RV_DEPS_BASE_DIR}/${_target}/build --parallel -v
  INSTALL_COMMAND ${CMAKE_COMMAND} --install ${RV_DEPS_BASE_DIR}/${_target}/build
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
