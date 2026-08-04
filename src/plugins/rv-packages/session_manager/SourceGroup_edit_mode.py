#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Source group edit mode — Python port of SourceGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import os
import sys

import rv.commands as commands
import rv.qtutils as qtutils
import rv.rvtypes

from PySide6 import QtCore, QtWidgets
from PySide6.QtCore import Qt
from PySide6.QtUiTools import QUiLoader

from session_manager import menuItem, setIntProp

# Mu's int.max, the value the cut properties carry when no cut point is set.
MU_INT_MAX = 2**31 - 1


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


def startTextEntryMode(prompt, func, okWhenEmpty=False):
    """Build the event handler rvui.startTextEntryMode() returns in Mu.

    RV's in-viewport text entry keeps its prompt and its commit callback in the Mu
    session State, and the callback has to be a Mu function pointer, so it cannot be
    driven from Python. A modal input dialog stands in for it.
    """

    def F(event):
        (text, ok) = QtWidgets.QInputDialog.getText(
            qtutils.sessionWindow(), "", prompt()
        )
        if ok and (okWhenEmpty or text != ""):
            func(text)

    return F


class SourceGroupEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def syncGuiInOut(self):
        p = "#RVFileSource.cut.syncGui"

        if commands.propertyExists(p):
            return commands.getIntProperty(p)[0] != 0

        return True

    def reset(self):
        self._locked = True
        try:
            if self.syncGuiInOut():
                commands.setInPoint(commands.frameStart())
                commands.setOutPoint(commands.frameEnd())
            setIntProp("#RVFileSource.cut.in", -MU_INT_MAX)
            setIntProp("#RVFileSource.cut.out", MU_INT_MAX)
        except Exception:
            pass
        self._locked = False
        self.updateUI()
        commands.redraw()

    def updateUI(self):
        if self._ui is None:
            return

        self._locked = True

        try:
            cutIn = commands.getIntProperty("#RVFileSource.cut.in")[0]
            cutOut = commands.getIntProperty("#RVFileSource.cut.out")[0]

            self._cutInEdit.setValue(cutIn)
            self._cutOutEdit.setValue(cutOut if cutOut != MU_INT_MAX else -MU_INT_MAX)

            self._syncCheckBox.setCheckState(
                Qt.CheckState.Checked
                if self.syncGuiInOut()
                else Qt.CheckState.Unchecked
            )
        except Exception:
            # The session may have been cleared.
            pass
        self._locked = False

    def resetSlot(self, checked):
        self.reset()

    def syncSlot(self, checked):
        if self._locked:
            return

        p = "#RVFileSource.cut.syncGui"

        setIntProp(p, 1 if checked else 0)
        if checked:
            self.updateFromProps()
        self.updateUI()

    def toggleSync(self, event):
        self.syncSlot(not self.syncGuiInOut())

    def changedSlot(self, prop):
        def F(v):
            if not self._locked and v != -MU_INT_MAX:
                if v < commands.frameStart():
                    return
                if v > commands.frameEnd():
                    return

                if prop == "in" and v > commands.outPoint():
                    return
                if prop == "out" and v < commands.inPoint():
                    return

                self._locked = True

                setIntProp("#RVFileSource.cut." + prop, v)

                try:
                    if self.syncGuiInOut() and prop == "in":
                        commands.setInPoint(v)
                    if self.syncGuiInOut() and prop == "out":
                        commands.setOutPoint(v)
                except Exception:
                    pass
                self._locked = False
            commands.redraw()

        return F

    def finishedSlot(self, prop):
        def F():
            v = self._cutInEdit.value() if prop == "in" else self._cutOutEdit.value()

            if v != -MU_INT_MAX:
                if v < commands.frameStart():
                    v = commands.frameStart()
                if v > commands.frameEnd():
                    v = commands.frameEnd()

                if prop == "in" and v > commands.outPoint():
                    v = commands.outPoint()
                if prop == "out" and v < commands.inPoint():
                    v = commands.inPoint()

                self._locked = True

                if prop == "in":
                    self._cutInEdit.setValue(v)
                if prop == "out":
                    self._cutOutEdit.setValue(v)

                setIntProp("#RVFileSource.cut." + prop, v)

                try:
                    if self.syncGuiInOut() and prop == "in":
                        commands.setInPoint(v)
                    if self.syncGuiInOut() and prop == "out":
                        commands.setOutPoint(v)
                except Exception:
                    pass
                self._locked = False
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
                self._ui = _loadUIFile(self.auxFilePath("source.ui"), m)

                self._cutInEdit = self._ui.findChild(QtWidgets.QSpinBox, "cutInEdit")
                self._cutInEdit.setRange(-MU_INT_MAX, MU_INT_MAX)
                self._cutInEdit.setSpecialValueText(" ")

                self._cutOutEdit = self._ui.findChild(QtWidgets.QSpinBox, "cutOutEdit")
                self._cutOutEdit.setRange(-MU_INT_MAX, MU_INT_MAX)
                self._cutOutEdit.setSpecialValueText(" ")

                self._resetButton = self._ui.findChild(
                    QtWidgets.QPushButton, "resetButton"
                )
                self._syncCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "syncCheckBox"
                )

                manager.addEditor("Source", self._ui)

                self._resetButton.clicked.connect(self.resetSlot)

                self._cutInEdit.editingFinished.connect(self.finishedSlot("in"))
                self._cutOutEdit.editingFinished.connect(self.finishedSlot("out"))

                self._cutInEdit.valueChanged.connect(self.changedSlot("in"))
                self._cutOutEdit.valueChanged.connect(self.changedSlot("out"))

                self._syncCheckBox.clicked.connect(self.syncSlot)

            self.updateUI()
            manager.useEditor("Source")

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        node = parts[0]

        if not self._locked and commands.nodeType(node) == "RVFileSource":
            self.updateUI()
            if self.syncGuiInOut():
                self.updateFromProps()
        event.reject()

    def cutInPrompt(self):
        v = commands.getIntProperty("#RVFileSource.cut.in")[0]

        if v == -MU_INT_MAX:
            return "Set Source In Point:"
        return "Set Source In Point (current=%d):" % v

    def cutOutPrompt(self):
        v = commands.getIntProperty("#RVFileSource.cut.out")[0]

        if v == MU_INT_MAX:
            return "Set Source Out Point:"
        return "Set Source Out Point (current=%d):" % v

    def setCutValue(self, prop, text):
        setIntProp("#RVFileSource.cut." + prop, int(text))
        commands.redraw()

    def resetCut(self, event):
        self.reset()

    def newInPoint(self, event):
        p = "#RVFileSource.cut.in"

        if not self._locked and self.syncGuiInOut() and commands.propertyExists(p):
            setIntProp(p, commands.inPoint())

        event.reject()

    def newOutPoint(self, event):
        p = "#RVFileSource.cut.out"

        if not self._locked and self.syncGuiInOut() and commands.propertyExists(p):
            setIntProp(p, commands.outPoint())

        event.reject()

    def updateFromProps(self):
        self._locked = True
        try:
            cutIn = commands.getIntProperty("#RVFileSource.cut.in")[0]
            cutOut = commands.getIntProperty("#RVFileSource.cut.out")[0]

            cutIn = min(max(cutIn, commands.frameStart()), commands.frameEnd())
            cutOut = min(max(cutOut, commands.frameStart()), commands.frameEnd())
            commands.setInPoint(cutIn)
            commands.setOutPoint(cutOut)
        except Exception:
            pass
        self._locked = False

    def activate(self):
        if self.syncGuiInOut():
            self.updateFromProps()

        rv.rvtypes.MinorMode.activate(self)

    def syncState(self):
        if self.syncGuiInOut():
            return commands.CheckedMenuState
        return commands.UncheckedMenuState

    def sourceMenuState(self):
        return commands.NeutralMenuState

    def menu(self, setCutInMode, setCutOutMode):
        return [
            (
                "Source",
                [
                    menuItem(
                        "Set Source Cut In ...",
                        "",
                        "source_category",
                        setCutInMode,
                        self.sourceMenuState,
                    ),
                    menuItem(
                        "Set Source Cut Out ...",
                        "",
                        "source_category",
                        setCutOutMode,
                        self.sourceMenuState,
                    ),
                    menuItem(
                        "Clear Source Cut In/Out",
                        "",
                        "source_category",
                        self.resetCut,
                        self.sourceMenuState,
                    ),
                    menuItem(
                        "Sync GUI With Source Cut In/Out",
                        "",
                        "source_category",
                        self.toggleSync,
                        self.syncState,
                    ),
                ],
            )
        ]

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._cutInEdit = None
        self._cutOutEdit = None
        self._syncCheckBox = None
        self._resetButton = None
        self._locked = False

        setCutInMode = startTextEntryMode(
            self.cutInPrompt, lambda text: self.setCutValue("in", text)
        )
        setCutOutMode = startTextEntryMode(
            self.cutOutPrompt, lambda text: self.setCutValue("out", text)
        )

        self.init(
            "SourceGroup_edit_mode",
            None,
            [
                ("new-in-point", self.newInPoint, "Update In Point"),
                ("new-out-point", self.newOutPoint, "Update Out Point"),
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
            ],
            self.menu(setCutInMode, setCutOutMode),
            None,
        )

        self._locked = False


def createMode():
    return SourceGroupEditMode()
