#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "OCIONodes"
)

FIND_PACKAGE(
  Qt5 REQUIRED
  COMPONENTS Core
)

FILE(GLOB _sources *.cpp)

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
  PUBLIC IPCore TwkFB ocio::ocio
  PRIVATE TwkUtil Qt5::Core
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC glew win_posix
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-wd4355"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
