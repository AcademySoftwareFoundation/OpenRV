#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MovieMuDraw"
)

LIST(APPEND _sources MovieMuDraw.cpp)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

SET(GLEW_USE_STATIC_LIBS
    TRUE
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Mu MuLang TwkGLF TwkMovie
  PRIVATE TwkAudio TwkExc TwkFB TwkGLText TwkGLF
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE nedmalloc glew
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
