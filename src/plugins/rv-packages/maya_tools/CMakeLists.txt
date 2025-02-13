#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "maya_tools"
)

IF(RV_VFX_PLATFORM STREQUAL CY2024)
  SET(MAYA_TOOLS_UPDATE_MAYA_SOURCE
      "let t = qt.QDateTime.currentDateTime().toString(\"h:mm\",  qt.QCalendar());\n"
  )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(MAYA_TOOLS_UPDATE_MAYA_SOURCE
      "let t = qt.QDateTime.currentDateTime().toString(\"h:mm\");\n"
  )
ENDIF()

CONFIGURE_FILE(maya_tools.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/maya_tools.mu @ONLY)

RV_STAGE(TYPE "RVPKG" TARGET ${_target})
