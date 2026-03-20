#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# IMPORTANT: CMake minimum version need to be increased everytime Boost version is increased. e.g. CMake 3.27 is needed for Boost 1.82 to be found by
# FindBoost.cmake.
#
# Starting from CMake 3.30, FindBoost.cmake has been removed in favor of BoostConfig.cmake (Boost 1.70+). This behavior is covered by CMake policy CMP0167.

SET(_ext_boost_version
    ${RV_DEPS_BOOST_VERSION}
)
SET(_major_minor_version
    ${RV_DEPS_BOOST_MAJOR_MINOR_VERSION}
)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_BOOST" "${_ext_boost_version}" "" "" FORCE_LIB)
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_boost_libs
    atomic
    chrono
    date_time
    filesystem
    graph
    iostreams
    locale
    program_options
    random
    regex
    serialization
    system
    thread
    timer
)

# Build DEPS_LIST_TARGETS from _boost_libs
SET(_boost_deps_list_targets
    ""
)
FOREACH(
  _boost_lib
  ${_boost_libs}
)
  LIST(APPEND _boost_deps_list_targets Boost::${_boost_lib})
ENDFOREACH()

# --- Try to find installed package ---
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  Boost
  VERSION
  ${_ext_boost_version}
  COMPONENTS
  ${_boost_libs}
  DEPS_LIST_TARGETS
  ${_boost_deps_list_targets}
)

# --- Library naming (shared between find and build paths) ---
# Note: Boost has a custom lib naming scheme on windows
IF(RV_TARGET_WINDOWS)
  SET(BOOST_SHARED_LIBRARY_PREFIX
      ""
  )
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(BOOST_LIBRARY_SUFFIX
        "-vc143-mt-gd-x64-${_major_minor_version}"
    )
  ELSE()
    SET(BOOST_LIBRARY_SUFFIX
        "-vc143-mt-x64-${_major_minor_version}"
    )
  ENDIF()
  SET(BOOST_SHARED_LIBRARY_SUFFIX
      ${BOOST_LIBRARY_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(BOOST_IMPORT_LIBRARY_PREFIX
      ""
  )
  SET(BOOST_IMPORT_LIBRARY_SUFFIX
      ${BOOST_LIBRARY_SUFFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
ELSE()
  SET(BOOST_SHARED_LIBRARY_PREFIX
      ${CMAKE_SHARED_LIBRARY_PREFIX}
  )
  SET(BOOST_SHARED_LIBRARY_SUFFIX
      ${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

# Compute per-lib paths AFTER RV_FIND_DEPENDENCY (which may override _lib_dir)
FOREACH(
  _boost_lib
  ${_boost_libs}
)
  SET(_boost_${_boost_lib}_lib_name
      ${BOOST_SHARED_LIBRARY_PREFIX}boost_${_boost_lib}${BOOST_SHARED_LIBRARY_SUFFIX}
  )
  SET(_boost_${_boost_lib}_lib
      ${_lib_dir}/${_boost_${_boost_lib}_lib_name}
  )
  LIST(APPEND _boost_byproducts ${_boost_${_boost_lib}_lib})
  IF(RV_TARGET_WINDOWS)
    SET(_boost_${_boost_lib}_implib
        ${_lib_dir}/${BOOST_IMPORT_LIBRARY_PREFIX}boost_${_boost_lib}${BOOST_IMPORT_LIBRARY_SUFFIX}
    )
    LIST(APPEND _boost_byproducts ${_boost_${_boost_lib}_implib})
  ENDIF()
ENDFOREACH()

# --- Build from source if not found ---
IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/boost.cmake)

  # Build path: we control the filenames, use OUTPUTS for precise tracking
  FOREACH(
    _boost_lib
    ${_boost_libs}
  )
    LIST(APPEND _boost_stage_output ${RV_STAGE_LIB_DIR}/${_boost_${_boost_lib}_lib_name})
  ENDFOREACH()
  # Note: On Windows, Boost's b2 puts both .lib and .dll in lib/, so we copy _lib_dir to both RV_STAGE_LIB_DIR and RV_STAGE_BIN_DIR.
  IF(RV_TARGET_WINDOWS)
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} EXTRA_LIB_DIRS ${RV_STAGE_BIN_DIR} OUTPUTS ${_boost_stage_output})
  ELSE()
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${_boost_stage_output})
  ENDIF()
ELSE()
  # CONFIG found — Boost::xxx targets already exist with proper LOCATION. Create any missing targets as a safety net.
  IF(RV_TARGET_WINDOWS)
    SET(_include_dir
        ${_install_dir}/include/boost-${_major_minor_version}
    )
  ENDIF()

  # Boost::headers is not in DEPS_LIST_TARGETS so the general symlink fixup in RV_FIND_DEPENDENCY doesn't reach it. Fix it here: Boost's config has a symlink
  # resolution bug where get_filename_component(REALPATH) on "../" normalizes before resolving, landing on the shared prefix include dir (e.g.
  # /opt/homebrew/include) instead of the package-specific one.
  IF(TARGET Boost::headers)
    GET_TARGET_PROPERTY(_boost_headers_inc Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
    IF(_boost_headers_inc
       AND NOT "${_boost_headers_inc}" STREQUAL "${_include_dir}"
    )
      SET_TARGET_PROPERTIES(
        Boost::headers
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_include_dir}"
      )
      MESSAGE(STATUS "  Fixed Boost::headers include: ${_boost_headers_inc} -> ${_include_dir}")
    ENDIF()
  ELSE()
    ADD_LIBRARY(Boost::headers INTERFACE IMPORTED GLOBAL)
    SET_TARGET_PROPERTIES(
      Boost::headers
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_include_dir}"
    )
  ENDIF()

  FOREACH(
    _boost_lib
    ${_boost_libs}
  )
    IF(NOT TARGET Boost::${_boost_lib})
      RV_ADD_IMPORTED_LIBRARY(
        NAME
        Boost::${_boost_lib}
        TYPE
        SHARED
        LOCATION
        ${_boost_${_boost_lib}_lib}
        SONAME
        ${_boost_${_boost_lib}_lib_name}
        IMPLIB
        ${_boost_${_boost_lib}_implib}
        INCLUDE_DIRS
        ${_include_dir}
        DEPENDS
        ${_target}
      )
    ENDIF()
  ENDFOREACH()

  # Found path: actual filenames may differ (e.g. -mt suffix), use TARGET_LIBS to resolve at build time
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS ${_boost_deps_list_targets})
ENDIF()
