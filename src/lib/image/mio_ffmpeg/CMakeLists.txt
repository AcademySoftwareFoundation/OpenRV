#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Note: This cache variable can be overriden to FALSE in a super-project if a custom mio_ffmpeg is built as part of that super-project.
SET(RV_BUILD_MIO_FFMPEG
    TRUE
    CACHE BOOL "Whether or not to build the Open RV mio_ffmpeg movie plugin"
)

IF(RV_BUILD_MIO_FFMPEG)

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

  FOREACH(
    RV_FFMPEG_NON_FREE_DECODER_TO_ENABLE
    ${RV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE}
  )
    TARGET_COMPILE_OPTIONS(
      ${_target}
      PRIVATE "-D__FFMPEG_ENABLE_NON_FREE_DECODER_${RV_FFMPEG_NON_FREE_DECODER_TO_ENABLE}"
    )
  ENDFOREACH()

  RV_STAGE(TYPE "MOVIE_FORMAT" TARGET ${_target})

ENDIF() # IF (RV_BUILD_MIO_FFMPEG)
