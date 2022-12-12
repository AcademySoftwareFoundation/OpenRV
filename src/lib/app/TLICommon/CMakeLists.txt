#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TLICommon"
)

FILE(GLOB _sources *.c*)
# Add QT-specific source files
LIST(APPEND _sources TwkLicenseWindow.cpp TwkServerDialog.cpp)
ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)
SET_PROPERTY(
  TARGET ${_target}
  PROPERTY AUTOMOC ON
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core Gui OpenGL Widgets Core
  REQUIRED
)
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC .. ${CMAKE_SOURCE_DIR}/src/lib/geometry/TwkContainer ${CMAKE_SOURCE_DIR}/src/lib/files/Gto ${CMAKE_SOURCE_DIR}/src/lib/base/TwkLM
         ${CMAKE_SOURCE_DIR}/src/lib/app/TwkApp ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  Gto
  TwkApp
  TwkContainer
  TwkExc
  TwkLM
  TwkUtil
  stl_ext
  ZLIB::ZLIB
  Qt5::Core
  Qt5::Gui
  Qt5::OpenGL
  Qt5::Widgets
  Qt5::Core
)
