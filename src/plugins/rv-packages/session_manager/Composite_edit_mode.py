#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Composite edit mode — Python port of Composite_edit_mode.mu.

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

from session_manager import menuItem, setFloatProp, setStringProp

_OP_NAMES = (
    "over",
    "add",
    "dissolve",
    "difference",
    "-difference",
    "replace",
    "topmost",
)


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


def _smartCycleInputs(forward):
    """Cycle the inputs of the view node, or of the stack it is looking through.

    rvui.smartCycleInputs() decides which node to cycle from the current input kept on
    the session State, which has no Python binding, so the whole cycle runs in Mu.
    """
    rv.runtime.eval(
        '{ rvui.smartCycleInputs(%s); "ok"; }' % ("true" if forward else "false"),
        ["rvtypes", "commands", "rvui"],
    )


def _cycleStackForward(event):
    _smartCycleInputs(True)


def _cycleStackBackward(event):
    _smartCycleInputs(False)


def _isStackMode():
    typeName = commands.nodeType(commands.viewNode())

    if typeName == "RVStackGroup" or typeName == "RVLayoutGroup":
        return commands.UncheckedMenuState

    return commands.DisabledMenuState


def _disabledItem():
    return commands.DisabledMenuState


class CompositeEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def setOp(self, index):
        name = "over"

        if index >= 0 and index < len(_OP_NAMES):
            name = _OP_NAMES[index]

        setStringProp("#RVStack.composite.type", name)

        # Force UI update immediately after changing blend mode
        self.updateUI()

        commands.redraw()

    def setOpEvent(self, event, index):
        self.setOp(index)

    def setDissolveAmount(self):
        amountText = self._dissolveLineEdit.text()

        try:
            amount = float(amountText)
            if amount < 0.0:
                amount = 0.0
            if amount > 1.0:
                amount = 1.0

            self._dissolveSlider.setValue(int(amount * 100.0))

            setFloatProp("#RVStack.composite.dissolveAmount", [amount])
            commands.redraw()
        except Exception:
            self._dissolveLineEdit.setText("0.5")
            self._dissolveSlider.setValue(50)
            setFloatProp("#RVStack.composite.dissolveAmount", [0.5])
            commands.redraw()

    def setDissolveAmountFromSlider(self, value):
        amount = float(value) / 100.0

        self._dissolveLineEdit.setText("%g" % amount)

        setFloatProp("#RVStack.composite.dissolveAmount", [amount])
        commands.redraw()

    def updateUI(self):
        if self._ui is None:
            return

        currentType = commands.getStringProperty("#RVStack.composite.type")[0]
        index = _OP_NAMES.index(currentType) if currentType in _OP_NAMES else 7

        self._comboBox.setCurrentIndex(index)

        showDissolve = currentType == "dissolve"
        self._dissolveLineEdit.setVisible(showDissolve)
        self._dissolveLabel.setVisible(showDissolve)
        self._dissolveSlider.setVisible(showDissolve)

        self._ui.adjustSize()
        self._ui.updateGeometry()
        self._ui.update()

        parent = self._ui.parentWidget()

        if parent is not None:
            parent.adjustSize()
            parent.update()

        if showDissolve:
            try:
                amounts = commands.getFloatProperty("#RVStack.composite.dissolveAmount")
                if len(amounts) > 0:
                    amount = amounts[0]
                    self._dissolveLineEdit.setText("%g" % amount)
                    self._dissolveSlider.setValue(int(amount * 100.0))
            except Exception:
                self._dissolveLineEdit.setText("0.5")
                self._dissolveSlider.setValue(50)

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        comp = parts[1]
        name = parts[2]

        #
        #  If a UI name changes we need to update the tree
        #

        if comp == "composite":
            if name == "type":
                self.updateUI()
            elif name == "dissolveAmount":
                self.updateUI()

        event.reject()

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
                self._ui = _loadUIFile(manager.auxFilePath("composite.ui"), m)
                self._comboBox = self._ui.findChild(QtWidgets.QComboBox, "comboBox")
                self._dissolveLineEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "dissolveLineEdit"
                )
                self._dissolveLabel = self._ui.findChild(
                    QtWidgets.QLabel, "dissolveLabel"
                )
                self._dissolveSlider = self._ui.findChild(
                    QtWidgets.QSlider, "dissolveSlider"
                )

                self._dissolveLineEdit.setVisible(False)
                self._dissolveLabel.setVisible(False)
                self._dissolveSlider.setVisible(False)

                manager.addEditor("Composite Function", self._ui)
                self._comboBox.currentIndexChanged.connect(self.setOp)
                self._dissolveLineEdit.editingFinished.connect(self.setDissolveAmount)
                self._dissolveSlider.valueChanged.connect(
                    self.setDissolveAmountFromSlider
                )

            self.updateUI()
            manager.useEditor("Composite Function")

        event.reject()

    def opState(self, n):
        def F():
            op = commands.getStringProperty("#RVStack.composite.type")[0]
            return commands.CheckedMenuState if op == n else commands.UncheckedMenuState

        return F

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._comboBox = None
        self._dissolveLineEdit = None
        self._dissolveLabel = None
        self._dissolveSlider = None

        self.init(
            "Composite_edit_mode",
            None,
            [
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
            ],
            [
                (
                    "Stack",
                    [
                        ("Composite Operation", None, None, _disabledItem),
                        menuItem(
                            "   Over",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=0),
                            self.opState("over"),
                        ),
                        menuItem(
                            "   Add",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=1),
                            self.opState("add"),
                        ),
                        menuItem(
                            "   Dissolve",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=2),
                            self.opState("dissolve"),
                        ),
                        menuItem(
                            "   Difference",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=3),
                            self.opState("difference"),
                        ),
                        menuItem(
                            "   Inverted Difference",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=4),
                            self.opState("-difference"),
                        ),
                        menuItem(
                            "   Replace",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=5),
                            self.opState("replace"),
                        ),
                        menuItem(
                            "   Topmost",
                            "",
                            "viewmode_category",
                            functools.partial(self.setOpEvent, index=6),
                            self.opState("topmost"),
                        ),
                        ("_", None),
                        menuItem(
                            "Cycle Forward",
                            "",
                            "viewmode_category",
                            _cycleStackForward,
                            _isStackMode,
                        ),
                        menuItem(
                            "Cycle Backward",
                            "",
                            "viewmode_category",
                            _cycleStackBackward,
                            _isStackMode,
                        ),
                    ],
                )
            ],
            "b",
        )


def createMode():
    return CompositeEditMode()
