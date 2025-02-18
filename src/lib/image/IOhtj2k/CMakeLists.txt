#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "IOhtj2k"
)

SET(_sources
    IOhtj2k.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

GET_TARGET_PROPERTY(OPENJPH_INCLUDES OpenJph::OpenJph INTERFACE_INCLUDE_DIRECTORIES)
MESSAGE(STATUS "OpenJph includes: ${OPENJPH_INCLUDES}")

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkFB TwkExc OpenJph::OpenJph
  PRIVATE  TwkUtil
  )

# ADD_LINK_OPTIONS("-Wl,-E")

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
