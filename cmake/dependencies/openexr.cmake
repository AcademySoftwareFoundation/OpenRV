#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OPENEXR" "${RV_DEPS_OPENEXR_VERSION}" "" "")

SET(_openexr_deps_list_targets
    OpenEXR::OpenEXR OpenEXR::IlmThread OpenEXR::Iex
)

# --- Try to find installed package ---
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  OpenEXR
  VERSION
  ${RV_DEPS_OPENEXR_VERSION}
  DEPS_LIST_TARGETS
  ${_openexr_deps_list_targets}
)

# --- Library naming (shared between find and build paths) ---
SET(_openexr_libname_suffix_
    "${RV_DEPS_OPENEXR_LIBNAME_SUFFIX}"
)

IF(RV_TARGET_WINDOWS)
  SET(_openexr_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXR-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_openexr_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXR-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()
SET(_openexr_lib
    ${_lib_dir}/${_openexr_name}
)

SET(LIB_VERSION_SUFFIX
    "${RV_DEPS_OPENEXR_LIB_VERSION_SUFFIX}"
)

IF(RV_TARGET_DARWIN)
  SET(_openexrcore_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXRCore-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}.${LIB_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_openexrcore_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXRCore-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${LIB_VERSION_SUFFIX}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_openexrcore_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}OpenEXRCore-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_openexrcore_lib
    ${_lib_dir}/${_openexrcore_name}
)

IF(RV_TARGET_DARWIN)
  SET(_ilmthread_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}IlmThread-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}.${LIB_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_ilmthread_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}IlmThread-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${LIB_VERSION_SUFFIX}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_ilmthread_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}IlmThread-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_ilmthread_lib
    ${_lib_dir}/${_ilmthread_name}
)

IF(RV_TARGET_DARWIN)
  SET(_iex_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}Iex-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}.${LIB_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_iex_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}Iex-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${LIB_VERSION_SUFFIX}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_iex_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}Iex-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_iex_lib
    ${_lib_dir}/${_iex_name}
)

LIST(APPEND _openexr_byproducts ${_openexr_lib} ${_ilmthread_lib} ${_iex_lib})

IF(RV_TARGET_WINDOWS)
  SET(_openexr_implib
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}OpenEXR-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_ilmthread_implib
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}IlmThread-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_iex_implib
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}Iex-${_openexr_libname_suffix_}${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )

  LIST(APPEND _openexr_byproducts ${_openexr_implib} ${_ilmthread_implib} ${_iex_implib})
ENDIF()

# --- Build from source if not found ---
IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/openexr.cmake)

  # Build path: we control the filenames, use OUTPUTS for precise tracking
  IF(RV_TARGET_WINDOWS)
    RV_STAGE_DEPENDENCY_LIBS(
      TARGET
      ${_target}
      BIN_DIR
      ${_bin_dir}
      OUTPUTS
      ${RV_STAGE_BIN_DIR}/${_openexr_name}
      ${RV_STAGE_BIN_DIR}/${_openexrcore_name}
      ${RV_STAGE_BIN_DIR}/${_ilmthread_name}
      ${RV_STAGE_BIN_DIR}/${_iex_name}
    )
  ELSE()
    RV_STAGE_DEPENDENCY_LIBS(
      TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_openexrcore_name} ${RV_STAGE_LIB_DIR}/${_ilmthread_name} ${RV_STAGE_LIB_DIR}/${_iex_name}
    )
  ENDIF()
ELSE()
  # CONFIG found — OpenEXR::xxx targets already exist. Create any missing targets as a safety net.
  FOREACH(
    _exr_target
    ${_openexr_deps_list_targets}
  )
    IF(NOT TARGET ${_exr_target})
      RV_ADD_IMPORTED_LIBRARY(
        NAME
        ${_exr_target}
        TYPE
        SHARED
        LOCATION
        ${_lib_dir}/${_openexr_name}
        SONAME
        ${_openexr_name}
        INCLUDE_DIRS
        ${_include_dir}
        DEPENDS
        ${_target}
      )
    ENDIF()
  ENDFOREACH()

  # Found path: actual filenames may differ, use TARGET_LIBS to resolve at build time. Include OpenEXRCore which exists when found via CONFIG but has no
  # imported target in build path.
  SET(_openexr_stage_targets
      ${_openexr_deps_list_targets}
  )
  IF(TARGET OpenEXR::OpenEXRCore)
    LIST(APPEND _openexr_stage_targets OpenEXR::OpenEXRCore)
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS ${_openexr_stage_targets})
ENDIF()
