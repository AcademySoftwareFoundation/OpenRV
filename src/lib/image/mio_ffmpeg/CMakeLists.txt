#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "mio_ffmpeg"
)

LIST(APPEND _sources init.cpp)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE MovieFFMpeg Boost::program_options TwkFB
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-D__STDC_CONSTANT_MACROS"
  )
ENDIF()

RV_STAGE(TYPE "MOVIE_FORMAT" TARGET ${_target})
