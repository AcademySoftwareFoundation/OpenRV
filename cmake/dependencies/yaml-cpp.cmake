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
ADD_LIBRARY(yaml_cpp STATIC IMPORTED GLOBAL)
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

SET(RV_DEPS_YAML_CPP_VERSION
    "0.7.0"
    CACHE INTERNAL "" FORCE
)
