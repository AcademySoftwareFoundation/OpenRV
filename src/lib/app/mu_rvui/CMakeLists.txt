#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "mu_rvui"
)

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
