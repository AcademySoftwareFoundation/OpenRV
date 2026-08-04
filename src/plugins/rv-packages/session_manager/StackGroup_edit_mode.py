#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Stack group edit mode — Python port of StackGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import os
import sys

import rv.commands as commands
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


def _syncWipeMode(on):
    """Reconcile the wipes minor mode with this view's ui.wipes flag.

    The wipe mode instance lives on the session State, which Python cannot reach,
    and toggleWipe has no Python binding either, so the whole reconciliation runs
    in Mu and the flag and the mode state are read together.

    Note on toggleWipe vs wipe.toggle: toggleWipe resets the wipes and turns them
    off (sets the ui.wipes flag to 0), while wipe.toggle only makes the mode
    inactive, so the wipes are in the same state when this view is returned to.
    """
    rv.runtime.eval(
        '{ State s = data(); '
        'let wipe = s.wipe; '
        'let p = viewNode() + ".ui.wipes"; '
        'let wipeon = %s && propertyExists(p) && getIntProperty(p).front() == 1; '
        'if (wipeon) { if (wipe eq nil || !wipe._active) toggleWipe(); } '
        'else { if (wipe neq nil && wipe._active) wipe.toggle(); } '
        '"ok"; }' % ("true" if on else "false"),
        ["rvtypes", "commands", "rvui"],
    )


class StackGroupEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def activateUI(self, on):
        for mode in ["Composite_edit_mode", "Stack_edit_mode"]:
            _activateModeEntry(mode, on)

        _syncWipeMode(on)

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)
        self.activateUI(True)

    def deactivate(self):
        rv.rvtypes.MinorMode.deactivate(self)
        self.activateUI(False)

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        comp = parts[1]
        name = parts[2]

        #
        #  If a UI name changes we need to update the tree
        #

        if (comp == "ui" and name == "wipes") or (
            comp == "timing" and name == "retimeToOutput"
        ):
            self.activateUI(True)
            commands.redraw()

        event.reject()

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self.init(
            "StackGroup_edit_mode",
            None,
            [("graph-state-change", self.propertyChanged, "Maybe update session UI")],
            [("Stack", [])],
            None,
        )


def createMode():
    return StackGroupEditMode()
