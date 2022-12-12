#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "mu_gl"
)

SET(_sources
    init.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

SET_TARGET_PROPERTIES(
  ${_target}
  PROPERTIES OUTPUT_NAME "gl"
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE Mu MuLang MuGL
)

RV_STAGE(TYPE "MU_PLUGIN" TARGET ${_target})
