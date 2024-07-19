#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
FILE(
  GLOB directories
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  *
)
FOREACH(
  directory
  ${directories}
)
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${directory}/CMakeLists.txt)
    ADD_SUBDIRECTORY(${directory})
  ENDIF()
ENDFOREACH()
