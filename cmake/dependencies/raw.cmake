#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# LibRaw official Web page: https://www.libraw.org/about
# LibRaw official sources:  https://www.libraw.org/data/LibRaw-0.21.1.tar.gz
# LibRaw build from sources: https://www.libraw.org/docs/Install-LibRaw-eng.html
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_RAW" "0.21.1" "make" "../src/configure")
IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' since this CMake-based project isn't using lib64
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/LibRaw/LibRaw/archive/refs/tags/${_version}.tar.gz"
)

SET(_download_hash
    "3ad334296a7a2c8ee841f353cc1b450b"
)

SET(_libraw_lib_version
    "23"
)
IF(NOT RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("raw" "${_libraw_lib_version}" "SHARED" "")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("libraw" "${_libraw_lib_version}" "SHARED" "")
ENDIF()

IF(RV_TARGET_WINDOWS)
  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_base_dir}/src
    INSTALL_DIR ${_install_dir}
    DEPENDS ZLIB::ZLIB
    CONFIGURE_COMMAND ""
    BUILD_COMMAND nmake /f Makefile.msvc
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )

  # Building with nmake for Windows doesn't provide a nice install target, we need to do it manually. We remove unneeded files after copying the required
  # directories
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs & files into ${_install_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_base_dir}/src/lib ${_lib_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_base_dir}/src/libraw ${_include_dir}/libraw
    # Copy only the DLL on Windows because there is no option to disable the "examples/samples" 
    # with Makefile.msvc.
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_bin_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/bin/${_libname} ${_bin_dir}
    COMMAND ${CMAKE_COMMAND} -E rm ${_lib_dir}/Makefile
  )

ELSE()
  # The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
  SET(_configure_options
      ""
  ) # Overrides defaults set in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
  LIST(APPEND _configure_options "--prefix=${_install_dir}")
  LIST(APPEND _configure_options "--enable-lcms")

  GET_TARGET_PROPERTY(_lcms_include_dir lcms INTERFACE_INCLUDE_DIRECTORIES)
  SET(_lcms2_flags
      "-I${_lcms_include_dir}"
  )
  SET(_lcms2_libs
      "-L${RV_STAGE_LIB_DIR} -llcms"
  )

  SET(_configure_command
    ${CMAKE_COMMAND} -E env LCMS2_CFLAGS='${_lcms2_flags}'
    ${CMAKE_COMMAND} -E env LCMS2_LIBS='${_lcms2_libs}'
    ${_configure_command}
  )

  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS ZLIB::ZLIB lcms
    CONFIGURE_COMMAND aclocal
    COMMAND autoreconf --install
    COMMAND ${_configure_command} ${_configure_options}
    BUILD_COMMAND ${_make_command} -j${_cpu_count}
    INSTALL_COMMAND ${_make_command} install
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ENDIF()

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(Raw::Raw SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(Raw::Raw ${_target})
SET_PROPERTY(
  TARGET Raw::Raw
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET Raw::Raw
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET Raw::Raw
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  Raw::Raw
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST Raw::Raw)
