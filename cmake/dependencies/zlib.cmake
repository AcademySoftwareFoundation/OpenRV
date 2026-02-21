#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_ZLIB" "${RV_DEPS_ZLIB_VERSION}" "" "")

SET(_download_url
    "https://github.com/madler/zlib/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    ${RV_DEPS_ZLIB_DOWNLOAD_HASH}
)

SET(_lib_dir
    ${_install_dir}/lib
)

IF(RV_TARGET_WINDOWS)
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_zlibname
        "zlibd"
    )
  ELSE()
    SET(_zlibname
        "zlib"
    )
  ENDIF()
ELSE()
  SET(_zlibname
      "z"
  )
ENDIF()

SET(_libname
    ${CMAKE_SHARED_LIBRARY_PREFIX}${_zlibname}${CMAKE_SHARED_LIBRARY_SUFFIX}
)
SET(_libpath
    ${_lib_dir}/${_libname}
)

IF(RV_TARGET_WINDOWS)
  # Zlib is a Shared lib (see _libname): On Windows it'll go in the bin dir.
  SET(_libpath
      ${_bin_dir}/${_libname}
  )
ENDIF()

LIST(APPEND _zlib_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibname
      ${CMAKE_IMPORT_LIBRARY_PREFIX}${_zlibname}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_implibpath
      ${_lib_dir}/${_implibname}
  )
  LIST(APPEND _zlib_byproducts ${_implibpath})
ENDIF()

# The patch comes from the ZLIB port in Vcpkg repository. The name of the patch is kept as is. See https://github.com/microsoft/vcpkg/tree/master/ports/zlib
# Description: Fix unistd.h being incorrectly required when imported from a project defining HAVE_UNISTD_H=0
SET(_patch_command
    patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/zconf.h.cmakein_prevent_invalid_inclusions.patch && patch -p1 <
    ${CMAKE_CURRENT_SOURCE_DIR}/patch/zconf.h.in_prevent_invalid_inclusions.patch
)

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  PATCH_COMMAND ${_patch_command}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_zlib_byproducts}
  USES_TERMINAL_BUILD TRUE
)

IF(RV_TARGET_WINDOWS)
  # FFmpeg expect "zlib" in Release and Debug.
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Renaming the ZLIB import debug lib to the name FFmpeg is expecting (release name)"
      COMMAND ${CMAKE_COMMAND} -E copy ${_implibpath} ${_lib_dir}/zlib.lib
    )
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_bin_dir} OUTPUTS ${RV_STAGE_BIN_DIR}/${_libname})
ELSEIF(RV_TARGET_DARWIN)
  RV_STAGE_DEPENDENCY_LIBS(
    TARGET
    ${_target}
    OUTPUTS
    ${RV_STAGE_LIB_DIR}/${_libname}
    PRE_COMMANDS
    COMMAND
    ${CMAKE_INSTALL_NAME_TOOL}
    -id
    "@rpath/${_libname}"
    "${_lib_dir}/${_libname}"
  )
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname})
ENDIF()

RV_ADD_IMPORTED_LIBRARY(
  NAME ZLIB::ZLIB
  TYPE SHARED
  LOCATION ${_libpath}
  SONAME ${_libname}
  IMPLIB ${_implibpath}
  INCLUDE_DIRS ${_include_dir}
  DEPENDS ${_target}
  ADD_TO_DEPS_LIST
)

SET(RV_DEPS_ZLIB_INCLUDE_DIR
    ${_include_dir}
    CACHE STRING "Path to installed includes for ${_target}"
)

# FFmpeg customization
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_DEPENDS" RV_DEPS_ZLIB
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
