#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_DAV1D" "${RV_DEPS_DAV1D_VERSION}" "ninja" "meson")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/videolan/dav1d/archive/refs/tags/${_version}.zip"
)
SET(_download_hash
    ${RV_DEPS_DAV1D_DOWNLOAD_HASH}
)

# _lib_dir_name is needed for the meson --libdir option
IF(RHEL_VERBOSE)
  SET(_lib_dir_name
      lib64
  )
ELSE()
  SET(_lib_dir_name
      lib
  )
ENDIF()

SET(_david_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}dav1d${CMAKE_STATIC_LIBRARY_SUFFIX}
)

SET(_dav1d_lib
    ${_lib_dir}/${_david_lib_name}
)

IF(APPLE)
  # Cross-file must be specified because if Rosetta is used to compile for x86_64 from ARM64, Meson still detects ARM64 as the default architecture.

  IF(RV_TARGET_APPLE_X86_64)
    SET(_meson_cross_file
        "${PROJECT_SOURCE_DIR}/src/build/meson_arch_x86_64.txt"
    )
  ELSEIF(RV_TARGET_APPLE_ARM64)
    SET(_meson_cross_file
        "${PROJECT_SOURCE_DIR}/src/build/meson_arch_arm64.txt"
    )
  ENDIF()

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
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${_configure_command} ./_build --libdir=${_lib_dir_name} --default-library=${_default_library} --prefix=${_install_dir} -Denable_tests=false
                    -Denable_tools=false
  BUILD_COMMAND ${_make_command} -C _build
  INSTALL_COMMAND ${_make_command} -C _build install
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_dav1d_lib}
  USES_TERMINAL_BUILD TRUE
)

RV_ADD_IMPORTED_LIBRARY(
  NAME dav1d::dav1d
  TYPE STATIC
  LOCATION ${_dav1d_lib}
  SONAME ${_david_lib_name}
  INCLUDE_DIRS ${_include_dir}
  DEPENDS ${_target}
  ADD_TO_DEPS_LIST
)

IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_install_dir}/bin OUTPUTS ${RV_STAGE_LIB_DIR}/${_david_lib_name})
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_david_lib_name})
ENDIF()

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
