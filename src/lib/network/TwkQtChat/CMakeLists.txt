#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkQtChat"
)

SET(CMAKE_AUTOMOC
    ON
)

SET(_sources
    Client.cpp Connection.cpp Server.cpp TwkQtChat/Client.h TwkQtChat/Connection.h TwkQtChat/Server.h
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core Network
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Qt::Core Qt::Network
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
