#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Sequence group edit mode — Python port of SequenceGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import functools
import os
import sys

import rv.commands as commands
import rv.qtutils as qtutils
import rv.rvtypes

from PySide6 import QtCore, QtWidgets
from PySide6.QtCore import Qt
from PySide6.QtUiTools import QUiLoader

from session_manager import checkStateIsChecked, menuItem, setFloatProp, setIntProp


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


class SequenceGroupEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def beforeSessionRead(self, event):
        self._disableUpdates = True
        event.reject()

    def afterSessionRead(self, event):
        self._disableUpdates = False
        self.updateUI()
        event.reject()

    def updateUI(self):
        if self._ui is None or self._disableUpdates:
            return

        try:
            if not commands.propertyExists("#RVSequence.mode.autoEDL"):
                return
        except Exception:
            return

        a = commands.getIntProperty("#RVSequence.mode.autoEDL")[0]
        u = commands.getIntProperty("#RVSequence.mode.useCutInfo")[0]
        r = commands.getIntProperty("#RVSequenceGroup.timing.retimeInputs")[0]
        fps = commands.getFloatProperty("#RVSequence.output.fps")[0]
        asize = commands.getIntProperty("#RVSequence.output.autoSize")[0]
        size = commands.getIntProperty("#RVSequence.output.size")
        isize = commands.getIntProperty("#RVSequence.output.interactiveSize")[0]

        self._outputWidthEdit.setEnabled(asize == 0 and isize == 0)
        self._outputHeightEdit.setEnabled(asize == 0 and isize == 0)

        self._autoEDLCheckBox.setCheckState(Qt.Unchecked if a == 0 else Qt.Checked)
        self._useCutInfoCheckBox.setCheckState(Qt.Unchecked if u == 0 else Qt.Checked)
        self._retimeCheckBox.setCheckState(Qt.Unchecked if r == 0 else Qt.Checked)
        self._autoSizeCheckBox.setCheckState(Qt.Unchecked if asize == 0 else Qt.Checked)
        self._outputFPSEdit.setText("%g" % fps)
        self._outputWidthEdit.setText("%d" % size[0])
        self._outputHeightEdit.setText("%d" % size[-1])
        self._interactiveSizeCheckBox.setCheckState(
            Qt.Unchecked if isize == 0 else Qt.Checked
        )

    def updateUIEvent(self, event):
        event.reject()
        self.updateUI()

    def fpsChanged(self):
        newFPS = float(self._outputFPSEdit.text())
        oldFPS = commands.getFloatProperty("#RVSequence.output.fps")[0]
        if newFPS != oldFPS:
            setFloatProp("#RVSequence.output.fps", newFPS)
            commands.setFPS(newFPS)
            commands.redraw()

    def widthChanged(self):
        val = float(self._outputWidthEdit.text())
        prop = commands.getIntProperty("#RVSequence.output.size")

        commands.setIntProperty("#RVSequence.output.size", [int(val), prop[-1]])
        commands.redraw()

    def heightChanged(self):
        val = float(self._outputHeightEdit.text())
        prop = commands.getIntProperty("#RVSequence.output.size")

        commands.setIntProperty("#RVSequence.output.size", [prop[0], int(val)])
        commands.redraw()

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        comp = parts[1]
        name = parts[2]

        #
        #  If a UI name changes we need to update the tree
        #

        if comp == "mode" or comp == "output":
            if name in (
                "autoEDL",
                "autoSize",
                "useCutInfo",
                "width",
                "fps",
                "height",
                "interactiveSize",
            ):
                self.updateUI()
                commands.redraw()

        event.reject()

    def checkBoxSlot(self, state, name):
        current = commands.getIntProperty(name)[0]
        value = 1 if checkStateIsChecked(state) else 0
        if value != current:
            setIntProp(name, value)

    def activateUI(self):
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
                self._ui = _loadUIFile(manager.auxFilePath("sequence.ui"), m)
                self._autoEDLCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "autoEDLCheckBox"
                )
                self._useCutInfoCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "useCutInfoCheckBox"
                )
                self._retimeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "retimeInputsCheckBox"
                )
                self._outputFPSEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "outputFPSEdit"
                )
                self._outputWidthEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "outputWidthEdit"
                )
                self._outputHeightEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "outputHeightEdit"
                )
                self._autoSizeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "autoSizeCheckBox"
                )
                self._interactiveSizeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "interactiveResizeCheckBox"
                )
                manager.addEditor("Sequence", self._ui)

                self._autoEDLCheckBox.stateChanged.connect(
                    functools.partial(self.checkBoxSlot, name="#RVSequence.mode.autoEDL")
                )
                self._useCutInfoCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSequence.mode.useCutInfo"
                    )
                )
                self._autoSizeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSequence.output.autoSize"
                    )
                )
                self._retimeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSequenceGroup.timing.retimeInputs"
                    )
                )
                self._interactiveSizeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSequence.output.interactiveSize"
                    )
                )

                self._outputFPSEdit.editingFinished.connect(self.fpsChanged)
                self._outputWidthEdit.editingFinished.connect(self.widthChanged)
                self._outputHeightEdit.editingFinished.connect(self.heightChanged)

            self.updateUI()
            manager.useEditor("Sequence")

    def loadUI(self, event):
        self._disableUpdates = False
        self.activateUI()
        event.reject()

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)
        self._disableUpdates = False
        self.activateUI()

    def autoEDL(self, event):
        p = "#RVSequence.mode.autoEDL"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def useCutInfo(self, event):
        p = "#RVSequence.mode.useCutInfo"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def stateFunc(self, name):
        def F():
            p = commands.getIntProperty("#RVSequence.mode.%s" % name)[0]
            return commands.UncheckedMenuState if p == 0 else commands.CheckedMenuState

        return F

    def menu(self):
        return [
            (
                "Sequence",
                [
                    ("_", None),
                    menuItem(
                        "Auto EDL",
                        "",
                        "viewmode_category",
                        self.autoEDL,
                        self.stateFunc("autoEDL"),
                    ),
                    menuItem(
                        "Use Source Cut Info",
                        "",
                        "viewmode_category",
                        self.useCutInfo,
                        self.stateFunc("useCutInfo"),
                    ),
                ],
            )
        ]

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._autoEDLCheckBox = None
        self._useCutInfoCheckBox = None
        self._retimeCheckBox = None
        self._autoSizeCheckBox = None
        self._interactiveSizeCheckBox = None
        self._outputFPSEdit = None
        self._outputWidthEdit = None
        self._outputHeightEdit = None
        self._disableUpdates = False

        self.init(
            "SequenceGroup_edit_mode",
            None,
            [
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("range-changed", self.updateUIEvent, "Update UI on range change"),
                (
                    "image-structure-change",
                    self.updateUIEvent,
                    "Update UI on range change",
                ),
                ("before-session-read", self.beforeSessionRead, "Freeze Updates"),
                ("after-session-read", self.afterSessionRead, "Resume Updates"),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
            ],
            self.menu(),
            None,
        )


def createMode():
    return SequenceGroupEditMode()
