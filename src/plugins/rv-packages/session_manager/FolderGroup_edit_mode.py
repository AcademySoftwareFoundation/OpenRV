#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Folder group edit mode — Python port of FolderGroup_edit_mode.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import rv.commands as commands
import rv.qtutils as qtutils
import rv.rvtypes
import rv.runtime

from PySide6 import QtCore, QtWidgets
from PySide6.QtCore import Qt
from PySide6.QtUiTools import QUiLoader

from session_manager import setStringProp


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


class FolderGroupEditMode(rv.rvtypes.MinorMode):
    def activateUI(self, on):
        currentType = commands.getStringProperty("#RVFolderGroup.mode.viewType")[0]

        if currentType == "switch":
            modes = ["Switch_edit_mode"]
        elif currentType == "layout":
            modes = ["LayoutGroup_edit_mode"]
        elif currentType == "stack":
            modes = ["StackGroup_edit_mode"]
        else:
            modes = ["LayoutGroup_edit_mode"]

        for mode in modes:
            _activateModeEntry(mode, on)

    def setViewType(self, index):
        currentType = commands.getStringProperty("#RVFolderGroup.mode.viewType")[0]
        newtype = str(self._viewTypeCombo.itemData(index, Qt.UserRole))

        if newtype != currentType:
            self.activateUI(False)
            setStringProp("#RVFolderGroup.mode.viewType", newtype)
            commands.redraw()
            self.activateUI(True)

            manager = _sessionManagerMode()

            if manager is not None:
                manager.reloadEditorTab()

    def updateUI(self):
        vnode = commands.viewNode()
        vnodeExists = vnode is not None

        if self._ui is None or not vnodeExists:
            return

        try:
            vtype = commands.getStringProperty("#RVFolderGroup.mode.viewType")[0]

            if vtype == "switch":
                index = 0
            elif vtype == "layout":
                index = 1
            elif vtype == "stack":
                index = 2
            else:
                index = 1

            self._viewTypeCombo.setCurrentIndex(index)
        except Exception:
            pass

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
                self._ui = _loadUIFile(manager.auxFilePath("folder.ui"), m)
                self._viewTypeCombo = self._ui.findChild(
                    QtWidgets.QComboBox, "viewTypeCombo"
                )

                self._viewTypeCombo.clear()
                self._viewTypeCombo.addItem("Switch", "switch")
                self._viewTypeCombo.addItem("Layout", "layout")
                self._viewTypeCombo.addItem("Stack", "stack")

                self._viewTypeCombo.currentIndexChanged.connect(self.setViewType)
                manager.addEditor("Folder View", self._ui)

            self.updateUI()
            manager.useEditor("Folder View")

        event.reject()

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

        if comp == "mode" and name == "viewType":
            self.updateUI()

        event.reject()

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._ui = None
        self._mainWindow = None
        self._viewTypeCombo = None

        self.init(
            "FolderGroup_edit_mode",
            None,
            [
                (
                    "session-manager-load-ui",
                    self.loadUI,
                    "Load UI into Session Manager",
                ),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
            ],
            None,
            None,
        )


def createMode():
    return FolderGroupEditMode()
