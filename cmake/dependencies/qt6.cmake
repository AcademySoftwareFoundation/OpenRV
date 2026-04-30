#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Qt5: OpenGLWidgets is in component Widgets Qt6: OpenGLWidgets is in component OpenGLWidgets
SET(QT6_QOPENGLWIDGET_COMPONENT
    "OpenGLWidgets"
    CACHE STRING "Qt QOpenGLWidget component name"
)
SET(QT6_QOPENGLWIDGET_TARGET
    "Qt6::OpenGLWidgets"
    CACHE STRING "Qt QOpenGLWidget target name"
)

FIND_PROGRAM(
  QT_MOC_EXECUTABLE
  NAMES moc
  PATHS ${RV_DEPS_QT_LOCATION}/bin ${RV_DEPS_QT_LOCATION}/libexec ${RV_DEPS_QT_LOCATION}/share/qt/libexec
  NO_DEFAULT_PATH
)
IF(NOT QT_MOC_EXECUTABLE)
  FIND_PROGRAM(
    QT_MOC_EXECUTABLE
    NAMES moc
  )
ENDIF()

FIND_PROGRAM(
  QT_UIC_EXECUTABLE
  NAMES uic
  PATHS ${RV_DEPS_QT_LOCATION}/bin ${RV_DEPS_QT_LOCATION}/libexec ${RV_DEPS_QT_LOCATION}/share/qt/libexec
  NO_DEFAULT_PATH
)
IF(NOT QT_UIC_EXECUTABLE)
  FIND_PROGRAM(
    QT_UIC_EXECUTABLE
    NAMES uic
  )
ENDIF()

SET(QT_MOC_EXECUTABLE
    "${QT_MOC_EXECUTABLE}"
    CACHE STRING "Qt MOC executable" FORCE
)
SET(QT_UIC_EXECUTABLE
    "${QT_UIC_EXECUTABLE}"
    CACHE STRING "Qt UIC executable" FORCE
)

FIND_PACKAGE(
  Qt6 REQUIRED
  COMPONENTS Core
             Widgets
             WebEngineCore
             WebEngineWidgets
             OpenGLWidgets
             Svg
             Network
             Sql
             Xml
             OpenGL
             Test
)
MESSAGE(STATUS "Using system Qt ${Qt6_VERSION}")
