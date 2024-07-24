#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkFB"
)

SET(_sources
    FrameBuffer.cpp
    IO.cpp
    Operations.cpp
    DepthMap.cpp
    Cache.cpp
    Histogram.cpp
    Attribute.cpp
    StreamingIO.cpp
    Exception.cpp
    TwkFBThreadPool.cpp
    FastMemcpy.cpp
    FastConversion.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(RV_TARGET_WINDOWS)
  TARGET_INCLUDE_DIRECTORIES(
    ${_target}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
ENDIF()

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkExc TwkMath TwkUtil OpenEXR::OpenEXR
  PRIVATE RVMath stl_ext lcms ${CMAKE_DL_LIBS}
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC win_pthreads
  )
ENDIF()

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-DHAVE_HALF"
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DTWKFB_BUILD"
  )
ENDIF()

RV_STAGE(TYPE "SHARED_LIBRARY" TARGET ${_target})
