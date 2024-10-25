#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "QtBonjour"
)

SET(CMAKE_AUTOUIC
    ON
)
SET(CMAKE_AUTOMOC
    ON
)
SET(CMAKE_AUTORCC
    ON
)

LIST(
  APPEND
  _sources
  Browser.cpp
  Register.cpp
  Resolver.cpp
  QtBonjour/Browser.h
  QtBonjour/Register.h
  QtBonjour/Resolver.h
  QtBonjour/Record.h
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Qt::Core
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
