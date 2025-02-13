#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "rvnuke"
)

IF(RV_VFX_PLATFORM STREQUAL CY2024)
  SET(RVNUKE_MODE_CURRENT_DATE_STRING
      "return d.toLocalTime().toString(\"MMM-dd hh:mm:ss\",  qt.QCalendar());\n"
  )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(RVNUKE_MODE_CURRENT_DATE_STRING
      "return d.toLocalTime().toString(\"MMM-dd hh:mm:ss\");\n"
  )
ENDIF()

CONFIGURE_FILE(rvnuke_mode.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/rvnuke_mode.mu @ONLY)

SET(_sources
    menu.py
    rvNetwork.py
    rvNuke.png
    rvNuke.py
    rvNukeCheck.png
    rvNukeGear.png
    rvnuke_help.html
    rvnuke_mode.mu
    rvnuke_process.mu
)

RV_STAGE(TYPE "RVPKG" TARGET ${_target} FILES ${_sources})
