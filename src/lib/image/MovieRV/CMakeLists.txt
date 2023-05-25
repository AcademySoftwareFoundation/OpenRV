#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MovieRV"
)

SET(_sources
    MovieRV.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkApp TwkMovie
  PRIVATE IPBaseNodes
          IPCore
          RvApp
          TwkContainer
          TwkFB
          TwkGLF
          TwkGLFMesa
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-D__WINGL_H__"
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
