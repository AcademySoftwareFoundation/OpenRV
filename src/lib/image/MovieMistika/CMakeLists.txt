#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MovieMistika"
)

SET(_sources
    MovieMistika.cpp
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
  PUBLIC TwkMovie
  PRIVATE IOcin TwkFB TwkMath TwkMath TwkUtil stl_ext
)

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(${_target})
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(${_target})
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
