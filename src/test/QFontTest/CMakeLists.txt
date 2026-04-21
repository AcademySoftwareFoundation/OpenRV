#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(CMAKE_AUTORCC
    ON
)
SET(_font_qrc
    ${PROJECT_SOURCE_DIR}/src/lib/app/RvCommon/qrc/RvCommon.qrc
)
SET(_font_path
    ${PROJECT_SOURCE_DIR}/src/lib/app/RvCommon/qrc/fonts/fontawesome-webfont.ttf
)
FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Widgets Core Gui
  REQUIRED
)
SET(_target_qt
    "QFontTest"
)
LIST(APPEND _sources_qt qtfont.cpp ${_font_qrc})

ADD_EXECUTABLE(
  ${_target_qt}
  ${_sources_qt}
)
TARGET_LINK_LIBRARIES(
  ${_target_qt}
  PRIVATE QTBundle Qt::Widgets Qt::Gui Qt::Core doctest::doctest
)
ADD_TEST(
  NAME ${_target_qt}
  COMMAND ${CMAKE_COMMAND} -E env RV_FONT_PATH=${_font_path};LD_LIBRARY_PATH=${RV_STAGE_LIB_DIR};QT_QPA_PLATFORM=offscreen "$<TARGET_FILE:${_target_qt}>"
)
RV_STAGE(TYPE "EXECUTABLE" TARGET ${_target_qt})
