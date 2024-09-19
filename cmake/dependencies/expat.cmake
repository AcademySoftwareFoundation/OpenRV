#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official source repository https://github.com/libexpat/libexpat
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_EXPAT" "2.6.3" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()


string(REPLACE "." "_" _version_underscored ${_version})
SET(_download_url
    "https://github.com/libexpat/libexpat/archive/refs/tags/R_${_version_underscored}.tar.gz"
)

SET(_download_hash
    "985086e206a01e652ca460eb069e4780"
)

SET(_libexpat_lib_version
    "2.6.3"
)

RV_MAKE_STANDARD_LIB_NAME("libexpat" "" "SHARED" "d")

# Remove the -S argument from _configure_options, and adjust the path for Expat.
list(REMOVE_ITEM _configure_options "-S ${_source_dir}")
# Expat source is under expat folder and not directly under repository root.
LIST(APPEND _configure_options "-S ${_source_dir}/expat")

LIST(APPEND _configure_options "-DEXPAT_BUILD_DOCS=OFF")
LIST(APPEND _configure_options "-DEXPAT_BUILD_EXAMPLES=OFF")
LIST(APPEND _configure_options "-DEXPAT_BUILD_TESTS=OFF")
LIST(APPEND _configure_options "-DEXPAT_BUILD_TOOLS=OFF")

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
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(EXPAT::EXPAT SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(EXPAT::EXPAT ${_target})

# An import library (.lib) file is often used to resolve references to 
# functions and variables in a DLL, enabling the linker to generate code 
# for loading the DLL and calling its functions at runtime.
SET_PROPERTY(
    TARGET EXPAT::EXPAT
    PROPERTY IMPORTED_LOCATION "${_libpath}"
)
SET_PROPERTY(
    TARGET EXPAT::EXPAT
    PROPERTY IMPORTED_IMPLIB "${_implibpath}"
)

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  EXPAT::EXPAT
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST EXPAT::EXPAT)