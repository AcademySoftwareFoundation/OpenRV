#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rvio_sw"
)

LIST(APPEND _sources ../rvio/UICommands.cpp ../rvio/main.cpp ../rvio/utf8Main.cpp glfix.c)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core Gui Widgets
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ../rvio
)

FIND_LIBRARY(
  MESAGL_LIBRARY
  NAMES libGLX_mesa.so.0
  PATHS /usr/lib64 /usr/lib
  DOC "MesaGL library"
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Mu
         MuLang
         MuTwkApp
         TwkMovie
         TwkAudio
         TwkGLFMesa
         MovieRV
  PRIVATE RvCommon
          IOproxy
          MovieProxy
          OpenEXR::OpenEXR
          PyTwkApp
          RvApp
          IPCore
          IPBaseNodes
          MovieFB
          MovieMuDraw
          MovieProcedural
          TwkMovie
          TwkAudio
          TwkCMS
          TwkMath
          TwkDeploy
          TwkExc
          TwkFB
          TwkUtil
          TwkQtBase
          arg
          stl_ext
          TwkMediaLibrary
          Qt5::Core
          BDWGC::Gc
          Qt5::Gui
          Qt5::Widgets
          QTBundle
          ${MESAGL_LIBRARY}
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"" "-DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\""
)

GET_PROPERTY(
  RVIO_THIRD_PARTY_CUSTOMIZATION GLOBAL
  PROPERTY "RVIO_THIRD_PARTY_CUSTOMIZATION"
)
IF(RVIO_THIRD_PARTY_CUSTOMIZATION)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRVIO_THIRD_PARTY_CUSTOMIZATION=1"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE ${RVIO_THIRD_PARTY_CUSTOMIZATION}
  )
ENDIF()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
