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
  PUBLIC .. ${PROJECT_SOURCE_DIR}/src/lib/geometry/TwkContainer ${PROJECT_SOURCE_DIR}/src/lib/files/Gto ${PROJECT_SOURCE_DIR}/src/lib/base/TwkLM
         ${PROJECT_SOURCE_DIR}/src/lib/app/TwkApp ${CMAKE_CURRENT_PROJECT_DIR}
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
