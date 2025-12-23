#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Create the standard variables common to most RV_DEPS_xyz modules
MACRO(RV_CREATE_STANDARD_DEPS_VARIABLES target_name version make_command configure_command)

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
  IF(RHEL_VERBOSE)
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
