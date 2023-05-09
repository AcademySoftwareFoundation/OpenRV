#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Debugging options
OPTION(RV_VERBOSE_INVOCATION "Show the compiler/link command invocation." OFF)
OPTION(RV_SHOW_ALL_VARIABLES "Displays all build variables." ON)

#
# General build options

OPTION(RV_USE_OCIO_OPENEXR "Uses OpenColorIO's own copy of OpenEXR." OFF)   # Mac & Windows do not work with OCIO's copy. Moreover, next release OCIO will remove OpenExr on their side.
OPTION(RV_USE_OCIO_YAML_CPP "Uses OpenColorIO's own copy of Yaml-Cpp." ON)  # IF OFF: the user must provide another yaml lib

SET(RV_DEPS_BASE_DIR
    "${CMAKE_BINARY_DIR}"
    CACHE STRING "RV's 3rd party cache location."
)
SET(RV_DEPS_DOWNLOAD_DIR
    "${RV_DEPS_BASE_DIR}/RV_DEPS_DOWNLOAD"
    CACHE STRING "RV's 3rd party download cache location."
)

IF(NOT EXISTS (${RV_DEPS_BASE_DIR}))
  FILE(MAKE_DIRECTORY ${RV_DEPS_BASE_DIR})
ENDIF()

IF(NOT EXISTS (${RV_DEPS_DOWNLOAD_DIR}))
  FILE(MAKE_DIRECTORY ${RV_DEPS_DOWNLOAD_DIR})
ENDIF()

SET(RV_CPP_STANDARD
    "17"
    CACHE STRING "RV's general C++ coding standard"
)
SET_PROPERTY(
  CACHE RV_CPP_STANDARD
  PROPERTY STRINGS 14 17
)

SET(RV_C_STANDARD
    "17"
    CACHE STRING "RV's general C coding standard"
)
SET_PROPERTY(
  CACHE RV_C_STANDARD
  PROPERTY STRINGS C99 11 17
)
