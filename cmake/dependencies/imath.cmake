#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMATH" "3.1.6" "" "")

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    "3900f9e7cf8a0ae3edf2552ea92ef7d8"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)
SET(_include_dir
    ${_install_dir}/include/Imath
)
SET(RV_DEPS_IMATH_ROOT_DIR ${_install_dir})

IF(RV_TARGET_DARWIN)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}.29.5.0${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.29.5.0
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

IF(RHEL_VERBOSE)
  SET(_lib_dir
      ${_install_dir}/lib64
  )
ELSE()
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

SET(RV_DEPS_IMATH_CMAKE_DIR
    ${_lib_dir}/cmake/Imath
    CACHE STRING "Path to Imath CMake files ${_target}"
)

SET(RV_DEPS_IMATH_CMAKE_DIR
    ${_lib_dir}/cmake/Imath
)

SET(_libpath
    ${_lib_dir}/${_libname}
)

LIST(APPEND _imath_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibpath
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  LIST(APPEND _imath_byproducts ${_implibpath})
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_imath_byproducts}
  USES_TERMINAL_BUILD TRUE
)

FILE(MAKE_DIRECTORY "${_include_dir}")

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_libname}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_libname}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_libname}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(Imath::Imath SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(Imath::Imath ${_target})
SET_PROPERTY(
  TARGET Imath::Imath
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET Imath::Imath
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET Imath::Imath
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()
TARGET_INCLUDE_DIRECTORIES(
  Imath::Imath
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST Imath::Imath)

SET(RV_DEPS_IMATH_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
