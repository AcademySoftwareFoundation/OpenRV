#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_DAV1D"
)

SET(_version
    "1.0.0"
)

SET(_download_url
    "https://github.com/videolan/dav1d/archive/refs/tags/${_version}.zip"
)
SET(_download_hash
    "425282a997804984c5c115aacb005ab4"
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

SET(_david_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}dav1d${CMAKE_STATIC_LIBRARY_SUFFIX}
)

SET(_dav1d_lib
    ${_lib_dir}/${_david_lib_name}
)

SET(_configure_command
    meson
)
SET(_make_command
    ninja
)

IF(${RV_OSX_EMULATION})
  SET(_meson_cross_file
      "${PROJECT_SOURCE_DIR}/src/build/meson_arch_x86_64.txt"
  )
  SET(_configure_command
      ${_configure_command} "--cross-file" ${_meson_cross_file}
  )
ENDIF()

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
  CONFIGURE_COMMAND ${_configure_command} ./_build --default-library=${_default_library} --prefix=${_install_dir} -Denable_tests=false -Denable_tools=false
  BUILD_COMMAND ${_make_command} -C _build
  INSTALL_COMMAND ${_make_command} -C _build install
  COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_dav1d_lib}
  USES_TERMINAL_BUILD TRUE
)

ADD_LIBRARY(dav1d::dav1d STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(dav1d::dav1d ${_target})
SET_PROPERTY(
  TARGET dav1d::dav1d
  PROPERTY IMPORTED_LOCATION ${_dav1d_lib}
)
SET_PROPERTY(
  TARGET dav1d::dav1d
  PROPERTY IMPORTED_SONAME ${_david_lib_name}
)

FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  dav1d::dav1d
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST dav1d::dav1d)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/lib" --destination
            "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/bin" --destination
            "${RV_STAGE_BIN_DIR}"
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_david_lib_name}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_david_lib_name}
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_david_lib_name}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_DAV1D_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)

# FFmpeg customization adding dav1d codec support to FFmpeg
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_DEPENDS" RV_DEPS_DAV1D
)
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_EXTRA_C_OPTIONS" "--extra-cflags=-I${_include_dir}"
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS" "--extra-ldflags=-LIBPATH:${_lib_dir}"
  )
ELSE()
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS" "--extra-ldflags=-L${_lib_dir}"
  )
ENDIF()
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_EXTERNAL_LIBS" "--enable-libdav1d"
)
SET(RV_DEPS_DAVID_LIB_DIR
    ${_lib_dir}
    CACHE INTERNAL ""
)
