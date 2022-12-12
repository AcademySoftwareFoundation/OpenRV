#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "rvnuke"
)

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
