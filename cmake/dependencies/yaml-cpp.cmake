#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(yaml-cpp REQUIRED)
IF(TARGET yaml-cpp)
  SET_PROPERTY(
    TARGET yaml-cpp
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
IF(TARGET yaml-cpp::yaml-cpp)
  SET_PROPERTY(
    TARGET yaml-cpp::yaml-cpp
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
# Map old name 'yaml_cpp' to new name 'yaml-cpp::yaml-cpp' for compatibility
IF(NOT TARGET yaml_cpp)
  ADD_LIBRARY(yaml_cpp INTERFACE IMPORTED GLOBAL)
  TARGET_LINK_LIBRARIES(
    yaml_cpp
    INTERFACE yaml-cpp::yaml-cpp
  )
ENDIF()
