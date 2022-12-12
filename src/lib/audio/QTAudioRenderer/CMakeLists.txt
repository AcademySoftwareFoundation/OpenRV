#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(CMAKE_AUTOUIC
    ON
)
SET(CMAKE_AUTOMOC
    ON
)
SET(CMAKE_AUTORCC
    ON
)

SET(_target
    "QTAudioRenderer"
)

FILE(GLOB _sources QTAudioRenderer.cpp QTAudioRenderer/QTAudioRenderer.h)
ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core Multimedia
  REQUIRED
)

# Note that the audio plugins are opened by the app with dlopen.  We need the includes to compile from some libs to compile, but we don't want to link with them
# to avoid duplicating symbols when the dlopen occurs
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "$<TARGET_PROPERTY:RvApp,INTERFACE_INCLUDE_DIRECTORIES>"
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkMath TwkUtil Qt5::Core Qt5::Multimedia
  PRIVATE TwkAudio
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
