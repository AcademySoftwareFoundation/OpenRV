#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_AJA"
)

SET(_version
    "16.2"
)

SET(_patch
    "bugfix5"
)

SET(_download_url
    "https://github.com/aja-video/ntv2/archive/refs/tags/v${_version}-${_patch}.zip"
)

SET(_download_hash
    "5ec7f3f7ecfc322ca9307203155a4481"
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

SET(_make_command
    make
)
SET(_configure_command
    cmake
)

IF(${RV_OSX_EMULATION})
  SET(_darwin_x86_64
      "arch" "${RV_OSX_EMULATION_ARCH}"
  )

  SET(_make_command
      ${_darwin_x86_64} ${_make_command}
  )
  SET(_configure_command
      ${_darwin_x86_64} ${_configure_command}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  # MSYS2/CMake defaults to Ninja
  SET(_make_command
      ninja
  )
ENDIF()

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(AJA_DEBUG_POSTFIX
      "d"
  )
ENDIF()

SET(_aja_ntv2_libname
    ${CMAKE_STATIC_LIBRARY_PREFIX}ajantv2${AJA_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
)

SET(_aja_ntv2_lib
    ${_lib_dir}/${_aja_ntv2_libname}
)
SET(_aja_ntv2_include_dir
    ${_include_dir}/ajalibraries/ajantv2/includes
)
SET(_aja_include_dir
    ${_include_dir}/ajalibraries
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
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${_install_dir} -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DAJA_BUILD_APPS=OFF ${RV_DEPS_BASE_DIR}/${_target}/src
  BUILD_COMMAND ${_make_command} -j${_cpu_count}
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_aja_ntv2_lib}
  USES_TERMINAL_BUILD TRUE
)

ADD_LIBRARY(aja::ntv2 STATIC IMPORTED GLOBAL)
ADD_DEPENDENCIES(aja::ntv2 ${_target})
SET_PROPERTY(
  TARGET aja::ntv2
  PROPERTY IMPORTED_LOCATION ${_aja_ntv2_lib}
)
SET_PROPERTY(
  TARGET aja::ntv2
  PROPERTY IMPORTED_SONAME ${_aja_ntv2_libname}
)

FILE(MAKE_DIRECTORY ${_aja_include_dir} ${_aja_ntv2_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  aja::ntv2
  INTERFACE ${_aja_include_dir} ${_aja_ntv2_include_dir}
)

IF(RV_TARGET_DARWIN)
  LIST(APPEND _aja_compile_options "-DAJAMac=1")
  LIST(APPEND _aja_compile_options "-DAJA_MAC=1")
ELSEIF(RV_TARGET_LINUX)
  LIST(APPEND _aja_compile_options "-DAJALinux=1")
  LIST(APPEND _aja_compile_options "-DAJA_LINUX=1")
ELSEIF(RV_TARGET_WINDOWS)
  LIST(APPEND _aja_compile_options "-DMSWindows=1")
  LIST(APPEND _aja_compile_options "-DAJA_WINDOWS=1")
ENDIF()
SET(RV_DEPS_AJA_COMPILE_OPTIONS
    ${_aja_compile_options}
    CACHE INTERNAL "" FORCE
)

LIST(APPEND RV_DEPS_LIST aja::ntv2)

IF(RV_TARGET_WINDOWS)
  FILE(MAKE_DIRECTORY ${_install_dir}/lib)
  FILE(MAKE_DIRECTORY ${_install_dir}/bin)

  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_aja_ntv2_libname}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_aja_ntv2_libname}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_aja_ntv2_libname}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_AJA_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
