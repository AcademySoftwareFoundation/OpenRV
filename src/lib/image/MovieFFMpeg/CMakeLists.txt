#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MovieFFMpeg"
)

LIST(APPEND _sources MovieFFMpeg.cpp)
IF(RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH)
  LIST(APPEND _sources AppleProRes.cpp)
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
  PUBLIC TwkMovie ffmpeg::avutil
  PRIVATE mp4v2Utils
          Boost::filesystem
          TwkUtil
          TwkFB
          IOhtj2k
          ffmpeg::avcodec
          ffmpeg::avformat
          ffmpeg::swscale
          OpenJph::OpenJph
)

IF(RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH)
  ADD_DEPENDENCIES(${_target} Apple::ProRes)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Apple::ProRes
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRV_USE_APPLE_PRORES_SDK"
  )
ENDIF()

IF(RV_FFMPEG_USE_VIDEOTOOLBOX)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE DarwinBundle "-framework VideoToolbox"
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-D__STDC_CONSTANT_MACROS"
  )
ENDIF()

IF(RV_TARGET_LINUX)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-D__STDC_CONSTANT_MACROS"
  )
ENDIF()

IF(RV_FFMPEG_USE_VIDEOTOOLBOX)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRV_FFMPEG_USE_VIDEOTOOLBOX"
  )
ENDIF()

TARGET_COMPILE_DEFINITIONS(
  ${_target}
  PRIVATE "SKIP_CODEC_CHECKS"
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
