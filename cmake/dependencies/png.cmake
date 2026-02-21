#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official source repository https://libpng.sourceforge.io/
#
# Some clone on GitHub https://github.com/glennrp/libpng
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_PNG" "${RV_DEPS_PNG_VERSION}" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/glennrp/libpng/archive/refs/tags/v${_version}.tar.gz"
)

SET(_download_hash
    ${RV_DEPS_PNG_DOWNLOAD_HASH}
)

SET(_libpng_lib_version
    "${_version_major}${_version_minor}.${_version_patch}.0"
)
IF(NOT RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("png${_version_major}${_version_minor}" "${_libpng_lib_version}" "SHARED" "d")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("libpng${_version_major}${_version_minor}" "${_libpng_lib_version}" "SHARED" "d")
ENDIF()
# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES' Future: The main branch of libpng has deprecated
# 'PNG_EXECUTABLES' in favor of 'PNG_TOOLS'.
LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")
LIST(APPEND _configure_options "-DPNG_EXECUTABLES=OFF")
LIST(APPEND _configure_options "-DPNG_TESTS=OFF")
LIST(APPEND _configure_options "-DPNG_FRAMEWORK=OFF")

# Disable PNG's automatic rpath setup to avoid conflicts with RV's rpath management
LIST(APPEND _configure_options "-DCMAKE_INSTALL_RPATH=")

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
  DEPENDS ZLIB::ZLIB
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

IF(NOT RV_TARGET_WINDOWS)
  RV_ADD_IMPORTED_LIBRARY(
    NAME PNG::PNG
    TYPE SHARED
    LOCATION ${_libpath}
    SONAME ${_libname}
    INCLUDE_DIRS ${_include_dir}
    DEPENDS ${_target}
    ADD_TO_DEPS_LIST
  )
ELSE()
  # An import library (.lib) file is often used to resolve references to functions and variables in a DLL, enabling the linker to generate code for loading the
  # DLL and calling its functions at runtime.
  RV_ADD_IMPORTED_LIBRARY(
    NAME PNG::PNG
    TYPE SHARED
    LOCATION ${_implibpath}
    IMPLIB ${_implibpath}
    INCLUDE_DIRS ${_include_dir}
    DEPENDS ${_target}
    ADD_TO_DEPS_LIST
  )
ENDIF()
