#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkGLFMesa"
)

LIST(APPEND _sources OSMesaVideoDevice.cpp)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

FIND_PACKAGE(OSMesa REQUIRED)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkFB TwkGLF
  PRIVATE OSMesa::OSMesa TwkGLText
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
