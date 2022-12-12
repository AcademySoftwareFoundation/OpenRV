#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkGLFCoreGraphics"
)

SET(_sources
    CGGLVideoDevice.cpp
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
  PUBLIC OpenGL::GL TwkFB TwkGLF
)

IF(RV_TARGET_LINUX)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DVM_NO_GL"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
