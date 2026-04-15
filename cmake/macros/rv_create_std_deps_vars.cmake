#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Create the standard variables common to most RV_DEPS_xyz modules
MACRO(RV_CREATE_STANDARD_DEPS_VARIABLES target_name version make_command configure_command)

  # Parse optional keyword arguments after the 4 positional params. FORCE_LIB: always use lib/ instead of lib64/ even when RHEL_VERBOSE is set. Use for
  # dependencies whose CMake install does not honor GNUInstallDirs lib64 (e.g. zlib).
  CMAKE_PARSE_ARGUMENTS(_RCSDV "FORCE_LIB" "" "" ${ARGN})

  SET(_target
      ${target_name}
  )
  SET(_base_dir
      ${RV_DEPS_BASE_DIR}/${target_name}
  )
  SET(_install_dir
      ${_base_dir}/install
  )
  # Create commonly used definition when cross-building dependencies RV_DEPS_<target-name>_ROOT_DIR variable
  SET(${target_name}_ROOT_DIR
      ${_install_dir}
  )
  SET(_include_dir
      ${_install_dir}/include
  )
  SET(_bin_dir
      ${_install_dir}/bin
  )
  SET(_build_dir
      ${_base_dir}/build
  )
  SET(_source_dir
      ${_base_dir}/src
  )
  IF(_RCSDV_FORCE_LIB)
    SET(_lib_dir
        ${_install_dir}/lib
    )
  ELSEIF(RHEL_VERBOSE)
    SET(_lib_dir
        ${_install_dir}/lib64
    )
  ELSE()
    SET(_lib_dir
        ${_install_dir}/lib
    )
  ENDIF()

  #
  # Create locally used _version and globally used RV_DEPS_XYZ_VERSION variables.
  #
  SET(_version
      ${version}
  )

  # Attempt to match Major.Minor.Patch CMAKE_MATCH_1 will be Major, CMAKE_MATCH_2 Minor, etc.
  STRING(REGEX MATCH "^([0-9]+)\\.?([0-9]*)\\.?([0-9]*)" _dummy "${_version}")
  # If the group was found, use it; otherwise default to 0
  IF(CMAKE_MATCH_1)
    SET(_version_major
        ${CMAKE_MATCH_1}
    )
  ELSE()
    SET(_version_major
        0
    )
  ENDIF()

  IF(CMAKE_MATCH_2)
    SET(_version_minor
        ${CMAKE_MATCH_2}
    )
  ELSE()
    SET(_version_minor
        0
    )
  ENDIF()

  IF(CMAKE_MATCH_3)
    SET(_version_patch
        ${CMAKE_MATCH_3}
    )
  ELSE()
    SET(_version_patch
        0
    )
  ENDIF()
  SET(${target_name}_VERSION
      ${_version}
      CACHE INTERNAL "" FORCE
  )

  #
  # Create locally used make command
  #
  SET(_make_command
      ${make_command}
  )

  SET(_cmake_build_command
      ${CMAKE_COMMAND} --build ${_build_dir} --config ${CMAKE_BUILD_TYPE} -j${_cpu_count}
  )

  IF(RV_TARGET_WINDOWS)
    # MSYS2/CMake defaults to Ninja
    SET(_make_command
        ninja
    )
  ENDIF()

  #
  # Create locally used configure command
  #
  SET(_configure_command
      ${configure_command}
  )

  #
  # Also reset a number of secondary list variables
  #
  SET(_byproducts
      ""
  )
  SET(_configure_options
      ""
  )
  LIST(APPEND _configure_options "-DCMAKE_INSTALL_PREFIX=${_install_dir}")
  LIST(APPEND _configure_options "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  LIST(APPEND _configure_options "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  LIST(APPEND _configure_options "-S ${_source_dir}")
  LIST(APPEND _configure_options "-B ${_build_dir}")

  SET(_cmake_install_command
      ${CMAKE_COMMAND} --install ${_build_dir} --prefix ${_install_dir} --config ${CMAKE_BUILD_TYPE}
  )

  #
  # Finally add a clean-<target-name> target
  #
  ADD_CUSTOM_TARGET(
    clean-${target_name}
    COMMENT "Cleaning '${target_name}' ..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${_base_dir}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${RV_DEPS_BASE_DIR}/cmake/dependencies/${_target}-prefix
  )
ENDMACRO()

MACRO(RV_SHOW_STANDARD_DEPS_VARIABLES)
  MESSAGE(DEBUG "RV_CREATE_STANDARD_DEPS_VARIABLES:")
  MESSAGE(DEBUG "  _target        ='${_target}'")
  MESSAGE(DEBUG "  _version       ='${_version}'")
  MESSAGE(DEBUG "  _version_major ='${_version_major}'")
  MESSAGE(DEBUG "  _version_minor ='${_version_minor}'")
  MESSAGE(DEBUG "  _version_patch ='${_version_patch}'")
  MESSAGE(DEBUG "  _base_dir      ='${_base_dir}'")
  MESSAGE(DEBUG "  _build_dir     ='${_build_dir}'")
  MESSAGE(DEBUG "  _source_dir    ='${_source_dir}'")
  MESSAGE(DEBUG "  _install_dir   ='${_install_dir}'")
  MESSAGE(DEBUG "  _include_dir   ='${_include_dir}'")
  MESSAGE(DEBUG "  _bin_dir       ='${_bin_dir}'")
  MESSAGE(DEBUG "  _lib_dir       ='${_lib_dir}'")
  MESSAGE(DEBUG "  _make_command      ='${_make_command}'")
  MESSAGE(DEBUG "  _configure_command ='${_configure_command}'")
  MESSAGE(DEBUG "  ${_target}_VERSION='${${_target}_VERSION}'")
  MESSAGE(DEBUG "  ${_target}_ROOT_DIR='${${_target}_ROOT_DIR}'")
ENDMACRO()

#
# RV_RESOLVE_IMPORTED_LOCATION
#
# Resolve an imported target's library path across config variants
#
# MODULE-found targets (e.g. FindZLIB) often only set config-specific variants (IMPORTED_LOCATION_RELEASE, IMPORTED_LOCATION_NOCONFIG) not plain
# IMPORTED_LOCATION. This macro tries each variant and stores the first hit.
#
# For INTERFACE_LIBRARY targets (e.g. vcpkg wrapper targets that aggregate the real library via INTERFACE_LINK_LIBRARIES), resolves one level through
# INTERFACE_LINK_LIBRARIES to find a linked IMPORTED target with a real location.
#
# Must be a MACRO so the output variable propagates to the caller's scope.
#
# Usage: RV_RESOLVE_IMPORTED_LOCATION(<target> <output_variable> [<resolved_target_variable>])
#
# If <resolved_target_variable> is provided, it is set to the target name that owns the resolved location (may differ from <target> for INTERFACE wrappers).
#
MACRO(RV_RESOLVE_IMPORTED_LOCATION _rril_target _rril_out_var)
  SET(${_rril_out_var}
      ""
  )
  # Optional third argument: output variable for the resolved target name
  SET(_rril_resolved_target_var
      "${ARGV2}"
  )
  IF(_rril_resolved_target_var)
    SET(${_rril_resolved_target_var}
        ""
    )
  ENDIF()

  IF(TARGET ${_rril_target})
    STRING(TOUPPER "${CMAKE_BUILD_TYPE}" _rril_config_upper)
    FOREACH(
      _rril_prop
      IMPORTED_LOCATION IMPORTED_LOCATION_${_rril_config_upper} IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_NOCONFIG
    )
      IF(NOT ${_rril_out_var})
        GET_TARGET_PROPERTY(${_rril_out_var} ${_rril_target} ${_rril_prop})
      ENDIF()
    ENDFOREACH()

    IF(${_rril_out_var})
      IF(_rril_resolved_target_var)
        SET(${_rril_resolved_target_var}
            "${_rril_target}"
        )
      ENDIF()
    ELSE()
      # For INTERFACE_LIBRARY targets (e.g. vcpkg wrappers, Conan CMakeDeps), try resolving through INTERFACE_LINK_LIBRARIES to find the real library target.
      # INTERFACE_LINK_LIBRARIES may contain plain target names or generator expressions ($<$<CONFIG:...>:target>). Conan CMakeDeps may create nested INTERFACE
      # chains (e.g. libraw::libraw → libraw::libraw_ → CONAN_LIB::...), so traverse up to 3 levels deep with a visited set to prevent infinite loops (Imath's
      # aggregated target links to itself).
      GET_TARGET_PROPERTY(_rril_type ${_rril_target} TYPE)
      IF(_rril_type STREQUAL "INTERFACE_LIBRARY")
        SET(_rril_queue
            ${_rril_target}
        )
        SET(_rril_visited
            ""
        )
        SET(_rril_depth
            0
        )
        WHILE(
          _rril_queue
          AND NOT ${_rril_out_var}
          AND _rril_depth LESS 3
        )
          SET(_rril_next_queue
              ""
          )
          FOREACH(
            _rril_iface_tgt
            ${_rril_queue}
          )
            IF(${_rril_out_var})
              BREAK()
            ENDIF()
            IF("${_rril_iface_tgt}" IN_LIST _rril_visited)
              CONTINUE()
            ENDIF()
            LIST(APPEND _rril_visited "${_rril_iface_tgt}")
            IF(NOT TARGET ${_rril_iface_tgt})
              CONTINUE()
            ENDIF()
            GET_TARGET_PROPERTY(_rril_link_libs ${_rril_iface_tgt} INTERFACE_LINK_LIBRARIES)
            IF(NOT _rril_link_libs)
              CONTINUE()
            ENDIF()
            RV_EXTRACT_LINK_TARGETS("${_rril_link_libs}" _rril_resolved_libs)
            FOREACH(
              _rril_link_lib
              ${_rril_resolved_libs}
            )
              IF(${_rril_out_var})
                BREAK()
              ENDIF()
              # Try IMPORTED_LOCATION variants on this target
              FOREACH(
                _rril_prop
                IMPORTED_LOCATION IMPORTED_LOCATION_${_rril_config_upper} IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_NOCONFIG
              )
                IF(NOT ${_rril_out_var})
                  GET_TARGET_PROPERTY(${_rril_out_var} ${_rril_link_lib} ${_rril_prop})
                ENDIF()
              ENDFOREACH()
              IF(${_rril_out_var})
                IF(_rril_resolved_target_var)
                  SET(${_rril_resolved_target_var}
                      "${_rril_link_lib}"
                  )
                ENDIF()
              ELSE()
                # If this target is also INTERFACE, queue it for the next depth level
                GET_TARGET_PROPERTY(_rril_link_lib_type ${_rril_link_lib} TYPE)
                IF(_rril_link_lib_type STREQUAL "INTERFACE_LIBRARY")
                  LIST(APPEND _rril_next_queue ${_rril_link_lib})
                ENDIF()
              ENDIF()
            ENDFOREACH()
          ENDFOREACH()
          SET(_rril_queue
              ${_rril_next_queue}
          )
          MATH(EXPR _rril_depth "${_rril_depth} + 1")
        ENDWHILE()
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

#
# RV_RESOLVE_IMPORTED_INCLUDE_DIR: Resolve INTERFACE_INCLUDE_DIRECTORIES, following INTERFACE wrapper chains
#
# Some package managers (vcpkg) create INTERFACE wrapper targets (e.g. PNG::PNG) that only have INTERFACE_LINK_LIBRARIES pointing to the real imported target
# (e.g. PNG::png_shared). INTERFACE_INCLUDE_DIRECTORIES lives on the real target, not the wrapper. This macro follows one level of INTERFACE_LINK_LIBRARIES to
# resolve include dirs when the direct property is empty.
#
# Must be a MACRO so the output variable propagates to the caller's scope.
#
# Usage: RV_RESOLVE_IMPORTED_INCLUDE_DIR(<target> <output_variable>)
#
MACRO(RV_RESOLVE_IMPORTED_INCLUDE_DIR _rriid_target _rriid_out_var)
  SET(${_rriid_out_var}
      ""
  )
  IF(TARGET ${_rriid_target})
    GET_TARGET_PROPERTY(${_rriid_out_var} ${_rriid_target} INTERFACE_INCLUDE_DIRECTORIES)
    IF(NOT ${_rriid_out_var})
      # Follow INTERFACE_LINK_LIBRARIES to find the real target's include dirs. Handles generator expressions from Conan CMakeDeps and similar.
      GET_TARGET_PROPERTY(_rriid_type ${_rriid_target} TYPE)
      IF(_rriid_type STREQUAL "INTERFACE_LIBRARY")
        GET_TARGET_PROPERTY(_rriid_link_libs ${_rriid_target} INTERFACE_LINK_LIBRARIES)
        IF(_rriid_link_libs)
          RV_EXTRACT_LINK_TARGETS("${_rriid_link_libs}" _rriid_resolved_libs)
          FOREACH(
            _rriid_ll
            ${_rriid_resolved_libs}
          )
            GET_TARGET_PROPERTY(${_rriid_out_var} ${_rriid_ll} INTERFACE_INCLUDE_DIRECTORIES)
            IF(${_rriid_out_var})
              BREAK()
            ENDIF()
          ENDFOREACH()
        ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

#
# RV_RESOLVE_IMPORTED_LINKER_FILE: Resolve the file a linker needs for an imported target
#
# On Windows: returns IMPORTED_IMPLIB (.lib import library), falling back to IMPORTED_LOCATION. On other platforms: returns IMPORTED_LOCATION (same as
# RV_RESOLVE_IMPORTED_LOCATION).
#
# Use this instead of RV_RESOLVE_IMPORTED_LOCATION when passing library paths to ExternalProject CMake builds via -D<Pkg>_LIBRARY=..., since those need the
# import library on Windows, not the DLL.
#
# Handles INTERFACE_LIBRARY targets (e.g. vcpkg wrappers) by resolving through INTERFACE_LINK_LIBRARIES.
#
# Must be a MACRO so the output variable propagates to the caller's scope.
#
# Usage: RV_RESOLVE_IMPORTED_LINKER_FILE(<target> <output_variable>)
#
MACRO(RV_RESOLVE_IMPORTED_LINKER_FILE _rrilf_target _rrilf_out_var)
  SET(${_rrilf_out_var}
      ""
  )
  IF(RV_TARGET_WINDOWS
     AND TARGET ${_rrilf_target}
  )
    # Resolve through INTERFACE_LIBRARY wrappers (e.g. vcpkg, Conan CMakeDeps) to find the real library target. Handles generator expressions in
    # INTERFACE_LINK_LIBRARIES.
    SET(_rrilf_real_tgt
        ${_rrilf_target}
    )
    GET_TARGET_PROPERTY(_rrilf_type ${_rrilf_target} TYPE)
    IF(_rrilf_type STREQUAL "INTERFACE_LIBRARY")
      GET_TARGET_PROPERTY(_rrilf_link_libs ${_rrilf_target} INTERFACE_LINK_LIBRARIES)
      IF(_rrilf_link_libs)
        RV_EXTRACT_LINK_TARGETS("${_rrilf_link_libs}" _rrilf_resolved_libs)
        FOREACH(
          _rrilf_ll
          ${_rrilf_resolved_libs}
        )
          GET_TARGET_PROPERTY(_rrilf_ll_type ${_rrilf_ll} TYPE)
          IF(NOT _rrilf_ll_type STREQUAL "INTERFACE_LIBRARY")
            SET(_rrilf_real_tgt
                ${_rrilf_ll}
            )
            BREAK()
          ENDIF()
        ENDFOREACH()
      ENDIF()
    ENDIF()
    # Try IMPORTED_IMPLIB variants on the resolved target
    STRING(TOUPPER "${CMAKE_BUILD_TYPE}" _rrilf_config_upper)
    FOREACH(
      _rrilf_prop
      IMPORTED_IMPLIB IMPORTED_IMPLIB_${_rrilf_config_upper} IMPORTED_IMPLIB_RELEASE IMPORTED_IMPLIB_NOCONFIG
    )
      IF(NOT ${_rrilf_out_var})
        GET_TARGET_PROPERTY(${_rrilf_out_var} ${_rrilf_real_tgt} ${_rrilf_prop})
      ENDIF()
    ENDFOREACH()
  ENDIF()
  # Fall back to IMPORTED_LOCATION (always used on non-Windows, or if no IMPLIB found)
  IF(NOT ${_rrilf_out_var})
    RV_RESOLVE_IMPORTED_LOCATION(${_rrilf_target} ${_rrilf_out_var})
  ENDIF()
ENDMACRO()

#
# RV_RESOLVE_DARWIN_INSTALL_NAME: Get a macOS dylib's actual install name (LC_ID_DYLIB)
#
# On macOS, the install name recorded in a binary by the linker comes from the library's LC_ID_DYLIB, which may differ from the file path (e.g. Homebrew symlink
# paths). This macro resolves the real install name via otool -D and caches the result as a target property (RV_DARWIN_INSTALL_NAME) for later use in rpath
# fixup.
#
# For built-from-source deps where the library doesn't exist at configure time, this is a no-op.
#
MACRO(RV_RESOLVE_DARWIN_INSTALL_NAME _rdain_target)
  IF(RV_TARGET_DARWIN
     AND TARGET ${_rdain_target}
  )
    RV_RESOLVE_IMPORTED_LOCATION(${_rdain_target} _rdain_loc)
    IF(_rdain_loc
       AND EXISTS "${_rdain_loc}"
    )
      EXECUTE_PROCESS(
        COMMAND otool -D "${_rdain_loc}"
        OUTPUT_VARIABLE _rdain_otool_out
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
      )
      # otool -D output: first line is file path, second line is the install name
      STRING(REGEX MATCH "[^\n]+$" _rdain_install_name "${_rdain_otool_out}")
      IF(_rdain_install_name)
        SET_PROPERTY(
          TARGET ${_rdain_target}
          PROPERTY RV_DARWIN_INSTALL_NAME "${_rdain_install_name}"
        )
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

#
# RV_MAKE_TARGETS_GLOBAL: Promote imported targets to GLOBAL visibility
#
# Imported targets created by find_package(CONFIG) are scoped to the calling directory. This promotes them to GLOBAL so all subdirectories can reference them.
#
MACRO(RV_MAKE_TARGETS_GLOBAL)
  FOREACH(
    _rmtg_target
    ${ARGN}
  )
    IF(TARGET ${_rmtg_target})
      SET_PROPERTY(
        TARGET ${_rmtg_target}
        PROPERTY IMPORTED_GLOBAL TRUE
      )
    ENDIF()
  ENDFOREACH()
ENDMACRO()

#
# RV_EXTRACT_LINK_TARGETS: Extract real target names from INTERFACE_LINK_LIBRARIES
#
# INTERFACE_LINK_LIBRARIES may contain plain target names ("Foo::Foo") or generator-expression-wrapped names ("$<$<CONFIG:Release>:CONAN_LIB::foo_RELEASE>").
# This function extracts all items that resolve to existing CMake targets, regardless of wrapping. Non-target items (library paths, flags, unrecognised genexes)
# are silently skipped.
#
# Generic: handles Conan CMakeDeps, vcpkg, and standard CMake export patterns.
#
# NOTE: This is a FUNCTION (has its own scope). When called from a MACRO, PARENT_SCOPE sets the output variable in the macro's caller's scope, which is the same
# scope where the macro's variables live. This is correct but subtle; do not change callers from MACRO to FUNCTION without verifying variable propagation.
#
# Usage: RV_EXTRACT_LINK_TARGETS(<input_list> <output_variable>)
#
FUNCTION(RV_EXTRACT_LINK_TARGETS _relt_input _relt_out_var)
  SET(_relt_result)
  FOREACH(
    _relt_item
    ${_relt_input}
  )
    IF(TARGET ${_relt_item})
      LIST(APPEND _relt_result ${_relt_item})
    ELSE()
      # Parse $<$<CONFIG:...>:target_name> patterns (Conan CMakeDeps, standard CMake exports)
      STRING(REGEX MATCHALL "\\$<\\$<CONFIG:[^>]+>:([^>$][^>]*)>" _relt_matches "${_relt_item}")
      FOREACH(
        _relt_match
        ${_relt_matches}
      )
        STRING(
          REGEX
          REPLACE "\\$<\\$<CONFIG:[^>]+>:([^>]+)>" "\\1" _relt_name "${_relt_match}"
        )
        IF(_relt_name
           AND TARGET ${_relt_name}
        )
          LIST(APPEND _relt_result ${_relt_name})
        ENDIF()
      ENDFOREACH()
      # Also handle $<TARGET_NAME:target> pattern
      STRING(REGEX MATCHALL "\\$<TARGET_NAME:([^>]+)>" _relt_tn_matches "${_relt_item}")
      FOREACH(
        _relt_tn_match
        ${_relt_tn_matches}
      )
        STRING(
          REGEX
          REPLACE "\\$<TARGET_NAME:([^>]+)>" "\\1" _relt_tn_name "${_relt_tn_match}"
        )
        IF(_relt_tn_name
           AND TARGET ${_relt_tn_name}
        )
          LIST(APPEND _relt_result ${_relt_tn_name})
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDFOREACH()
  IF(_relt_result)
    LIST(REMOVE_DUPLICATES _relt_result)
  ENDIF()
  SET(${_relt_out_var}
      ${_relt_result}
      PARENT_SCOPE
  )
ENDFUNCTION()

#
# RV_COLLECT_ALL_LIBRARY_TARGETS: Recursively collect all non-INTERFACE library targets from a dependency chain
#
# Traverses INTERFACE_LINK_LIBRARIES of each input target (handling generator expressions via RV_EXTRACT_LINK_TARGETS) and collects every IMPORTED
# SHARED/STATIC/UNKNOWN/MODULE target. Visits each target at most once (cycle-safe). This captures all transitive library components that a package manager
# (Conan, vcpkg) creates, even ones not explicitly listed in DEPS_LIST_TARGETS.
#
# Usage: RV_COLLECT_ALL_LIBRARY_TARGETS(<input_target_list> <output_variable>)
#
FUNCTION(RV_COLLECT_ALL_LIBRARY_TARGETS _rcalt_input _rcalt_out_var)
  SET(_rcalt_result)
  SET(_rcalt_visited)
  SET(_rcalt_queue
      ${_rcalt_input}
  )

  WHILE(_rcalt_queue)
    LIST(POP_FRONT _rcalt_queue _rcalt_current)
    IF("${_rcalt_current}" IN_LIST _rcalt_visited)
      CONTINUE()
    ENDIF()
    LIST(APPEND _rcalt_visited "${_rcalt_current}")

    IF(NOT TARGET ${_rcalt_current})
      CONTINUE()
    ENDIF()

    GET_TARGET_PROPERTY(_rcalt_type ${_rcalt_current} TYPE)
    IF(_rcalt_type STREQUAL "INTERFACE_LIBRARY")
      # Traverse INTERFACE_LINK_LIBRARIES to find real targets
      GET_TARGET_PROPERTY(_rcalt_libs ${_rcalt_current} INTERFACE_LINK_LIBRARIES)
      IF(_rcalt_libs)
        RV_EXTRACT_LINK_TARGETS("${_rcalt_libs}" _rcalt_extracted)
        LIST(APPEND _rcalt_queue ${_rcalt_extracted})
      ENDIF()
    ELSEIF(_rcalt_type MATCHES "SHARED_LIBRARY|STATIC_LIBRARY|UNKNOWN_LIBRARY|MODULE_LIBRARY")
      # Real library target: collect it
      LIST(APPEND _rcalt_result ${_rcalt_current})
    ENDIF()
  ENDWHILE()

  IF(_rcalt_result)
    LIST(REMOVE_DUPLICATES _rcalt_result)
  ENDIF()
  SET(${_rcalt_out_var}
      ${_rcalt_result}
      PARENT_SCOPE
  )
ENDFUNCTION()

#
# RV_SET_FOUND_PACKAGE_DIRS

# Set directory variables from a found package
#
# When a dependency is found via find_package() instead of built from source, the _lib_dir, _bin_dir, _include_dir, and _install_dir variables set by
# RV_CREATE_STANDARD_DEPS_VARIABLES point at the (non-existent) ExternalProject install dir. This macro overrides them to point at the found package's
# actuallocation, so downstream code (including RV_STAGE_DEPENDENCY_LIBS) works identically for both paths.
#
# Also creates a dummy custom target for the dependency so that DEPENDS ${_target} works in the find path where there's no ExternalProject target.
#
# Must be a MACRO so the variable overrides propagate to the caller's scope.
#
MACRO(RV_SET_FOUND_PACKAGE_DIRS rv_deps_target find_package_name)
  # Optional: DEPS_LIST_TARGETS <t1> <t2> ... (imported targets for root validation). When provided, the walk-up root is validated against actual
  # IMPORTED_LOCATION paths.
  CMAKE_PARSE_ARGUMENTS(_SFPD "" "" "DEPS_LIST_TARGETS" ${ARGN})

  # ${find_package_name}_DIR is set by find_package(CONFIG) to the directory containing the CMake config files, typically: <root>/lib/cmake/<Package>/   (3
  # levels up) <root>/share/cmake/<Package>/ (3 levels up)
  #
  # On macOS, resolve through symlinks FIRST, then walk up. This is critical for Homebrew which symlinks cmake config dirs into a shared prefix (e.g.,
  # /opt/homebrew/lib/cmake/Boost-1.85.0 → Cellar/boost@1.85/.../lib/cmake/Boost-1.85.0). Without REALPATH, walking up lands on the shared prefix
  # (/opt/homebrew) instead of the package-specific root, causing include path contamination. TODO: Verify this is also needed on Linux when testing with system
  # packages.
  IF(APPLE)
    GET_FILENAME_COMPONENT(_sfpd_config_dir "${${find_package_name}_DIR}" REALPATH)
  ELSE()
    GET_FILENAME_COMPONENT(_sfpd_config_dir "${${find_package_name}_DIR}" ABSOLUTE)
  ENDIF()

  # Walk up from the resolved config dir until we find a directory with include/ or lib/. This handles any config layout: Homebrew/Conan:
  # <root>/lib/cmake/<Package>/  (3 levels) vcpkg:          <root>/share/<Package>/       (2 levels)
  SET(_sfpd_root
      "${_sfpd_config_dir}"
  )
  FOREACH(
    _sfpd_i
    RANGE 1 4
  )
    GET_FILENAME_COMPONENT(_sfpd_root "${_sfpd_root}/.." ABSOLUTE)
    IF(EXISTS "${_sfpd_root}/include"
       OR EXISTS "${_sfpd_root}/lib"
    )
      BREAK()
    ENDIF()
  ENDFOREACH()

  # Validate the resolved root against actual target locations. Conan CMakeDeps puts config files in a flat generators/ folder; the walk-up may land on the
  # build tree root (which falsely has include/ and lib/) instead of the actual package root.
  IF(_SFPD_DEPS_LIST_TARGETS)
    SET(_sfpd_validated
        FALSE
    )
    FOREACH(
      _sfpd_tgt
      ${_SFPD_DEPS_LIST_TARGETS}
    )
      RV_RESOLVE_IMPORTED_LOCATION(${_sfpd_tgt} _sfpd_tgt_loc)
      IF(_sfpd_tgt_loc)
        # On macOS, resolve symlinks in IMPORTED_LOCATION before comparing against the walk-up root (which was already resolved via REALPATH). Homebrew's
        # IMPORTED_LOCATION uses the symlinked path (/opt/homebrew/lib/libfoo.dylib) but the walk-up root is the Cellar path.
        IF(APPLE)
          GET_FILENAME_COMPONENT(_sfpd_tgt_loc "${_sfpd_tgt_loc}" REALPATH)
        ENDIF()
        STRING(FIND "${_sfpd_tgt_loc}" "${_sfpd_root}" _sfpd_find_idx)
        IF(_sfpd_find_idx EQUAL 0)
          SET(_sfpd_validated
              TRUE
          )
          BREAK()
        ELSE()
          # Derive root from the actual library location
          GET_FILENAME_COMPONENT(_sfpd_loc_dir "${_sfpd_tgt_loc}" DIRECTORY)
          GET_FILENAME_COMPONENT(_sfpd_root "${_sfpd_loc_dir}/.." ABSOLUTE)
          SET(_sfpd_validated
              TRUE
          )
          MESSAGE(STATUS "  Walk-up root did not contain ${_sfpd_tgt_loc}; derived root from IMPORTED_LOCATION: ${_sfpd_root}")
          BREAK()
        ENDIF()
      ENDIF()
    ENDFOREACH()
    IF(NOT _sfpd_validated)
      MESSAGE(AUTHOR_WARNING "RV_SET_FOUND_PACKAGE_DIRS: Could not validate walk-up root ${_sfpd_root} against any target IMPORTED_LOCATION")
    ENDIF()
  ENDIF()

  SET(_install_dir
      "${_sfpd_root}"
  )

  # Same walk-up for the unresolved path (without symlink resolution) so callers can detect and fix imported targets whose config files point to a shared
  # symlink prefix (Homebrew). On non-symlinked systems (vcpkg, Conan) both paths resolve identically.
  GET_FILENAME_COMPONENT(_sfpd_config_dir_unresolved "${${find_package_name}_DIR}" ABSOLUTE)
  SET(_sfpd_unresolved_root
      "${_sfpd_config_dir_unresolved}"
  )
  FOREACH(
    _sfpd_i
    RANGE 1 4
  )
    GET_FILENAME_COMPONENT(_sfpd_unresolved_root "${_sfpd_unresolved_root}/.." ABSOLUTE)
    IF(EXISTS "${_sfpd_unresolved_root}/include"
       OR EXISTS "${_sfpd_unresolved_root}/lib"
    )
      BREAK()
    ENDIF()
  ENDFOREACH()

  SET(_sfpd_unresolved_include_dir
      "${_sfpd_unresolved_root}/include"
  )

  # Set both the regular variable (to override the one from RV_CREATE_STANDARD_DEPS_VARIABLES that would otherwise shadow the cache) and the cache variable (for
  # persistence across reconfigures).
  SET(${rv_deps_target}_ROOT_DIR
      "${_install_dir}"
  )
  SET(${rv_deps_target}_ROOT_DIR
      "${_install_dir}"
      CACHE INTERNAL "" FORCE
  )
  SET(_include_dir
      "${_install_dir}/include"
  )
  SET(_bin_dir
      "${_install_dir}/bin"
  )
  IF(RHEL_VERBOSE
     AND EXISTS "${_install_dir}/lib64"
  )
    SET(_lib_dir
        "${_install_dir}/lib64"
    )
  ELSE()
    SET(_lib_dir
        "${_install_dir}/lib"
    )
  ENDIF()

  IF(NOT TARGET ${rv_deps_target})
    ADD_CUSTOM_TARGET(${rv_deps_target})
  ENDIF()
ENDMACRO()

#
# RV_SET_FOUND_PKGCONFIG_DIRS

# Set directory variables from a pkg-config found package
#
# Parallel to RV_SET_FOUND_PACKAGE_DIRS but uses variables set by pkg_check_modules() instead of find_package(CONFIG). Sets _lib_dir, _include_dir,
# _install_dir, _bin_dir in the caller's scope and creates a dummy custom target. _install_dir, _bin_dir in the caller's scope and creates a dummy custom
# target.
#
# Must be a MACRO so the variable overrides propagate to the caller's scope.
#
MACRO(RV_SET_FOUND_PKGCONFIG_DIRS rv_deps_target pc_prefix)
  LIST(GET ${pc_prefix}_LIBRARY_DIRS 0 _lib_dir)
  LIST(GET ${pc_prefix}_INCLUDE_DIRS 0 _include_dir)

  GET_FILENAME_COMPONENT(_install_dir "${_lib_dir}/.." ABSOLUTE)
  SET(_bin_dir
      "${_install_dir}/bin"
  )

  SET(${rv_deps_target}_ROOT_DIR
      "${_install_dir}"
  )
  SET(${rv_deps_target}_ROOT_DIR
      "${_install_dir}"
      CACHE INTERNAL "" FORCE
  )

  IF(NOT TARGET ${rv_deps_target})
    ADD_CUSTOM_TARGET(${rv_deps_target})
  ENDIF()
ENDMACRO()

#
# RV_SET_FOUND_MODULE_DIRS
#
# Set directory variables from a package found via CMake MODULE mode (built-in Find modules like FindZLIB, FindJPEG, etc.).
#
# Unlike CONFIG mode packages that provide ${Package}_DIR, MODULE mode results derive locations from the imported target's properties and standard
# <Package>_INCLUDE_DIRS / <Package>_LIBRARIES variables.
#
# Must be a MACRO so the variable overrides propagate to the caller's scope.
#
MACRO(RV_SET_FOUND_MODULE_DIRS rv_deps_target find_package_name deps_list_targets)
  SET(_sfmd_inc_dir
      ""
  )
  SET(_sfmd_lib_dir
      ""
  )

  # Derive include dir: prefer <Package>_INCLUDE_DIRS, fall back to first target's INTERFACE_INCLUDE_DIRECTORIES
  IF(${find_package_name}_INCLUDE_DIRS)
    LIST(GET ${find_package_name}_INCLUDE_DIRS 0 _sfmd_inc_dir)
  ELSE()
    SET(_sfmd_dep_targets
        ${deps_list_targets}
    )
    LIST(LENGTH _sfmd_dep_targets _sfmd_dep_count)
    IF(_sfmd_dep_count GREATER 0)
      LIST(GET _sfmd_dep_targets 0 _sfmd_primary_tgt)
      IF(TARGET ${_sfmd_primary_tgt})
        GET_TARGET_PROPERTY(_sfmd_inc_dir ${_sfmd_primary_tgt} INTERFACE_INCLUDE_DIRECTORIES)
      ENDIF()
    ENDIF()
  ENDIF()

  # Derive lib dir from the first imported target's IMPORTED_LOCATION.
  SET(_sfmd_dep_targets
      ${deps_list_targets}
  )
  LIST(LENGTH _sfmd_dep_targets _sfmd_dep_count)
  IF(_sfmd_dep_count GREATER 0)
    LIST(GET _sfmd_dep_targets 0 _sfmd_primary_tgt)
    RV_RESOLVE_IMPORTED_LOCATION(${_sfmd_primary_tgt} _sfmd_loc)
    IF(_sfmd_loc)
      # On macOS, resolve through symlinks to get the real package root. Homebrew symlinks libraries into a shared prefix (e.g. /opt/homebrew/lib) which causes
      # include path contamination if we derive _include_dir from the unresolved path. TODO: Verify this is also needed on Linux when testing with system
      # packages (e.g. Debian's /usr/lib/x86_64-linux-gnu symlinks).
      IF(APPLE)
        GET_FILENAME_COMPONENT(_sfmd_loc "${_sfmd_loc}" REALPATH)
      ENDIF()
      GET_FILENAME_COMPONENT(_sfmd_lib_dir "${_sfmd_loc}" DIRECTORY)
    ENDIF()
  ENDIF()

  IF(_sfmd_lib_dir)
    SET(_lib_dir
        "${_sfmd_lib_dir}"
    )
    IF(APPLE)
      # Derive include dir from the resolved install root rather than trusting <Package>_INCLUDE_DIRS which may be the unresolved Homebrew shared prefix (e.g.
      # /opt/homebrew/include instead of /opt/homebrew/Cellar/libpng/1.6.55/include).
      GET_FILENAME_COMPONENT(_sfmd_resolved_root "${_sfmd_lib_dir}/.." ABSOLUTE)
      IF(EXISTS "${_sfmd_resolved_root}/include")
        SET(_sfmd_inc_dir
            "${_sfmd_resolved_root}/include"
        )
      ENDIF()
    ENDIF()
  ENDIF()

  IF(_sfmd_inc_dir)
    SET(_include_dir
        "${_sfmd_inc_dir}"
    )
  ENDIF()

  GET_FILENAME_COMPONENT(_install_dir "${_lib_dir}/.." ABSOLUTE)
  SET(_bin_dir
      "${_install_dir}/bin"
  )

  SET(${rv_deps_target}_ROOT_DIR
      "${_install_dir}"
  )
  SET(${rv_deps_target}_ROOT_DIR
      "${_install_dir}"
      CACHE INTERNAL "" FORCE
  )

  IF(NOT TARGET ${rv_deps_target})
    ADD_CUSTOM_TARGET(${rv_deps_target})
  ENDIF()
ENDMACRO()

#
# RV_PRINT_PACKAGE_INFO: Print diagnostic info for a found package
#
MACRO(RV_PRINT_PACKAGE_INFO package_name)
  MESSAGE(STATUS "  ${package_name} package info:")
  IF(DEFINED ${package_name}_VERSION)
    MESSAGE(STATUS "    Version:    ${${package_name}_VERSION}")
  ENDIF()
  IF(DEFINED ${package_name}_DIR)
    MESSAGE(STATUS "    Config dir: ${${package_name}_DIR}")
  ENDIF()
  MESSAGE(STATUS "    Root:       ${_install_dir}")
  MESSAGE(STATUS "    Include:    ${_include_dir}")
  MESSAGE(STATUS "    Lib:        ${_lib_dir}")
  MESSAGE(STATUS "    Bin:        ${_bin_dir}")
ENDMACRO()

#
# RV_BUILD_PKG_CONFIG_PATH: Build a filtered, priority-ordered pkg-config search path string
#
# Constructs a colon-separated path for use with ExternalProject builds (FFmpeg, etc.) that run ./configure or other tools needing pkg-config. The result is
# intended to be assigned to PKG_CONFIG_LIBDIR (which *replaces* the built-in search path) rather than PKG_CONFIG_PATH (which only prepends). This prevents
# MSYS2 MinGW system packages from being found, which would inject incompatible MinGW headers into MSVC compile tests.
#
# The caller is responsible for also clearing PKG_CONFIG_PATH in the child environment (e.g. cmake -E env "PKG_CONFIG_PATH="), since PKG_CONFIG_PATH takes
# priority over PKG_CONFIG_LIBDIR and inherited MSYS2 paths would shadow the correct entries.
#
# Priority order (highest first): 1. Directories passed via EXTRA_DIRS 2. Directories collected in the RV_DEPS_PKG_CONFIG_PATH global property (set by found
# dependencies via RV_FIND_DEPENDENCY) 3. $ENV{PKG_CONFIG_PATH}, filtered to exclude prefixes in CMAKE_IGNORE_PREFIX_PATH (e.g. msys64/mingw64)
#
# On Windows, all paths are converted from C:/... to /C/... (MSYS format) since pkg-config expects Unix-style paths.
#
# cmake-format: off
# Usage:
#   RV_BUILD_PKG_CONFIG_PATH(<output_var> [EXTRA_DIRS <dir1> [dir2...]])
# cmake-format: on
FUNCTION(RV_BUILD_PKG_CONFIG_PATH _rbpcp_out_var)
  CMAKE_PARSE_ARGUMENTS(_RBPCP "" "" "EXTRA_DIRS" ${ARGN})

  SET(_rbpcp_dirs)

  # 1. Caller-supplied extra dirs (highest priority)
  IF(_RBPCP_EXTRA_DIRS)
    LIST(APPEND _rbpcp_dirs ${_RBPCP_EXTRA_DIRS})
  ENDIF()

  # 1. Dirs from found dependencies
  GET_PROPERTY(
    _rbpcp_dep_dirs GLOBAL
    PROPERTY "RV_DEPS_PKG_CONFIG_PATH"
  )
  IF(_rbpcp_dep_dirs)
    LIST(APPEND _rbpcp_dirs ${_rbpcp_dep_dirs})
  ENDIF()

  # 1. Environment PKG_CONFIG_PATH, filtered by CMAKE_IGNORE_PREFIX_PATH
  SET(_rbpcp_env_path
      $ENV{PKG_CONFIG_PATH}
  )
  FOREACH(
    _rbpcp_env_element IN
    LISTS _rbpcp_env_path
  )
    SET(_rbpcp_ignored
        FALSE
    )
    FOREACH(
      _rbpcp_ignore_prefix IN
      LISTS CMAKE_IGNORE_PREFIX_PATH
    )
      STRING(FIND "${_rbpcp_env_element}" "${_rbpcp_ignore_prefix}" _rbpcp_ignore_idx)
      IF(_rbpcp_ignore_idx EQUAL 0)
        SET(_rbpcp_ignored
            TRUE
        )
        BREAK()
      ENDIF()
    ENDFOREACH()
    IF(NOT _rbpcp_ignored)
      LIST(APPEND _rbpcp_dirs "${_rbpcp_env_element}")
    ENDIF()
  ENDFOREACH()

  # On Windows, convert C:/... to /C/... (MSYS format) since pkg-config expects Unix-style paths.
  SET(_rbpcp_result)
  IF(RV_TARGET_WINDOWS)
    FOREACH(
      _rbpcp_element IN
      LISTS _rbpcp_dirs
    )
      STRING(REPLACE "\\" "/" _rbpcp_element "${_rbpcp_element}")
      STRING(REPLACE ":" "" _rbpcp_element "${_rbpcp_element}")
      STRING(FIND "${_rbpcp_element}" "/" _rbpcp_slash_idx)
      IF(_rbpcp_slash_idx GREATER 0)
        STRING(PREPEND _rbpcp_element "/")
      ENDIF()
      LIST(APPEND _rbpcp_result "${_rbpcp_element}")
    ENDFOREACH()
  ELSE()
    SET(_rbpcp_result
        ${_rbpcp_dirs}
    )
  ENDIF()

  LIST(JOIN _rbpcp_result ":" _rbpcp_joined)
  SET(${_rbpcp_out_var}
      "${_rbpcp_joined}"
      PARENT_SCOPE
  )
ENDFUNCTION()
