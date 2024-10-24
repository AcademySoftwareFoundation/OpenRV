#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rvshell"
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

SET(_sources
    main.cpp utf8Main.cpp ShellDialog.cpp
)

FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core Gui Network Widgets
  REQUIRED
)

QT_WRAP_UI(_qt_ui_headers ShellDialog.ui)

ADD_EXECUTABLE(
  ${_target}
  ${_sources} ${_qt_ui_headers}
)
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE TwkQtChat Qt::Core Qt::Gui Qt::Network Qt::Widgets
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
