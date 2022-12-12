#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkAudio"
)

SET(_sources
    Audio.cpp
    AudioCache.cpp
    AudioFormats.cpp
    Interlace.cpp
    Resampler.cpp
    Mix.cpp
    Filters.cpp
    ScaleTime.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkMath stl_ext
  PRIVATE TwkUtil resample
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DTWKAUDIO_BUILD"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC win_posix win_pthreads
  )
ENDIF()

RV_STAGE(TYPE "SHARED_LIBRARY" TARGET ${_target})
