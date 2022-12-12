#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "ALSASafeAudioModule"
)

SET(_sources
    ALSASafeAudioRenderer.cpp init.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

# Note that the audio plugins are opened by the app with dlopen.  We need the includes to compile from some libs to compile, but we don't want to link with them
# to avoid duplicating symbols when the dlopen occurs
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "$<TARGET_PROPERTY:IPCore,INTERFACE_INCLUDE_DIRECTORIES>" "$<TARGET_PROPERTY:IPBaseNodes,INTERFACE_INCLUDE_DIRECTORIES>"
         "$<TARGET_PROPERTY:RvApp,INTERFACE_INCLUDE_DIRECTORIES>"
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE TwkAudio asound
)

ADD_LINK_OPTIONS("-Wl,-E")

RV_STAGE(TYPE "SHARED_LIBRARY" TARGET ${_target})
