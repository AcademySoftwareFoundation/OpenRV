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
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

FIND_LIBRARY(
  MESAGL_LIBRARY
  NAMES libGLX_mesa.so.0 
  PATHS /usr/lib64 /usr/lib
  DOC "MesaGL library"
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE Mu
          MuLang
          MuTwkApp
          IOproxy
          MovieProxy
          OpenEXR::OpenEXR
          PyTwkApp
          RvApp
          IPCore
          IPBaseNodes
          MovieRV
          MovieFB
          MovieMuDraw
          MovieProcedural
          TwkAudio
          TwkMovie
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
          Qt5::Gui 
          Qt5::Widgets 
          BDWGC::Gc 
          QTBundle 

          TwkGLFMesa
          ${MESAGL_LIBRARY}
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"" "-DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\""
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
