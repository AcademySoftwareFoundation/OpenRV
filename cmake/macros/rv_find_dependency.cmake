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
#     [ALLOW_MODULE]                 # Also try find_package(MODULE) for packages with built-in Find modules
#     [COMPONENTS <c1>...]           # Optional find_package COMPONENTS (e.g., Boost components)
#     [DEPS_LIST_TARGETS <t1>...]    # Targets to append to RV_DEPS_LIST
#   )
# cmake-format: on
MACRO(RV_FIND_DEPENDENCY)
  CMAKE_PARSE_ARGUMENTS(_RFD "ALLOW_MODULE" "TARGET;PACKAGE;VERSION;PKG_CONFIG_NAME" "DEPS_LIST_TARGETS;COMPONENTS" ${ARGN})

  SET(${_RFD_TARGET}_FOUND
      FALSE
  )
  SET(_RFD_FOUND_VIA
      ""
  )

  IF(RV_DEPS_PREFER_INSTALLED
     AND NOT ${_RFD_TARGET}_FORCE_BUILD
  )

    # Determine version match mode: per-dep override > global default
    IF(DEFINED ${_RFD_TARGET}_VERSION_MATCH)
      SET(_rfd_match_mode
          "${${_RFD_TARGET}_VERSION_MATCH}"
      )
    ELSE()
      SET(_rfd_match_mode
          "${RV_DEPS_VERSION_MATCH}"
      )
    ENDIF()

    # Build optional COMPONENTS argument list
    SET(_rfd_components_args
        ""
    )
    IF(_RFD_COMPONENTS)
      SET(_rfd_components_args
          COMPONENTS ${_RFD_COMPONENTS}
      )
    ENDIF()

    # Strategy 1: CMake CONFIG mode
    IF(_RFD_VERSION)
      IF(_rfd_match_mode STREQUAL "EXACT")
        FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} EXACT CONFIG ${_rfd_components_args})
      ELSE()
        FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} CONFIG ${_rfd_components_args})
      ENDIF()
    ELSE()
      FIND_PACKAGE(${_RFD_PACKAGE} CONFIG ${_rfd_components_args})
    ENDIF()

    IF(${_RFD_PACKAGE}_FOUND)
      SET(_RFD_FOUND_VIA
          "config"
      )
    ENDIF()

    # Strategy 1.5: CMake MODULE mode (built-in Find modules) Some packages (e.g., zlib) don't ship CMake config files but have built-in Find modules
    # (FindZLIB.cmake) that create proper imported targets.  Opt-in via ALLOW_MODULE to avoid unexpected matches.
    IF(NOT ${_RFD_PACKAGE}_FOUND
       AND _RFD_ALLOW_MODULE
    )
      IF(_RFD_VERSION)
        IF(_rfd_match_mode STREQUAL "EXACT")
          FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} EXACT MODULE ${_rfd_components_args})
        ELSE()
          FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} MODULE ${_rfd_components_args})
        ENDIF()
      ELSE()
        FIND_PACKAGE(${_RFD_PACKAGE} MODULE ${_rfd_components_args})
      ENDIF()

      IF(${_RFD_PACKAGE}_FOUND)
        SET(_RFD_FOUND_VIA
            "module"
        )
      ENDIF()
    ENDIF()

    # Strategy 2: pkg-config fallback
    IF(NOT ${_RFD_PACKAGE}_FOUND
       AND _RFD_PKG_CONFIG_NAME
    )
      FIND_PACKAGE(PkgConfig QUIET)
      IF(PKG_CONFIG_FOUND)
        # pkg_check_modules only searches PKG_CONFIG_PATH by default. Temporarily enable CMAKE_PREFIX_PATH integration so that
        # -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/zlib also exposes <prefix>/lib/pkgconfig/*.pc to pkg-config.  Restored afterwards to avoid side-effects on
        # unrelated pkg_check_modules calls.
        SET(_rfd_saved_pc_prefix_path
            "${PKG_CONFIG_USE_CMAKE_PREFIX_PATH}"
        )
        SET(PKG_CONFIG_USE_CMAKE_PREFIX_PATH
            ON
        )

        IF(_RFD_VERSION)
          IF(_rfd_match_mode STREQUAL "EXACT")
            PKG_CHECK_MODULES(${_RFD_TARGET}_PC QUIET IMPORTED_TARGET "${_RFD_PKG_CONFIG_NAME}=${_RFD_VERSION}")
          ELSE()
            PKG_CHECK_MODULES(${_RFD_TARGET}_PC QUIET IMPORTED_TARGET "${_RFD_PKG_CONFIG_NAME}>=${_RFD_VERSION}")
          ENDIF()
        ELSE()
          PKG_CHECK_MODULES(${_RFD_TARGET}_PC QUIET IMPORTED_TARGET ${_RFD_PKG_CONFIG_NAME})
        ENDIF()

        SET(PKG_CONFIG_USE_CMAKE_PREFIX_PATH
            "${_rfd_saved_pc_prefix_path}"
        )

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

        # Package config files may suffer the same symlink resolution bug as our walk-up logic (CMake normalizes "../" before resolving symlinks). If the
        # resolved and unresolved include dirs differ, fix ALL imported targets from this package (not just DEPS_LIST_TARGETS). Config files often create
        # auxiliary interface targets (e.g. Imath::ImathConfig, OpenEXR::OpenEXRConfig) that carry include dirs and are linked transitively by the library
        # targets.
        IF(NOT "${_sfpd_unresolved_include_dir}" STREQUAL "${_include_dir}")
          # Record the contaminating prefix so ExternalProject sub-builds can exclude it via CMAKE_IGNORE_PREFIX_PATH. This is how we propagate the symlink fix
          # to sub-builds that run their own CMake and would otherwise rediscover packages at the unresolved (shared) prefix.
          LIST(APPEND RV_DEPS_IGNORE_PREFIXES "${_sfpd_unresolved_root}")
          LIST(REMOVE_DUPLICATES RV_DEPS_IGNORE_PREFIXES)
          SET(RV_DEPS_IGNORE_PREFIXES
              "${RV_DEPS_IGNORE_PREFIXES}"
              CACHE INTERNAL "Contaminating prefixes detected during symlink resolution (for ExternalProject sub-builds)" FORCE
          )
          MESSAGE(STATUS "  Added ${_sfpd_unresolved_root} to RV_DEPS_IGNORE_PREFIXES (symlink resolved to ${_install_dir})")

          GET_PROPERTY(
            _rfd_all_imported_targets
            DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            PROPERTY IMPORTED_TARGETS
          )
          IF(NOT _rfd_all_imported_targets)
            MESSAGE(WARNING "RV_FIND_DEPENDENCY: IMPORTED_TARGETS is empty for ${_RFD_PACKAGE} — symlink include fixup skipped")
          ENDIF()
          FOREACH(
            _rfd_dep
            ${_rfd_all_imported_targets}
          )
            IF("${_rfd_dep}" MATCHES "^${_RFD_PACKAGE}::")
              GET_TARGET_PROPERTY(_rfd_inc_dirs ${_rfd_dep} INTERFACE_INCLUDE_DIRECTORIES)
              IF(_rfd_inc_dirs)
                STRING(REPLACE "${_sfpd_unresolved_include_dir}" "${_include_dir}" _rfd_fixed_inc_dirs "${_rfd_inc_dirs}")
                IF(NOT "${_rfd_fixed_inc_dirs}" STREQUAL "${_rfd_inc_dirs}")
                  SET_TARGET_PROPERTIES(
                    ${_rfd_dep}
                    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_rfd_fixed_inc_dirs}"
                  )
                  MESSAGE(STATUS "  Fixed ${_rfd_dep} include: ${_rfd_inc_dirs} -> ${_rfd_fixed_inc_dirs}")
                ENDIF()
              ENDIF()
            ENDIF()
          ENDFOREACH()
        ENDIF()
      ELSEIF(_RFD_FOUND_VIA STREQUAL "module")
        MESSAGE(STATUS "Found ${_RFD_PACKAGE} ${${_RFD_PACKAGE}_VERSION} via CMake Find module. Using installed package.")
        RV_SET_FOUND_MODULE_DIRS(${_RFD_TARGET} ${_RFD_PACKAGE} "${_RFD_DEPS_LIST_TARGETS}")
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
