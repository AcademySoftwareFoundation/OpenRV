#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "mu_rvui"
)

IF(RV_VFX_PLATFORM STREQUAL CY2024)
    SET(MU_QT_QPROCESS_STARTDETACHED "qt.QProcess.startDetached (cmd, string[] {\"\"});")
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
    SET(MU_QT_QPROCESS_STARTDETACHED "qt.QProcess.startDetached (cmd);")
ENDIF()
configure_file(rvui.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/rvui.mu @ONLY)


IF(RV_VFX_PLATFORM STREQUAL CY2024)
    set(MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY
        "let screen = qt.QApplication.screenAt(mainWin.mapToGlobal(qt.QPoint(0, 0)));\n \
        let usedRect = screen.availableGeometry();\n"
    )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
    set(MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY
        "let usedRect = qt.QApplication.desktop().availableGeometry(mainWin);\n"
    )
ENDIF()
set(MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY "${MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY}")
configure_file(window_mode.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/window_mode.mu @ONLY)

SET(_mumodules
    rvui.mu
    glyph.mu
    app_utils.mu
    export_utils.mu
    rvtypes.mu
    HUD.mu
    extra_commands.mu
    gltexture.mu
    wipes.mu
    inspector.mu
    window_mode.mu
    mode_manager.mu
    xmlrpc.mu
    timeline.mu
    property_hash_table.mu
    motion_scope.mu
    lutgen.mu
    external_qprocess.mu
    external_ioprocess.mu
    presentation_mode.mu
)

SET(_mudmodules
    commands.mud
    system.mud
    gl.mud
    glu.mud
    gltext.mud
    io.mud
    math.mud
    math_util.mud
    math_linear.mud
    encoding.mud
)

ADD_CUSTOM_TARGET(${_target})

RV_STAGE(
  TARGET
  ${_target}
  TYPE
  "MU_SOURCE_MODULE"
  FILES
  ${_mumodules}
  ${_mudmodules}
  PATH
  ${RV_STAGE_PLUGINS_MU_DIR}
)
