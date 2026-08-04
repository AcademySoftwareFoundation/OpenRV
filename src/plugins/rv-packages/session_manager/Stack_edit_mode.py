#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Stack edit mode — Python port of Stack_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import functools
import os
import sys

import rv.commands as commands
import rv.extra_commands as extra_commands
import rv.qtutils as qtutils
import rv.rvtypes

from PySide6 import QtCore, QtWidgets
from PySide6.QtCore import Qt
from PySide6.QtUiTools import QUiLoader

from session_manager import checkStateIsChecked, menuItem, setFloatProp, setIntProp, setStringProp


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


class StackEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def updateUI(self):
        vnode = commands.viewNode()
        vnodeExists = vnode is not None

        if self._ui is None or not vnodeExists:
            return

        self._uiInFlux = True

        try:
            a = commands.getIntProperty("#RVStack.mode.alignStartFrames")[0]
            st = commands.getIntProperty("#RVStack.mode.strictFrameRanges")[0]
            u = commands.getIntProperty("#RVStack.mode.useCutInfo")[0]
            c = commands.getStringProperty("#RVStack.output.chosenAudioInput")[0]
            asize = commands.getIntProperty("#RVStack.output.autoSize")[0]
            size = commands.getIntProperty("#RVStack.output.size")
            fps = commands.getFloatProperty("#RVStack.output.fps")[0]
            isize = commands.getIntProperty("#RVStack.output.interactiveSize")[0]

            self._alignCheckBox.setCheckState(Qt.Unchecked if a == 0 else Qt.Checked)
            self._strictRangesCheckBox.setCheckState(
                Qt.Unchecked if st == 0 else Qt.Checked
            )
            self._useCutInfoCheckBox.setCheckState(
                Qt.Unchecked if u == 0 else Qt.Checked
            )
            self._autoSizeCheckBox.setCheckState(
                Qt.Unchecked if asize == 0 else Qt.Checked
            )
            self._interactiveSizeCheckBox.setCheckState(
                Qt.Unchecked if isize == 0 else Qt.Checked
            )

            self._chosenAudioInputCombo.clear()
            self._chosenAudioInputCombo.addItem("All Inputs Mixed", ".all.")
            self._chosenAudioInputCombo.addItem("First Input Only", ".first.")
            self._chosenAudioInputCombo.addItem("First Visible Input", ".topmost.")

            chosenIndex = 0
            inputs = commands.nodeConnections(commands.viewNode(), False)[0]

            if c == ".first.":
                chosenIndex = 1
            if c == ".topmost.":
                chosenIndex = 2

            for i, inputNode in enumerate(inputs):
                self._chosenAudioInputCombo.addItem(
                    extra_commands.uiName(inputNode), inputNode
                )
                #
                #  i+3 because we used the first three slots for "play
                #  everything" and "play first only" and "play first visible"
                #
                if inputNode == c:
                    chosenIndex = i + 3

            self._chosenAudioInputCombo.setCurrentIndex(chosenIndex)

            self._outputWidthEdit.setEnabled(asize == 0)
            self._outputHeightEdit.setEnabled(asize == 0)

            self._outputFPSEdit.setText("%g" % fps)
            self._outputWidthEdit.setText("%d" % size[0])
            self._outputHeightEdit.setText("%d" % size[-1])

            retimeProp = "#View.timing.retimeInputs"

            if commands.propertyExists(retimeProp):
                self._retimeCheckBox.setCheckState(
                    Qt.Checked
                    if commands.getIntProperty(retimeProp)[0] == 1
                    else Qt.Unchecked
                )
        except Exception:
            pass

        commands.redraw()
        self._uiInFlux = False

    def updateUIEvent(self, event):
        event.reject()
        self.updateUI()

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        comp = parts[1]
        name = parts[2]

        if comp == "mode" or comp == "output":
            if name in (
                "alignStartFrames",
                "strictFrameRanges",
                "useCutInfo",
                "chosenAudioInput",
                "size",
                "autoSize",
                "fps",
                "interactiveSize",
            ):
                if self._ui is not None:
                    self.updateUI()

        event.reject()

    def checkBoxSlot(self, state, name):
        v = commands.getIntProperty(name)[0]
        newV = 1 if checkStateIsChecked(state) else 0

        if v != newV:
            setIntProp(name, newV)

    def updateMenu(self):
        self.setMenu(self.menu())

    def setChosenAudioInput(self, index):
        if self._uiInFlux:
            return

        currentName = commands.getStringProperty("#RVStack.output.chosenAudioInput")[0]
        name = ".all."

        if index >= 0 and index < self._chosenAudioInputCombo.count():
            data = self._chosenAudioInputCombo.itemData(index, Qt.UserRole)
            name = "" if data is None else str(data)

        if name != currentName:
            setStringProp("#RVStack.output.chosenAudioInput", name)
            commands.redraw()

    def fpsChanged(self):
        newFPS = float(self._outputFPSEdit.text())

        try:
            setFloatProp("#RVStack.output.fps", newFPS)
            commands.setFPS(newFPS)
        except Exception:
            pass

        commands.redraw()

    def widthChanged(self):
        val = float(self._outputWidthEdit.text())
        prop = commands.getIntProperty("#RVStack.output.size")

        commands.setIntProperty("#RVStack.output.size", [int(val), prop[-1]])
        commands.redraw()

    def heightChanged(self):
        val = float(self._outputHeightEdit.text())
        prop = commands.getIntProperty("#RVStack.output.size")

        commands.setIntProperty("#RVStack.output.size", [prop[0], int(val)])
        commands.redraw()

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
                self._ui = _loadUIFile(manager.auxFilePath("stack.ui"), m)
                self._alignCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "alignCheckBox"
                )
                self._strictRangesCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "strictRangesCheckBox"
                )
                self._useCutInfoCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "useCutInfoCheckBox"
                )
                self._retimeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "retimeInputsCheckBox"
                )
                self._autoSizeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "autoSizeCheckBox"
                )
                self._chosenAudioInputCombo = self._ui.findChild(
                    QtWidgets.QComboBox, "chosenAudioInputCombo"
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
                self._interactiveSizeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "interactiveResizeCheckBox"
                )

                manager.addEditor("Stack", self._ui)

                self._alignCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVStack.mode.alignStartFrames"
                    )
                )
                self._strictRangesCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVStack.mode.strictFrameRanges"
                    )
                )
                self._useCutInfoCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVStack.mode.useCutInfo"
                    )
                )
                self._autoSizeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVStack.output.autoSize"
                    )
                )
                self._retimeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#View.timing.retimeInputs"
                    )
                )
                self._interactiveSizeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVStack.output.interactiveSize"
                    )
                )

                self._chosenAudioInputCombo.currentIndexChanged.connect(
                    self.setChosenAudioInput
                )
                self._outputFPSEdit.editingFinished.connect(self.fpsChanged)
                self._outputWidthEdit.editingFinished.connect(self.widthChanged)
                self._outputHeightEdit.editingFinished.connect(self.heightChanged)

            self.updateUI()
            manager.useEditor("Stack")

        event.reject()

    def alignStartFrames(self, event):
        p = "#RVStack.mode.alignStartFrames"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def strictFrameRanges(self, event):
        p = "#RVStack.mode.strictFrameRanges"
        s = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if s != 0 else 1)

    def useCutInfo(self, event):
        p = "#RVStack.mode.useCutInfo"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def stateFunc(self, name):
        def F():
            p = commands.getIntProperty("#RVStack.mode.%s" % name)[0]
            return commands.UncheckedMenuState if p == 0 else commands.CheckedMenuState

        return F

    def retimeState(self):
        p = commands.getIntProperty("#View.timing.retimeInputs")[0]
        return commands.UncheckedMenuState if p == 0 else commands.CheckedMenuState

    def autoRetimeInputs(self, event):
        p = "#View.timing.retimeInputs"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def menu(self):
        n = commands.viewNode()
        t = commands.nodeType(n)
        name = "Layout" if t == "RVLayoutGroup" else "Stack"

        return [
            (
                name,
                [
                    ("_", None),
                    menuItem(
                        "Align Start Frames",
                        "",
                        "viewmode_category",
                        self.alignStartFrames,
                        self.stateFunc("alignStartFrames"),
                    ),
                    menuItem(
                        "Use Source Cut Info",
                        "",
                        "viewmode_category",
                        self.useCutInfo,
                        self.stateFunc("useCutInfo"),
                    ),
                    menuItem(
                        "Automatically Retime Inputs",
                        "",
                        "viewmode_category",
                        self.autoRetimeInputs,
                        self.retimeState,
                    ),
                    menuItem(
                        "Use Strict Frame Ranges",
                        "",
                        "viewmode_category",
                        self.strictFrameRanges,
                        self.stateFunc("strictFrameRanges"),
                    ),
                ],
            )
        ]

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._alignCheckBox = None
        self._strictRangesCheckBox = None
        self._useCutInfoCheckBox = None
        self._retimeCheckBox = None
        self._autoSizeCheckBox = None
        self._interactiveSizeCheckBox = None
        self._chosenAudioInputCombo = None
        self._outputFPSEdit = None
        self._outputWidthEdit = None
        self._outputHeightEdit = None
        self._uiInFlux = False

        self.init(
            "Stack_edit_mode",
            None,
            [
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("range-changed", self.updateUIEvent, "Update UI"),
                ("image-structure-change", self.updateUIEvent, "Update UI"),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
            ],
            None,
            "z",
        )


def createMode():
    return StackEditMode()
