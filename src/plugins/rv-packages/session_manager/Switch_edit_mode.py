#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Switch edit mode — Python port of Switch_edit_mode.mu.

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

from session_manager import checkStateIsChecked, menuItem, setIntProp, setStringProp


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


class SwitchEditMode(rv.rvtypes.MinorMode):
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
            a = commands.getIntProperty("#RVSwitch.mode.alignStartFrames")[0]
            u = commands.getIntProperty("#RVSwitch.mode.useCutInfo")[0]
            c = commands.getStringProperty("#RVSwitch.output.input")[0]
            asize = commands.getIntProperty("#RVSwitch.output.autoSize")[0]
            size = commands.getIntProperty("#RVSwitch.output.size")

            self._alignCheckBox.setCheckState(Qt.Unchecked if a == 0 else Qt.Checked)
            self._useCutInfoCheckBox.setCheckState(
                Qt.Unchecked if u == 0 else Qt.Checked
            )
            self._autoSizeCheckBox.setCheckState(
                Qt.Unchecked if asize == 0 else Qt.Checked
            )

            self._selectedInputCombo.clear()

            selectedIndex = 0
            inputs = commands.nodeConnections(commands.viewNode(), False)[0]

            for i, inputNode in enumerate(inputs):
                self._selectedInputCombo.addItem(
                    extra_commands.uiName(inputNode), inputNode
                )
                if inputNode == c:
                    selectedIndex = i

            self._selectedInputCombo.setCurrentIndex(selectedIndex)

            self._outputWidthEdit.setEnabled(asize == 0)
            self._outputHeightEdit.setEnabled(asize == 0)

            self._outputWidthEdit.setText("%d" % size[0])
            self._outputHeightEdit.setText("%d" % size[-1])
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
                "useCutInfo",
                "input",
                "size",
                "autoSize",
            ):
                if self._ui is not None:
                    self.updateUI()

        event.reject()

    def checkBoxSlot(self, state, name):
        setIntProp(name, 1 if checkStateIsChecked(state) else 0)

    def updateMenu(self):
        self.setMenu(self.menu())

    def setSelectedInput(self, index):
        if self._uiInFlux:
            return

        currentName = commands.getStringProperty("#RVSwitch.output.input")[0]
        name = ""

        if index >= 0 and index < self._selectedInputCombo.count():
            data = self._selectedInputCombo.itemData(index, Qt.UserRole)
            name = "" if data is None else str(data)

        if name != currentName:
            setStringProp("#RVSwitch.output.input", name)
            commands.redraw()

    def widthChanged(self):
        val = float(self._outputWidthEdit.text())
        prop = commands.getIntProperty("#RVSwitch.output.size")

        commands.setIntProperty("#RVSwitch.output.size", [int(val), prop[-1]])
        commands.redraw()

    def heightChanged(self):
        val = float(self._outputHeightEdit.text())
        prop = commands.getIntProperty("#RVSwitch.output.size")

        commands.setIntProperty("#RVSwitch.output.size", [prop[0], int(val)])
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
                self._ui = _loadUIFile(manager.auxFilePath("switch.ui"), m)
                self._alignCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "alignCheckBox"
                )
                self._useCutInfoCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "useCutInfoCheckBox"
                )
                self._autoSizeCheckBox = self._ui.findChild(
                    QtWidgets.QCheckBox, "autoSizeCheckBox"
                )
                self._selectedInputCombo = self._ui.findChild(
                    QtWidgets.QComboBox, "selectedInputCombo"
                )
                self._outputWidthEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "outputWidthEdit"
                )
                self._outputHeightEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "outputHeightEdit"
                )

                manager.addEditor("Switch", self._ui)

                self._alignCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSwitch.mode.alignStartFrames"
                    )
                )
                self._useCutInfoCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSwitch.mode.useCutInfo"
                    )
                )
                self._autoSizeCheckBox.stateChanged.connect(
                    functools.partial(
                        self.checkBoxSlot, name="#RVSwitch.output.autoSize"
                    )
                )

                self._selectedInputCombo.currentIndexChanged.connect(
                    self.setSelectedInput
                )
                self._outputWidthEdit.editingFinished.connect(self.widthChanged)
                self._outputHeightEdit.editingFinished.connect(self.heightChanged)

            self.updateUI()
            manager.useEditor("Switch")

        event.reject()

    def alignStartFrames(self, event):
        p = "#RVSwitch.mode.alignStartFrames"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def useCutInfo(self, event):
        p = "#RVSwitch.mode.useCutInfo"
        a = commands.getIntProperty(p)[0]
        setIntProp(p, 0 if a != 0 else 1)

    def stateFunc(self, name):
        def F():
            p = commands.getIntProperty("#RVSwitch.mode.%s" % name)[0]
            return commands.UncheckedMenuState if p == 0 else commands.CheckedMenuState

        return F

    def retimeState(self):
        p = commands.getIntProperty("#View.timing.retimeInputs")[0]
        return commands.UncheckedMenuState if p == 0 else commands.CheckedMenuState

    def menu(self):
        return [
            (
                "Switch",
                [
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
        self._useCutInfoCheckBox = None
        self._autoSizeCheckBox = None
        self._selectedInputCombo = None
        self._outputWidthEdit = None
        self._outputHeightEdit = None
        self._uiInFlux = False

        self.init(
            "Switch_edit_mode",
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
            self.menu(),
            "z0",
        )


def createMode():
    return SwitchEditMode()
