#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "IPMu"
)

SET(_sources
    CommandsModule.cpp
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
  PRIVATE IPBaseNodes
          IPCore
          Mu
          MuLang
          MuTwkApp
          TwkApp
          TwkContainer
          TwkDeploy
          TwkFB
          TwkMovie
          TwkUtil
          stl_ext
          MuSystem
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DRELEASE_VARIANT=\"${RV_RELEASE_DESCRIPTION}\" -DAPPLICATION_TYPE=\"${RV_APPLICATION_TYPE}\"
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

IF(RV_TARGET_DARWIN)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DREG_EXTENDED=1"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
