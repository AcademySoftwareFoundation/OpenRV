#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FILE(
  GLOB packages
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  *
)
FOREACH(
  package
  ${packages}
)
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${package}/CMakeLists.txt)
    ADD_SUBDIRECTORY(${package})
  ENDIF()
ENDFOREACH()
