#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkMovie"
)

SET(_sources
    Movie.cpp
    MovieIO.cpp
    MovieReader.cpp
    MovieWriter.cpp
    ReformattingMovie.cpp
    NullWriter.cpp
    MovieNullIO.cpp
    SequenceMovie.cpp
    LeaderFooterMovie.cpp
    ThreadedMovie.cpp
    Exception.cpp
    ResamplingMovie.cpp
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
  PUBLIC TwkAudio TwkExc TwkFB stl_ext
  PRIVATE TwkFBAux TwkUtil ffmpeg::swresample ${CMAKE_DL_LIBS}
)

IF(RV_TARGET_LINUX)
  SET(THREADS_PREFER_PTHREAD_FLAG
      TRUE
  )
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC Threads::Threads
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DTWKMOVIE_BUILD"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC win_pthreads win_posix
  )
ENDIF()

RV_STAGE(TYPE "SHARED_LIBRARY" TARGET ${_target})
