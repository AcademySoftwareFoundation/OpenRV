#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official sources: https://github.com/uclouvain/openjpeg
#
# Build instructions: https://github.com/uclouvain/openjpeg/blob/master/INSTALL.md
#

# version 2+ requires changes to IOjp2 project
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OPENJPEG" "${RV_DEPS_OPENJPEG_VERSION}" "make" "")

IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' since this CMake-based project isn't using lib64
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/uclouvain/openjpeg/archive/refs/tags/v${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_OPENJPEG_DOWNLOAD_HASH}
)

IF(RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("openjp2" "" "SHARED" "")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("openjp2" "${RV_DEPS_OPENJPEG_VERSION}" "SHARED" "")
ENDIF()

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'

# Do not build the executables (OpenJPEG calls them "codec executables"). BUILD_THIRDPARTY options is valid only if BUILD_CODEC=ON. PNG, TIFF and ZLIB are not
# needed anymore because they are used for the executables only.
LIST(APPEND _configure_options "-DBUILD_CODEC=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  DEPENDS ZLIB::ZLIB Tiff::Tiff PNG::PNG
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

ADD_LIBRARY(OpenJpeg::OpenJpeg SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(OpenJpeg::OpenJpeg ${_target})

SET_PROPERTY(
  TARGET OpenJpeg::OpenJpeg
  PROPERTY IMPORTED_LOCATION ${_libpath}
)

IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET OpenJpeg::OpenJpeg
    PROPERTY IMPORTED_IMPLIB ${_bin_dir}/${_implibname}
  )
ENDIF()

SET_PROPERTY(
  TARGET OpenJpeg::OpenJpeg
  PROPERTY IMPORTED_SONAME ${_libname}
)

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
SET(_openjpeg_include_dir
    "${_include_dir}/openjpeg-${_version_major}.${_version_minor}"
)
FILE(MAKE_DIRECTORY "${_openjpeg_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  OpenJpeg::OpenJpeg
  INTERFACE ${_openjpeg_include_dir}
)

LIST(APPEND RV_DEPS_LIST OpenJpeg::OpenJpeg)
