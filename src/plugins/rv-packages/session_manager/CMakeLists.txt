#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "session_manager"
)

IF(RV_VFX_PLATFORM STREQUAL CY2024)
    SET(MU_QT_QDRAGMOVEEVENT_POSITION "event.position().toPoint()")
    SET(MU_QT_QPALETTE_COLORROLE "QPalette.Window")
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
    SET(MU_QT_QDRAGMOVEEVENT_POSITION "event.pos()")
    SET(MU_QT_QPALETTE_COLORROLE "QPalette.Background")
ENDIF()

configure_file(session_manager.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/session_manager.mu @ONLY)

RV_STAGE(TYPE "RVPKG" TARGET ${_target})
