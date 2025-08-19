#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "mu_rvui"
)

IF(RV_VFX_PLATFORM STREQUAL CY2024)
  SET(MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY
    "
       // Get available screen geometry for the screen containing the window\n \
       let screen = qt.QGuiApplication.screenAt(globalPos);\n \
       if (screen eq nil) screen = qt.QGuiApplication.primaryScreen();\n \
       let usedRect = screen.availableGeometry();\n \
    "
  )
  SET(MU_DPI_SCALING_ADJUSTMENT
    "
       // Check for DPI/scaling on the current screen\n \
       let globalPos = mainWin.mapToGlobal(qt.QPoint(0, 0));\n \
       let dpiScreen = qt.QGuiApplication.screenAt(globalPos);\n \
       if (dpiScreen eq nil) dpiScreen = qt.QGuiApplication.primaryScreen();\n \
       let hdpi = system.getenv(\"RV_NO_QT_HDPI_SUPPORT\", nil);\n \
       if (hdpi eq nil || hdpi == 0)\n \
       {\n \
          if (dpiScreen neq nil)\n \
          {\n \
              let dpr = dpiScreen.devicePixelRatio();\n \
              if (dpr > 1.0)\n \
              {\n \
                  width = int(width / dpr + 0.5);\n \
                  height = int(height / dpr + 0.5);\n \
              }\n \
          }\n \
       }\n \
    "
  )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY
      "let usedRect = qt.QApplication.desktop().availableGeometry(mainWin);\n"
  )
  SET(MU_DPI_SCALING_ADJUSTMENT "")
ENDIF()
SET(MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY
    "${MU_QT_QAPPLICATION_AVAILABLE_GEOMETRY}"
)
SET(MU_DPI_SCALING_ADJUSTMENT
    "${MU_DPI_SCALING_ADJUSTMENT}"
)
CONFIGURE_FILE(window_mode.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/window_mode.mu @ONLY)

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
