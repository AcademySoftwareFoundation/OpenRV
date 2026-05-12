#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_PCRE2" "pcre2-${RV_DEPS_PCRE2_VERSION}" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/PCRE2Project/pcre2/archive/refs/tags/${_version}.zip"
)

SET(_download_hash
    ${RV_DEPS_PCRE2_DOWNLOAD_HASH}
)

# PCRE is not used for Linux and MacOS (Boost regex is used) in the current code.
IF(RV_TARGET_WINDOWS)
  # PCRE2's CMakeLists.txt sets CMAKE_DEBUG_POSTFIX to "d" internally, which cannot be overridden via cache variable. Account for it here.
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_pcre2_debug_postfix
        "d"
    )
  ELSE()
    SET(_pcre2_debug_postfix
        ""
    )
  ENDIF()

  # MSVC library naming (CMake build)
  SET(_pcre2_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}pcre2-8${_pcre2_debug_postfix}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_pcre2_libname_posix
      ${CMAKE_SHARED_LIBRARY_PREFIX}pcre2-posix${_pcre2_debug_postfix}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )

  SET(_pcre2_implibname
      ${CMAKE_IMPORT_LIBRARY_PREFIX}pcre2-8${_pcre2_debug_postfix}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_pcre2_implibname_posix
      ${CMAKE_IMPORT_LIBRARY_PREFIX}pcre2-posix${_pcre2_debug_postfix}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )

  SET(_pcre2_libpath
      ${_bin_dir}/${_pcre2_libname}
  )
  SET(_pcre2_libpath_posix
      ${_bin_dir}/${_pcre2_libname_posix}
  )

  SET(_pcre2_implibpath
      ${_lib_dir}/${_pcre2_implibname}
  )
  SET(_pcre2_implibpath_posix
      ${_lib_dir}/${_pcre2_implibname_posix}
  )
ENDIF()

SET(_pcre2_include_dir
    ${_install_dir}/include
)

# PCRE2-specific CMake options (replaces autotools configure args)
LIST(APPEND _configure_options "-DBUILD_SHARED_LIBS=ON")
LIST(APPEND _configure_options "-DBUILD_STATIC_LIBS=OFF")
LIST(APPEND _configure_options "-DPCRE2_BUILD_PCRE2GREP=OFF")
LIST(APPEND _configure_options "-DPCRE2_BUILD_TESTS=OFF")
LIST(APPEND _configure_options "-DPCRE2_SUPPORT_LIBBZ2=OFF")
LIST(APPEND _configure_options "-DPCRE2_SUPPORT_LIBZ=OFF")
LIST(APPEND _configure_options "-DINSTALL_MSVC_PDB=OFF")

IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
  LIST(APPEND _configure_options "-DPCRE2_DEBUG=ON")
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  DEPENDS ZLIB::ZLIB
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_pcre2_libpath} ${_pcre2_libpath_posix} ${_pcre2_implibpath} ${_pcre2_implibpath_posix}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(
  TARGET
  ${_target}
  BIN_DIR
  ${_bin_dir}
  OUTPUTS
  ${RV_STAGE_BIN_DIR}/${_pcre2_libname}
  ${RV_STAGE_BIN_DIR}/${_pcre2_libname_posix}
)

RV_ADD_IMPORTED_LIBRARY(
  NAME
  pcre2-8
  TYPE
  SHARED
  LOCATION
  ${_pcre2_libpath}
  SONAME
  ${_pcre2_libname}
  IMPLIB
  ${_pcre2_implibpath}
  INCLUDE_DIRS
  ${_pcre2_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)
TARGET_COMPILE_DEFINITIONS(
  pcre2-8
  INTERFACE PCRE2_CODE_UNIT_WIDTH=8
)

RV_ADD_IMPORTED_LIBRARY(
  NAME
  pcre2-posix
  TYPE
  SHARED
  LOCATION
  ${_pcre2_libpath_posix}
  SONAME
  ${_pcre2_libname_posix}
  IMPLIB
  ${_pcre2_implibpath_posix}
  INCLUDE_DIRS
  ${_pcre2_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)
