#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Switch group edit mode — Python port of SwitchGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import rv.rvtypes
import rv.runtime


def _activateModeEntry(name, on):
    """Activate/deactivate a sibling mode through RV's Mu mode manager.

    commands.activateMode() cannot be used: these modes are declared
    `load: delay` in PACKAGE, and it returns successfully while leaving an
    unloaded mode inactive. The mode manager lazy-loads the entry first, and it
    has no Python binding, so it is reached through the Mu bridge.
    """
    rv.runtime.eval(
        '{ State s = data(); '
        'mode_manager.ModeManagerMode mm = s.modeManager; '
        'mm.activateEntry(mm.findModeEntry("%s"), %s); '
        '"ok"; }' % (name, "true" if on else "false"),
        ["rvtypes", "commands", "mode_manager"],
    )


class SwitchGroupEditMode(rv.rvtypes.MinorMode):
    def activateUI(self, on):
        for mode in ["Switch_edit_mode"]:
            _activateModeEntry(mode, on)

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)
        self.activateUI(True)

    def deactivate(self):
        rv.rvtypes.MinorMode.deactivate(self)
        self.activateUI(False)

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self.init("SwitchGroup_edit_mode", None, None, None, None)


def createMode():
    return SwitchGroupEditMode()
