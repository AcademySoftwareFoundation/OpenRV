#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "PyTwkApp"
)

SET(_sources
    PyInterface.cpp
    PyEventType.cpp
    PyFunctionAction.cpp
    PyCommands.cpp
    PyMenuState.cpp
    PyMu.cpp
    PyMuSymbolType.cpp
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
  REQUIRED
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
  PUBLIC Python::Python
  PRIVATE Mu MuLang MuPy MuTwkApp Qt5::Core TwkPython
)

IF(RV_TARGET_DARWIN)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-Wno-deprecated-writable-strings"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
