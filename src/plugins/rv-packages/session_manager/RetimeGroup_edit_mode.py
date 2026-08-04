#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Retime group edit mode — Python port of RetimeGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import functools
import os
import sys

import rv.commands as commands
import rv.qtutils as qtutils
import rv.rvtypes
import rv.runtime

from PySide6 import QtCore, QtWidgets
from PySide6.QtUiTools import QUiLoader

from session_manager import menuItem, setFloatProp, setIntProp

#
#  Internal events used to reach the Mu side of the raw-parameter menu items, and to
#  carry the entered text back from RV's text entry mode.
#
_PARAMETER_MODES = (
    ("retime-group-edit-visual-scale", "#RVRetime.visual.scale", 0.05, 1.0),
    ("retime-group-edit-visual-offset", "#RVRetime.visual.offset", 0.05, 0.0),
    ("retime-group-edit-audio-scale", "#RVRetime.audio.scale", 0.05, 1.0),
    ("retime-group-edit-audio-offset", "#RVRetime.audio.offset", 0.05, 0.0),
)
_TEXT_ENTRY_COMMIT_EVENT = "retime-group-text-entry-commit"


def _sessionManagerMode():
    """The session manager mode, or None when it is not loaded.

    The session State field the Mu modes read has no Python binding, so the mode is
    reached through the sibling module's own accessor. Callers must tolerate None:
    there is no editor panel to populate unless the session manager is loaded.
    """
    try:
        import session_manager

        return session_manager.theMode()
    except Exception:
        return None


def _loadUIFile(path, parent):
    """Build a widget from a Qt Designer file; loadUIFile has no Python binding."""
    loader = QUiLoader()
    uiFile = QtCore.QFile(path)
    uiFile.open(QtCore.QIODevice.ReadOnly)

    try:
        return loader.load(uiFile, parent)
    finally:
        uiFile.close()


def _bindParameterModes(modeName):
    """Bind rvui's parameter scrubbing handlers into this mode's event table.

    rvui.startParameterMode() returns a Mu event handler that pushes the paramscrub
    event table and draws a feedback glyph; neither has a Python binding, and a Mu
    event handler cannot be called from Python because it needs an Event. Binding it
    to an internal event name lets the menu items reach it through
    sendInternalEvent(). The mode must already be defined, so this runs after init().
    """
    binds = "".join(
        'commands.bind("%s", "global", "%s", '
        'rvui.startParameterMode("%s", %r, %r), "Edit %s"); '
        % (modeName, event, param, scale, reset, param)
        for event, param, scale, reset in _PARAMETER_MODES
    )

    rv.runtime.eval('{ %s"ok"; }' % binds, ["rvtypes", "commands", "rvui"])


def _startTextEntryMode(prompt, commitEvent):
    """Start RV's text entry mode, committing the entered text as an internal event.

    rvui.startTextEntryMode() wants a Mu prompt function and a Mu commit function,
    which Python cannot supply, so the session State fields it would set are set here
    instead and the text comes back as `commitEvent`. Unlike the Mu original this
    cannot seed the entry with a digit typed to open it, which only applies to key
    bindings; the menu items this serves open the entry empty either way.
    """
    escaped = prompt.replace("\\", "\\\\").replace('"', '\\"')

    rv.runtime.eval(
        "{ State s = data(); "
        's.prompt = "%s"; '
        's.textFunc = \\: (void; string t) { sendInternalEvent("%s", t); }; '
        "s.textEntry = true; "
        "s.textOkWhenEmpty = false; "
        's.text = ""; '
        'pushEventTable("textentry"); '
        "redraw(); "
        '"ok"; }' % (escaped, commitEvent),
        ["rvtypes", "commands", "rvui"],
    )


def _enabledItem():
    return commands.NeutralMenuState


def _disabledItem():
    return commands.DisabledMenuState


class RetimeGroupEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def reset(self):
        setFloatProp("#RVRetime.visual.scale", 1.0)
        setFloatProp("#RVRetime.visual.offset", 0.0)
        setFloatProp("#RVRetime.audio.scale", 1.0)
        setFloatProp("#RVRetime.audio.offset", 0.0)
        commands.redraw()

    def reverse(self):
        length = commands.frameEnd() - commands.frameStart()
        scl = commands.getFloatProperty("#RVRetime.visual.scale")[0]

        if scl < 0:
            setFloatProp("#RVRetime.visual.scale", 1.0)
            setIntProp("#RVRetime.visual.offset", 0)
            setFloatProp("#RVRetime.audio.scale", 1.0)
            setIntProp("#RVRetime.audio.offset", 0)
        else:
            setFloatProp("#RVRetime.visual.scale", -1.0)
            setFloatProp("#RVRetime.visual.offset", float(-length))
            setFloatProp("#RVRetime.audio.scale", 1.0)
            setIntProp("#RVRetime.audio.offset", 0)

        commands.redraw()

    def updateUI(self):
        if self._ui is None:
            return

        fps = commands.getFloatProperty("#RVRetime.output.fps")[0]
        vscale = commands.getFloatProperty("#RVRetime.visual.scale")[0]
        ascale = commands.getFloatProperty("#RVRetime.audio.scale")[0]
        voffset = commands.getFloatProperty("#RVRetime.visual.offset")[0]
        aoffset = commands.getFloatProperty("#RVRetime.audio.offset")[0]

        self._fpsEdit.setText("%g" % fps)
        self._vscaleEdit.setText("%g" % vscale)
        self._ascaleEdit.setText("%g" % ascale)
        self._voffsetEdit.setText("%g" % voffset)
        self._aoffsetEdit.setText("%g" % aoffset)

    def resetSlot(self, checked):
        self.reset()

    def reverseSlot(self, checked):
        self.reverse()

    def editSlot(self, lineEdit, prop):
        def F():
            v = float(lineEdit.text())
            setFloatProp("#RVRetime" + prop, v)
            if prop == ".output.fps":
                commands.setFPS(v)
            commands.redraw()

        return F

    def loadUI(self, event):
        manager = _sessionManagerMode()

        if manager is not None:
            #
            #  The .ui tree below is parented to the session window, so the wrapper for it
            #  has to outlive them: dropping the last Python reference to a wrapper obtained
            #  from wrapInstance() takes the widgets parented to it down with it, and the
            #  panel then raises "Internal C++ object already deleted".
            #
            self._mainWindow = qtutils.sessionWindow()
            m = self._mainWindow

            if self._ui is None:
                self._ui = _loadUIFile(manager.auxFilePath("retime.ui"), m)
                self._fpsEdit = self._ui.findChild(QtWidgets.QLineEdit, "fpsEdit")
                self._ascaleEdit = self._ui.findChild(QtWidgets.QLineEdit, "ascaleEdit")
                self._vscaleEdit = self._ui.findChild(QtWidgets.QLineEdit, "vscaleEdit")
                self._aoffsetEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "aoffsetEdit"
                )
                self._voffsetEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "voffsetEdit"
                )
                self._resetButton = self._ui.findChild(
                    QtWidgets.QPushButton, "resetButton"
                )
                self._reverseButton = self._ui.findChild(
                    QtWidgets.QPushButton, "reverseButton"
                )

                manager.addEditor("Retime", self._ui)

                self._resetButton.clicked.connect(self.resetSlot)
                self._reverseButton.clicked.connect(self.reverseSlot)

                for edit, prop in [
                    (self._fpsEdit, ".output.fps"),
                    (self._ascaleEdit, ".audio.scale"),
                    (self._vscaleEdit, ".visual.scale"),
                    (self._aoffsetEdit, ".audio.offset"),
                    (self._voffsetEdit, ".visual.offset"),
                ]:
                    edit.editingFinished.connect(self.editSlot(edit, prop))

            self.updateUI()
            manager.useEditor("Retime")

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        node = parts[0]

        if commands.nodeType(node) == "RVRetime":
            self.updateUI()

        event.reject()

    def factorPrompt(self, fmt, invert):
        factor = commands.getFloatProperty("#RVRetime.visual.scale")[0]
        return fmt % (1.0 / factor if invert else factor)

    def slowDownPrompt(self):
        return self.factorPrompt("Slow Down by Factor (current=%g):", True)

    def speedUpPrompt(self):
        return self.factorPrompt("Speed Up by Factor (current=%g):", False)

    def setFactorValue(self, text, invert):
        factor = 1.0 / float(text) if invert else float(text)
        setFloatProp("#RVRetime.visual.scale", factor)
        commands.redraw()

    def fpsPrompt(self):
        return (
            "Convert to FPS (current=%g):"
            % commands.getFloatProperty("#RVRetime.output.fps")[0]
        )

    def setConvertFPS(self, text):
        newFPS = float(text)
        setFloatProp("#RVRetime.output.fps", newFPS)
        commands.setFPS(newFPS)

    def convertToFPS(self, event, newFPS):
        for src in commands.sourcesRendered():
            setFloatProp("#RVRetime.output.fps", newFPS)

        commands.setFPS(newFPS)

    def resetTiming(self, event):
        self.reset()

    def reverseTiming(self, event):
        self.reverse()

    def editVScale(self, event):
        commands.sendInternalEvent("retime-group-edit-visual-scale", "")

    def editVOffset(self, event):
        commands.sendInternalEvent("retime-group-edit-visual-offset", "")

    def editAScale(self, event):
        commands.sendInternalEvent("retime-group-edit-audio-scale", "")

    def editAOffset(self, event):
        commands.sendInternalEvent("retime-group-edit-audio-offset", "")

    def slowDownFactor(self, event):
        self._textCommit = functools.partial(self.setFactorValue, invert=True)
        _startTextEntryMode(self.slowDownPrompt(), _TEXT_ENTRY_COMMIT_EVENT)

    def speedUpFactor(self, event):
        self._textCommit = functools.partial(self.setFactorValue, invert=False)
        _startTextEntryMode(self.speedUpPrompt(), _TEXT_ENTRY_COMMIT_EVENT)

    def editFPS(self, event):
        self._textCommit = self.setConvertFPS
        _startTextEntryMode(self.fpsPrompt(), _TEXT_ENTRY_COMMIT_EVENT)

    def textEntryCommitted(self, event):
        commit = self._textCommit
        self._textCommit = None

        if commit is not None:
            commit(event.contents())

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._fpsEdit = None
        self._voffsetEdit = None
        self._aoffsetEdit = None
        self._vscaleEdit = None
        self._ascaleEdit = None
        self._reverseButton = None
        self._resetButton = None
        self._textCommit = None

        self.init(
            "RetimeGroup_edit_mode",
            None,
            [
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
                (
                    _TEXT_ENTRY_COMMIT_EVENT,
                    self.textEntryCommitted,
                    "Apply text entered in the retime prompts",
                ),
            ],
            [
                (
                    "Retime",
                    [
                        (
                            "Convert to FPS",
                            [
                                menuItem(
                                    "24",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=24.0),
                                    _enabledItem,
                                ),
                                menuItem(
                                    "25",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=25.0),
                                    _enabledItem,
                                ),
                                menuItem(
                                    "23.98",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=23.98),
                                    _enabledItem,
                                ),
                                menuItem(
                                    "29.97",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=29.97),
                                    _enabledItem,
                                ),
                                menuItem(
                                    "30",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=30.0),
                                    _enabledItem,
                                ),
                                menuItem(
                                    "59.94",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=59.94),
                                    _enabledItem,
                                ),
                                menuItem(
                                    "60",
                                    "",
                                    "viewmode_category",
                                    functools.partial(self.convertToFPS, newFPS=60.0),
                                    _enabledItem,
                                ),
                                ("_", None),
                                menuItem(
                                    "Custom...",
                                    "",
                                    "viewmode_category",
                                    self.editFPS,
                                    _enabledItem,
                                ),
                            ],
                        ),
                        ("_", None),
                        menuItem(
                            "Slow Down by Factor...",
                            "",
                            "viewmode_category",
                            self.slowDownFactor,
                            _enabledItem,
                        ),
                        menuItem(
                            "Speed Up By Factor...",
                            "",
                            "viewmode_category",
                            self.speedUpFactor,
                            _enabledItem,
                        ),
                        menuItem(
                            "Reverse",
                            "",
                            "viewmode_category",
                            self.reverseTiming,
                            _enabledItem,
                        ),
                        ("_", None),
                        ("Edit Raw", None, None, _disabledItem),
                        menuItem(
                            "    Visual Scale...",
                            "",
                            "viewmode_category",
                            self.editVScale,
                            _enabledItem,
                        ),
                        menuItem(
                            "    Visual Offset...",
                            "",
                            "viewmode_category",
                            self.editVOffset,
                            _enabledItem,
                        ),
                        menuItem(
                            "    Audio Scale...",
                            "",
                            "viewmode_category",
                            self.editAScale,
                            _enabledItem,
                        ),
                        menuItem(
                            "    Audio Offset...",
                            "",
                            "viewmode_category",
                            self.editAOffset,
                            _enabledItem,
                        ),
                        ("_", None),
                        menuItem(
                            "Reset Timing",
                            "",
                            "viewmode_category",
                            self.resetTiming,
                            _enabledItem,
                        ),
                    ],
                )
            ],
            None,
        )

        _bindParameterModes("RetimeGroup_edit_mode")


def createMode():
    return RetimeGroupEditMode()
