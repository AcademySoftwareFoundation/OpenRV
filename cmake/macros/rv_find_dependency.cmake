#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# RV_FIND_DEPENDENCY — Try to find a pre-built package, set ${TARGET}_FOUND
#
# Checks RV_DEPS_PREFER_INSTALLED and per-dep RV_DEPS_<NAME>_FORCE_BUILD. Tries find_package(CONFIG) first, then falls back to pkg-config if PKG_CONFIG_NAME is
# set. If found: sets up directory vars, adds to RV_DEPS_LIST. If not found or finding disabled: sets ${TARGET}_FOUND=FALSE (caller handles fallback).
#
# Imported targets from find_package(CONFIG) are automatically GLOBAL via CMAKE_FIND_PACKAGE_TARGETS_GLOBAL (set in cmake/dependencies/CMakeLists.txt). For
# pkg-config, the caller is responsible for creating proper imported targets (with LOCATION) after this macro returns — INTERFACE targets won't work because
# rv_stage.cmake expects LOCATION on all RV_DEPS_LIST entries.
#
# Must be a MACRO because RV_SET_FOUND_PACKAGE_DIRS / RV_SET_FOUND_PKGCONFIG_DIRS set _lib_dir, _bin_dir, _include_dir in the caller's scope via text
# substitution.
#
# cmake-format: off
# Usage:
#   RV_FIND_DEPENDENCY(
#     TARGET <rv-deps-target>        # REQUIRED: e.g., RV_DEPS_IMATH
#     PACKAGE <find-package-name>    # REQUIRED: e.g., Imath
#     [VERSION <version>]            # Optional version (EXACT or MINIMUM per RV_DEPS_VERSION_MATCH)
#     [PKG_CONFIG_NAME <name>]       # Optional pkg-config module name for fallback
#     [DEPS_LIST_TARGETS <t1>...]    # Targets to append to RV_DEPS_LIST
#   )
# cmake-format: on
MACRO(RV_FIND_DEPENDENCY)
  CMAKE_PARSE_ARGUMENTS(_RFD "" "TARGET;PACKAGE;VERSION;PKG_CONFIG_NAME" "DEPS_LIST_TARGETS" ${ARGN})

  SET(${_RFD_TARGET}_FOUND
      FALSE
  )
  SET(_RFD_FOUND_VIA
      ""
  )

  IF(RV_DEPS_PREFER_INSTALLED
     AND NOT RV_DEPS_${_RFD_TARGET}_FORCE_BUILD
  )

    # Determine version match mode: per-dep override > global default
    IF(DEFINED RV_DEPS_${_RFD_TARGET}_VERSION_MATCH)
      SET(_rfd_match_mode
          "${RV_DEPS_${_RFD_TARGET}_VERSION_MATCH}"
      )
    ELSE()
      SET(_rfd_match_mode
          "${RV_DEPS_VERSION_MATCH}"
      )
    ENDIF()

    # Strategy 1: CMake CONFIG mode
    IF(_RFD_VERSION)
      IF(_rfd_match_mode STREQUAL "EXACT")
        FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} EXACT CONFIG)
      ELSE()
        FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} CONFIG)
      ENDIF()
    ELSE()
      FIND_PACKAGE(${_RFD_PACKAGE} CONFIG)
    ENDIF()

    IF(${_RFD_PACKAGE}_FOUND)
      SET(_RFD_FOUND_VIA
          "config"
      )
    ENDIF()

    # Strategy 2: pkg-config fallback
    IF(NOT ${_RFD_PACKAGE}_FOUND
       AND _RFD_PKG_CONFIG_NAME
    )
      FIND_PACKAGE(PkgConfig QUIET)
      IF(PKG_CONFIG_FOUND)
        IF(_RFD_VERSION)
          IF(_rfd_match_mode STREQUAL "EXACT")
            PKG_CHECK_MODULES(${_RFD_TARGET}_PC QUIET IMPORTED_TARGET "${_RFD_PKG_CONFIG_NAME}=${_RFD_VERSION}")
          ELSE()
            PKG_CHECK_MODULES(${_RFD_TARGET}_PC QUIET IMPORTED_TARGET "${_RFD_PKG_CONFIG_NAME}>=${_RFD_VERSION}")
          ENDIF()
        ELSE()
          PKG_CHECK_MODULES(${_RFD_TARGET}_PC QUIET IMPORTED_TARGET ${_RFD_PKG_CONFIG_NAME})
        ENDIF()

        IF(${_RFD_TARGET}_PC_FOUND)
          SET(${_RFD_PACKAGE}_FOUND
              TRUE
          )
          SET(${_RFD_PACKAGE}_VERSION
              "${${_RFD_TARGET}_PC_VERSION}"
          )
          SET(_RFD_FOUND_VIA
              "pkgconfig"
          )
        ENDIF()
      ENDIF()
    ENDIF()

    # Common handling for found packages
    IF(${_RFD_PACKAGE}_FOUND)
      SET(${_RFD_TARGET}_FOUND
          TRUE
      )

      IF(_RFD_FOUND_VIA STREQUAL "config")
        MESSAGE(STATUS "Found ${_RFD_PACKAGE} ${${_RFD_PACKAGE}_VERSION} via CMake config. Using installed package.")
        RV_SET_FOUND_PACKAGE_DIRS(${_RFD_TARGET} ${_RFD_PACKAGE})
      ELSEIF(_RFD_FOUND_VIA STREQUAL "pkgconfig")
        MESSAGE(STATUS "Found ${_RFD_PACKAGE} ${${_RFD_PACKAGE}_VERSION} via pkg-config. Using installed package.")
        RV_SET_FOUND_PKGCONFIG_DIRS(${_RFD_TARGET} ${_RFD_TARGET}_PC)
      ENDIF()

      FOREACH(
        _rfd_dep
        ${_RFD_DEPS_LIST_TARGETS}
      )
        LIST(APPEND RV_DEPS_LIST ${_rfd_dep})
        RV_RESOLVE_DARWIN_INSTALL_NAME(${_rfd_dep})
      ENDFOREACH()

      STRING(TOUPPER ${_RFD_PACKAGE} _RFD_PKG_UPPER)
      SET(RV_DEPS_${_RFD_PKG_UPPER}_VERSION
          ${${_RFD_PACKAGE}_VERSION}
          CACHE INTERNAL "" FORCE
      )

      RV_PRINT_PACKAGE_INFO(${_RFD_PACKAGE})
    ELSE()
      MESSAGE(STATUS "${_RFD_PACKAGE} ${_RFD_VERSION} not found. It will be build from source")
    ENDIF()
  ENDIF()
ENDMACRO()
