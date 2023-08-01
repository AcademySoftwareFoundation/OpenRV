#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkApp"
)

SET(_sources
    SelectionType.cpp
    Selection.cpp
    SelectionState.cpp
    Mode.cpp
    Menu.cpp
    Document.cpp
    EventNode.cpp
    Action.cpp
    EventTable.cpp
    Event.cpp
    Command.cpp
    Application.cpp
    Bundle.cpp
    Exception.cpp
    OutputPlugin.cpp
    OutputPlugins.cpp
    VideoDevice.cpp
    VideoModule.cpp
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
  PUBLIC TwkAudio TwkMath TwkUtil
  PRIVATE stl_ext Boost::filesystem
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
