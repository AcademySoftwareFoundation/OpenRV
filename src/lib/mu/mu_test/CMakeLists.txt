#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "mu_test"
)

LIST(
  APPEND
  _sources
  AClassType.cpp
  BClassType.cpp
  BarInterface.cpp
  BaseType.cpp
  CClassType.cpp
  SequenceInterface.cpp
  TestModule.cpp
  init.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

# Special case, this plugin module is used and called 'test.so'
SET_TARGET_PROPERTIES(
  ${_target}
  PROPERTIES OUTPUT_NAME "test"
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Mu
  PRIVATE MuLang
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE win_pthreads
  )
ENDIF()

RV_STAGE(TYPE "MU_PLUGIN" TARGET ${_target})
