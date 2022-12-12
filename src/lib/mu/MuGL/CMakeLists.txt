#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MuGL"
)

SET(_sources
    GLModule.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

FIND_PACKAGE(OpenGL REQUIRED)
TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Mu
  PRIVATE OpenGL::GL OpenGL::GLU MuLang
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE glew
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
