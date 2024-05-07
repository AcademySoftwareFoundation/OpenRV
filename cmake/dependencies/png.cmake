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

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_PNG" "1.6.39" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/glennrp/libpng/archive/refs/tags/v${_version}.tar.gz"
)

SET(_download_hash
    "a704977d681a40d8223d8b957fd41b29"
)

SET(_libpng_lib_version
    "16.39.0"
)
IF(NOT RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("png16" "${_libpng_lib_version}" "SHARED" "d")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("libpng16" "${_libpng_lib_version}" "SHARED" "d")
ENDIF()
# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
# Future: The main branch of libpng has deprecated 'PNG_EXECUTABLES' in favor of 'PNG_TOOLS'.
LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")
LIST(APPEND _configure_options "-DPNG_EXECUTABLES=OFF")
LIST(APPEND _configure_options "-DPNG_TESTS=OFF")

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

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(PNG::PNG SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(PNG::PNG ${_target})
IF(NOT RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET PNG::PNG
    PROPERTY IMPORTED_LOCATION ${_libpath}
  )
  SET_PROPERTY(
    TARGET PNG::PNG
    PROPERTY IMPORTED_SONAME ${_libname}
  )
ELSE()
  # An import library (.lib) file is often used to resolve references to 
  # functions and variables in a DLL, enabling the linker to generate code 
  # for loading the DLL and calling its functions at runtime.
  SET_PROPERTY(
    TARGET PNG::PNG
    PROPERTY IMPORTED_LOCATION "${_implibpath}"
  )
  SET_PROPERTY(
    TARGET PNG::PNG
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  PNG::PNG
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST PNG::PNG)
