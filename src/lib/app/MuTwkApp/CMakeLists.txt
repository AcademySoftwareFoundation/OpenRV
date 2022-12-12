#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MuTwkApp"
)

SET(_sources
    CommandsModule.cpp
    MuInterface.cpp
    EventType.cpp
    FunctionAction.cpp
    MenuItem.cpp
    MenuState.cpp
    SettingsValueType.cpp
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
  PUBLIC Mu MuLang TwkApp
  PRIVATE TwkUtil stl_ext
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
