#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# ##############################################################################################################################################################
#
# Expose OCIO's own 'yaml_cpp' target replacing the legacy 'src/pub/yaml_cpp' folder.
#
# ##############################################################################################################################################################
ADD_LIBRARY(yaml_cpp UNKNOWN IMPORTED GLOBAL)
ADD_DEPENDENCIES(yaml_cpp RV_DEPS_OCIO)
IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
  # Here the postfix is "d" and not "_d": the postfix inside OCIO is: "d".
  SET(_debug_postfix
      "d"
  )
  MESSAGE(DEBUG "Using debug postfix: '${_debug_postfix}'")
ELSE()
  SET(_debug_postfix
      ""
  )
ENDIF()

IF(RHEL_VERBOSE)
  SET(_lib_dir
      ${RV_DEPS_OCIO_DIST_DIR}/lib64
  )
ELSE()
  SET(_lib_dir
      ${RV_DEPS_OCIO_DIST_DIR}/lib
  )
ENDIF()

SET(_ocio_yaml_cpp_libpath
    ${_lib_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${_debug_postfix}${CMAKE_STATIC_LIBRARY_SUFFIX}
)

# When OCIO_INSTALL_EXT_PACKAGES=MISSING and a system yaml-cpp (e.g. Homebrew) was found during OCIO's configure,
# OCIO does not install yaml-cpp into ext/dist. Fall back to the system-installed dylib so downstream targets
# compile and link. The correct long-term fix is OCIO_INSTALL_EXT_PACKAGES=ALL (see ocio.cmake).
IF(NOT EXISTS "${_ocio_yaml_cpp_libpath}")
  FIND_PACKAGE(yaml-cpp QUIET)
  IF(yaml-cpp_FOUND)
    MESSAGE(STATUS "yaml_cpp: OCIO did not vendor yaml-cpp; falling back to system yaml-cpp (${yaml-cpp_VERSION})")
    GET_TARGET_PROPERTY(_system_yaml_cpp_loc yaml-cpp::yaml-cpp IMPORTED_LOCATION)
    IF(NOT _system_yaml_cpp_loc)
      GET_TARGET_PROPERTY(_system_yaml_cpp_loc yaml-cpp::yaml-cpp IMPORTED_LOCATION_RELEASE)
    ENDIF()
    IF(NOT _system_yaml_cpp_loc)
      GET_TARGET_PROPERTY(_system_yaml_cpp_loc yaml-cpp::yaml-cpp IMPORTED_LOCATION_NOCONFIG)
    ENDIF()
    SET_PROPERTY(
      TARGET yaml_cpp
      PROPERTY IMPORTED_LOCATION ${_system_yaml_cpp_loc}
    )
    GET_TARGET_PROPERTY(_system_yaml_cpp_inc yaml-cpp::yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
    TARGET_INCLUDE_DIRECTORIES(
      yaml_cpp
      INTERFACE ${_system_yaml_cpp_inc}
    )
  ELSE()
    MESSAGE(WARNING "yaml_cpp: OCIO did not vendor yaml-cpp and no system yaml-cpp was found.")
    SET_PROPERTY(
      TARGET yaml_cpp
      PROPERTY IMPORTED_LOCATION ${_ocio_yaml_cpp_libpath}
    )
    FILE(MAKE_DIRECTORY ${RV_DEPS_OCIO_DIST_DIR}/include)
    TARGET_INCLUDE_DIRECTORIES(
      yaml_cpp
      INTERFACE ${RV_DEPS_OCIO_DIST_DIR}/include
    )
  ENDIF()
ELSE()
  SET_PROPERTY(
    TARGET yaml_cpp
    PROPERTY IMPORTED_LOCATION ${_ocio_yaml_cpp_libpath}
  )
  # It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
  SET(_yaml_cpp_include_dir
      ${RV_DEPS_OCIO_DIST_DIR}/include
  )
  FILE(MAKE_DIRECTORY ${_yaml_cpp_include_dir})
  TARGET_INCLUDE_DIRECTORIES(
    yaml_cpp
    INTERFACE ${_yaml_cpp_include_dir}
  )
ENDIF()

SET(RV_DEPS_YAML_CPP_VERSION
    "0.7.0"
    CACHE INTERNAL "" FORCE
)
