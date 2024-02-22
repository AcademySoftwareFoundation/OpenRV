#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_OPENEXR"
)

SET(_version
    "3.1.7"
)

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    "a278571601083a74415d40d2497d205c"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(${_target}_ROOT_DIR
    ${_install_dir}
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
IF(${RV_OSX_EMULATION})
  SET(_darwin_x86_64
      "arch" "${RV_OSX_EMULATION_ARCH}"
  )
  SET(_make_command
      ${_darwin_x86_64} ${_make_command}
  )
ENDIF()
IF(RV_TARGET_WINDOWS)
  # MSYS2/CMake defaults to Ninja
  SET(_make_command
      ninja
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_openexr_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXR-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_openexr_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXR-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()
SET(_openexr_lib
    ${_lib_dir}/${_openexr_name}
)

SET(LIB_VERSION_SUFFIX
    "30.7.1"
)

IF(RV_TARGET_DARWIN)
  SET(_openexrcore_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXRCore-3_1${RV_DEBUG_POSTFIX}.${LIB_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_openexrcore_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXRCore-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${LIB_VERSION_SUFFIX}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_openexrcore_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXRCore-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_openexrcore_lib
    ${_lib_dir}/${_openexrcore_name}
)

IF(RV_TARGET_DARWIN)
  SET(_ilmthread_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}IlmThread-3_1${RV_DEBUG_POSTFIX}.${LIB_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_ilmthread_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}IlmThread-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${LIB_VERSION_SUFFIX}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_ilmthread_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}IlmThread-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_ilmthread_lib
    ${_lib_dir}/${_ilmthread_name}
)

IF(RV_TARGET_DARWIN)
  SET(_iex_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}Iex-3_1${RV_DEBUG_POSTFIX}.${LIB_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_iex_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}Iex-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${LIB_VERSION_SUFFIX}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_iex_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}Iex-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_iex_lib
    ${_lib_dir}/${_iex_name}
)

LIST(APPEND _openexr_byproducts ${_openexr_lib} ${_ilmthread_lib} ${_iex_lib})

IF(RV_TARGET_WINDOWS)
  SET(_openexr_implib
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}OpenEXR-3_1${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_ilmthread_implib
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}IlmThread-3_1${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_iex_implib
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}Iex-3_1${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )

  LIST(APPEND _openexr_byproducts ${_openexr_implib} ${_ilmthread_implib} ${_iex_implib})
ENDIF()

SET(_patch_command
    patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/openexr_invalid_to_black.patch
)

SET(_cmake_configure_command
    ${CMAKE_COMMAND}
)
LIST(APPEND _cmake_configure_command "-DCMAKE_INSTALL_PREFIX=${_install_dir}")
LIST(APPEND _cmake_configure_command "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
LIST(APPEND _cmake_configure_command "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
LIST(APPEND _cmake_configure_command "-DCMAKE_PREFIX_PATH=${RV_DEPS_IMATH_CMAKE_DIR}")
LIST(APPEND _cmake_configure_command "-DBUILD_TESTING=OFF")
LIST(APPEND _cmake_configure_command "${RV_DEPS_BASE_DIR}/${_target}/src")
IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(_zlib_implibpath ZLIB::ZLIB IMPORTED_IMPLIB)
  LIST(APPEND _cmake_configure_command "-DZLIB_INCLUDE_DIR=${RV_DEPS_ZLIB_INCLUDE_DIR}")
  LIST(APPEND _cmake_configure_command "-DZLIB_LIBRARY=${_zlib_implibpath}")
ENDIF()

# OpenEXR tools are not needed.
LIST(APPEND _cmake_configure_command "-DOPENEXR_BUILD_TOOLS=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  DEPENDS Imath::Imath ZLIB::ZLIB
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${_cmake_configure_command}
  BUILD_COMMAND ${_make_command} -j${_cpu_count}
  PATCH_COMMAND ${_patch_command}
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_openexr_byproducts}
  USES_TERMINAL_BUILD TRUE
)

SET(_include_dir
    ${_install_dir}/include/OpenEXR
)

FILE(MAKE_DIRECTORY ${_include_dir})

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/lib" --destination
            "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/bin" --destination
            "${RV_STAGE_BIN_DIR}"
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_openexrcore_name} ${RV_STAGE_BIN_DIR}/${_ilmthread_name} ${RV_STAGE_BIN_DIR}/${_iex_name}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_openexrcore_name} ${RV_STAGE_LIB_DIR}/${_ilmthread_name} ${RV_STAGE_LIB_DIR}/${_iex_name}
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_openexrcore_name} ${RV_STAGE_LIB_DIR}/${_ilmthread_name} ${RV_STAGE_LIB_DIR}/${_iex_name}
  )
ENDIF()

ADD_LIBRARY(OpenEXR::IlmThread SHARED IMPORTED GLOBAL)
SET_PROPERTY(
  TARGET OpenEXR::IlmThread
  PROPERTY IMPORTED_LOCATION ${_ilmthread_lib}
)
SET_PROPERTY(
  TARGET OpenEXR::IlmThread
  PROPERTY IMPORTED_SONAME ${_ilmthread_name}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET OpenEXR::IlmThread
    PROPERTY IMPORTED_IMPLIB ${_ilmthread_implib}
  )
ENDIF()
LIST(APPEND RV_DEPS_LIST OpenEXR::IlmThread)

ADD_LIBRARY(OpenEXR::Iex SHARED IMPORTED GLOBAL)
SET_PROPERTY(
  TARGET OpenEXR::Iex
  PROPERTY IMPORTED_LOCATION ${_iex_lib}
)
SET_PROPERTY(
  TARGET OpenEXR::Iex
  PROPERTY IMPORTED_SONAME ${_iex_name}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET OpenEXR::Iex
    PROPERTY IMPORTED_IMPLIB ${_iex_implib}
  )
ENDIF()
LIST(APPEND RV_DEPS_LIST OpenEXR::Iex)

ADD_LIBRARY(OpenEXR::OpenEXR SHARED IMPORTED GLOBAL)
SET_PROPERTY(
  TARGET OpenEXR::OpenEXR
  PROPERTY IMPORTED_LOCATION ${_openexr_lib}
)
SET_PROPERTY(
  TARGET OpenEXR::OpenEXR
  PROPERTY IMPORTED_SONAME ${_openexr_name}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET OpenEXR::OpenEXR
    PROPERTY IMPORTED_IMPLIB ${_openexr_implib}
  )
ENDIF()

TARGET_INCLUDE_DIRECTORIES(
  OpenEXR::OpenEXR
  INTERFACE ${_include_dir}
)
TARGET_LINK_LIBRARIES(
  OpenEXR::OpenEXR
  INTERFACE Imath::Imath OpenEXR::Iex OpenEXR::IlmThread ZLIB::ZLIB
)
LIST(APPEND RV_DEPS_LIST OpenEXR::OpenEXR)

ADD_DEPENDENCIES(OpenEXR::OpenEXR ${_target})

SET(RV_DEPS_OPENEXR_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
