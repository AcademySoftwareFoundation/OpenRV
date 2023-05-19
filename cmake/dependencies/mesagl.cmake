#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_MESAGL"
)

SET(_version
    "23.0.3"
)

SET(_download_url
    "https://gitlab.freedesktop.org/mesa/mesa/-/archive/mesa-${_version}/mesa-mesa-${_version}.zip"
)

SET(_download_hash
    "7be593152e45a706c52528dbac19a616"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(_include_dir
    ${_install_dir}/include
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

SET(_mesagl_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}mesagl${CMAKE_STATIC_LIBRARY_SUFFIX}
)

SET(_mesagl_lib
    ${_lib_dir}/${_mesagl_lib_name}
)

SET(_make_command
    ninja
)

SET(_configure_command
    meson setup
)

IF(RV_TARGET_WINDOWS)
  SET(_default_library
      shared
  )
ELSE()
  SET(_default_library
      static
  )
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${_configure_command} ./_build --default-library=${_default_library} --prefix=${_install_dir} #-Dosmesa=true -Dglx=xlib #-Dgallium-drivers=swrast
  BUILD_COMMAND ${_make_command} -C _build
  INSTALL_COMMAND ${_make_command} -C _build install
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_mesagl_lib}
  USES_TERMINAL_BUILD TRUE
)

ADD_LIBRARY(MesaGL::MesaGL STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(MesaGL::MesaGL ${_target})
SET_PROPERTY(
  TARGET MesaGL::MesaGL
  PROPERTY IMPORTED_LOCATION ${_mesagl_lib}
)
SET_PROPERTY(
  TARGET MesaGL::MesaGL
  PROPERTY IMPORTED_SONAME ${_mesagl_lib_name}
)

FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  MesaGL::MesaGL
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST MesaGL::MesaGL)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_mesagl_lib_name}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_mesagl_lib_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_mesagl_lib_name}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_MESAGL_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
