#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "ICCNodes"
)

SET(_sources
    ICCDefinitions.cpp ICCIPNode.cpp
)
IF(RV_TARGET_DARWIN)

ENDIF()

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
  PUBLIC Boost::thread IPCore
  PRIVATE TwkUtil TwkFB TwkMath lcms
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC glew nedmalloc win_posix
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-wd4355"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
