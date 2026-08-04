#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Layout group edit mode — Python port of LayoutGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import os
import sys

import rv.commands as commands
import rv.qtutils as qtutils
import rv.rvtypes
import rv.runtime

from PySide6 import QtCore, QtWidgets
from PySide6.QtUiTools import QUiLoader

from session_manager import menuItem


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


class LayoutGroupEditMode(rv.rvtypes.MinorMode):
    def auxFilePath(self, name):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), name
        )

    def layoutMode(self):
        modeProp = "#RVLayoutGroup.layout.mode"

        try:
            return commands.getStringProperty(modeProp)[0]
        except Exception:
            pass
        return ""

    def setLayoutMode(self, mode):
        modeProp = "#RVLayoutGroup.layout.mode"
        commands.setStringProperty(modeProp, [mode], True)

    def setSpacing(self, value):
        prop = "#RVLayoutGroup.layout.spacing"
        commands.setFloatProperty(prop, [value], True)

    def setGridRowsColumns(self, rows, columns):
        prop = "#RVLayoutGroup.layout."
        commands.setIntProperty(prop + "gridRows", [rows], True)
        commands.setIntProperty(prop + "gridColumns", [columns], True)

        self.setLayoutMode("grid")

    def updateUI(self):
        if self._ui is None:
            return

        try:
            self._modeCombo.setCurrentIndex(
                {
                    "packed": 0,
                    "packed2": 1,
                    "row": 2,
                    "column": 3,
                    "grid": 4,
                    "manual": 5,
                }.get(self.layoutMode(), 6)
            )

            sp = commands.getFloatProperty("#RVLayoutGroup.layout.spacing")[0]
            self._spacingSlider.setValue(
                int((max(0.5, min(1.0, sp)) * 2.0 - 1.0) * 999.0)
            )

            r = commands.getIntProperty("#RVLayoutGroup.layout.gridRows")[0]
            self._gridRowsLineEdit.setText("%d" % r)

            c = commands.getIntProperty("#RVLayoutGroup.layout.gridColumns")[0]
            self._gridColumnsLineEdit.setText("%d" % c)
        except Exception:
            self._modeCombo.setCurrentIndex(0)

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        comp = parts[1]
        name = parts[2]

        if comp == "layout" and self._ui is not None:
            if name in ("mode", "spacing", "gridRows", "gridColumns"):
                self.updateUI()
                commands.redraw()

        event.reject()

    def spacingSliderChangedSlot(self, value):
        self.setSpacing(float(value) / 999.0 / 2.0 + 0.5)

    def gridRowsChangedSlot(self):
        newRows = int(self._gridRowsLineEdit.text())

        self.setGridRowsColumns(newRows, 0)
        commands.redraw()

    def gridColumnsChangedSlot(self):
        newColumns = int(self._gridColumnsLineEdit.text())

        self.setGridRowsColumns(0, newColumns)
        commands.redraw()

    def modeComboChangedSlot(self, index):
        if index == 0:
            self.layoutPacked()
        elif index == 1:
            self.layoutPacked2()
        elif index == 2:
            self.layoutInRow()
        elif index == 3:
            self.layoutInColumn()
        elif index == 4:
            self.layoutInGrid()
        elif index == 5:
            self.layoutManually()
        else:
            self.layoutStatic()

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
                self._ui = _loadUIFile(self.auxFilePath("layout.ui"), m)
                self._modeCombo = self._ui.findChild(QtWidgets.QComboBox, "modeCombo")
                self._spacingSlider = self._ui.findChild(
                    QtWidgets.QSlider, "spacingSlider"
                )
                self._gridRowsLineEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "gridRowsLineEdit"
                )
                self._gridColumnsLineEdit = self._ui.findChild(
                    QtWidgets.QLineEdit, "gridColumnsLineEdit"
                )

                manager.addEditor("Layout", self._ui)
                self._modeCombo.currentIndexChanged.connect(self.modeComboChangedSlot)
                self._spacingSlider.sliderMoved.connect(self.spacingSliderChangedSlot)
                self._gridRowsLineEdit.editingFinished.connect(self.gridRowsChangedSlot)
                self._gridColumnsLineEdit.editingFinished.connect(
                    self.gridColumnsChangedSlot
                )

            self.updateUI()
            manager.useEditor("Layout")

        event.reject()

    def layoutInRow(self):
        self.setLayoutMode("row")
        self.activateTransformMode(False)

    def layoutInColumn(self):
        self.setLayoutMode("column")
        self.activateTransformMode(False)

    def layoutPacked(self):
        self.setLayoutMode("packed")
        self.activateTransformMode(False)

    def layoutInGrid(self):
        self.setLayoutMode("grid")
        self.activateTransformMode(False)

    def layoutPacked2(self):
        self.setLayoutMode("packed2")
        self.activateTransformMode(False)

    def layoutManually(self):
        self.setLayoutMode("manual")
        self.activateTransformMode(True)

    def layoutStatic(self):
        self.setLayoutMode("static")
        self.activateTransformMode(False)

    def layoutPackedEvent(self, event):
        self.layoutPacked()

    def layoutPacked2Event(self, event):
        self.layoutPacked2()

    def layoutInRowEvent(self, event):
        self.layoutInRow()

    def layoutInColumnEvent(self, event):
        self.layoutInColumn()

    def layoutInGridEvent(self, event):
        self.layoutInGrid()

    def layoutManuallyEvent(self, event):
        self.layoutManually()

    def layoutStaticEvent(self, event):
        self.layoutStatic()

    def activateTransformMode(self, on):
        _activateModeEntry("transform_manip", on)

    def activateUI(self, on):
        for mode in ["Stack_edit_mode", "Composite_edit_mode"]:
            _activateModeEntry(mode, on)

    def deactivate(self):
        rv.rvtypes.MinorMode.deactivate(self)
        self.activateUI(False)
        self.activateTransformMode(False)

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)
        self.activateUI(True)
        self.activateTransformMode(self.layoutMode() == "manual")

    def isLayoutMode(self, name):
        def F():
            if self.layoutMode() == name:
                return commands.CheckedMenuState
            return commands.UncheckedMenuState

        return F

    def menu(self):
        return [
            (
                "Layout",
                [
                    ("Layout Method", None, None, lambda: commands.DisabledMenuState),
                    menuItem(
                        "    Packed",
                        "",
                        "viewmode_category",
                        self.layoutPackedEvent,
                        self.isLayoutMode("packed"),
                    ),
                    menuItem(
                        "    Packed With Fluid Layout",
                        "",
                        "viewmode_category",
                        self.layoutPacked2Event,
                        self.isLayoutMode("packed2"),
                    ),
                    menuItem(
                        "    Row",
                        "",
                        "viewmode_category",
                        self.layoutInRowEvent,
                        self.isLayoutMode("row"),
                    ),
                    menuItem(
                        "    Column",
                        "",
                        "viewmode_category",
                        self.layoutInColumnEvent,
                        self.isLayoutMode("column"),
                    ),
                    menuItem(
                        "    Grid",
                        "",
                        "viewmode_category",
                        self.layoutInGridEvent,
                        self.isLayoutMode("grid"),
                    ),
                    menuItem(
                        "    Manual",
                        "",
                        "viewmode_category",
                        self.layoutManuallyEvent,
                        self.isLayoutMode("manual"),
                    ),
                    menuItem(
                        "    Static",
                        "",
                        "viewmode_category",
                        self.layoutStaticEvent,
                        self.isLayoutMode("static"),
                    ),
                ],
            )
        ]

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._modeCombo = None
        self._spacingSlider = None
        self._gridRowsLineEdit = None
        self._gridColumnsLineEdit = None

        self.init(
            "LayoutGroup_edit_mode",
            [
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
            ],
            None,
            self.menu(),
            "a",
        )

        self.activateTransformMode(self.layoutMode() == "manual")


def createMode():
    return LayoutGroupEditMode()
