#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# RV_ADD_IMPORTED_LIBRARY - Create an imported library target with standard properties
#
# Consolidates the repeated ADD_LIBRARY(IMPORTED) + SET_PROPERTY + TARGET_INCLUDE_DIRECTORIES
# pattern used across dependency files.
#
# cmake-format: off
# Usage:
#   RV_ADD_IMPORTED_LIBRARY(
#     NAME <target-name>           # REQUIRED: e.g., ZLIB::ZLIB
#     TYPE <SHARED|STATIC>         # REQUIRED: library type
#     LOCATION <path>              # REQUIRED: IMPORTED_LOCATION
#     [SONAME <name>]              # IMPORTED_SONAME (optional)
#     [IMPLIB <path>]              # IMPORTED_IMPLIB (optional, typically Windows only)
#     [INCLUDE_DIRS <dir1>...]     # INTERFACE_INCLUDE_DIRECTORIES (also creates dirs)
#     [DEPENDS <target>]           # ADD_DEPENDENCIES (optional)
#     [ADD_TO_DEPS_LIST]           # Append to RV_DEPS_LIST
#   )
# cmake-format: on
FUNCTION(RV_ADD_IMPORTED_LIBRARY)
  CMAKE_PARSE_ARGUMENTS(_ARG "ADD_TO_DEPS_LIST" "NAME;TYPE;LOCATION;SONAME;IMPLIB;DEPENDS" "INCLUDE_DIRS" ${ARGN})

  IF(NOT _ARG_NAME)
    MESSAGE(FATAL_ERROR "RV_ADD_IMPORTED_LIBRARY: NAME is required")
  ENDIF()
  IF(NOT _ARG_TYPE)
    MESSAGE(FATAL_ERROR "RV_ADD_IMPORTED_LIBRARY: TYPE is required")
  ENDIF()
  IF(NOT _ARG_LOCATION)
    MESSAGE(FATAL_ERROR "RV_ADD_IMPORTED_LIBRARY: LOCATION is required")
  ENDIF()

  ADD_LIBRARY(${_ARG_NAME} ${_ARG_TYPE} IMPORTED GLOBAL)

  IF(_ARG_DEPENDS)
    ADD_DEPENDENCIES(${_ARG_NAME} ${_ARG_DEPENDS})
  ENDIF()

  SET_PROPERTY(
    TARGET ${_ARG_NAME}
    PROPERTY IMPORTED_LOCATION ${_ARG_LOCATION}
  )

  IF(_ARG_SONAME)
    SET_PROPERTY(
      TARGET ${_ARG_NAME}
      PROPERTY IMPORTED_SONAME ${_ARG_SONAME}
    )
  ENDIF()

  IF(_ARG_IMPLIB)
    SET_PROPERTY(
      TARGET ${_ARG_NAME}
      PROPERTY IMPORTED_IMPLIB ${_ARG_IMPLIB}
    )
  ENDIF()

  IF(_ARG_INCLUDE_DIRS)
    FILE(MAKE_DIRECTORY ${_ARG_INCLUDE_DIRS})
    TARGET_INCLUDE_DIRECTORIES(
      ${_ARG_NAME}
      INTERFACE ${_ARG_INCLUDE_DIRS}
    )
  ENDIF()

  IF(_ARG_ADD_TO_DEPS_LIST)
    SET(RV_DEPS_LIST
        ${RV_DEPS_LIST} ${_ARG_NAME}
        PARENT_SCOPE
    )
  ENDIF()
ENDFUNCTION()
