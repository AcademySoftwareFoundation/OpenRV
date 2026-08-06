"""Gate 5 — FolderGroup_edit_mode on the port itself.

setViewType is the folder's switch/layout/stack selector. It writes the property only
on an actual change, because each change tears the editor panel down and rebuilds it.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtWidgets

_app = None


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)
    global _app
    _app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])


class FolderTest(unittest.TestCase):
    PROP = "folderGroup.mode.viewType"

    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("FolderGroup_edit_mode")
        self.mode = self.mod.FolderGroupEditMode.__new__(self.mod.FolderGroupEditMode)
        self.mode._ui = None

        self.graph.addNode("folderGroup", "RVFolderGroup")
        self.graph.viewNode = "folderGroup"
        self.graph.seedString(self.PROP, ["switch"])

        self.combo = QtWidgets.QComboBox()
        self.combo.addItem("Switch", "switch")
        self.combo.addItem("Layout", "layout")
        self.combo.addItem("Stack", "stack")
        self.mode._viewTypeCombo = self.combo

        # activateUI toggles sibling modes through the mode manager, which needs a
        # live RV; the property write is what this method is being tested for.
        self.activations = []
        self.mode.activateUI = lambda on: self.activations.append(on)

    def viewType(self):
        return self.graph.getStringProperty(self.PROP)[0]


class TestSetViewType(FolderTest):
    def test_switching_to_layout(self):
        self.mode.setViewType(1)
        self.assertEqual(self.viewType(), "layout")

    def test_switching_to_stack(self):
        self.mode.setViewType(2)
        self.assertEqual(self.viewType(), "stack")

    def test_selecting_the_current_type_writes_nothing(self):
        before = self.graph.redraws
        self.mode.setViewType(0)
        self.assertEqual(self.viewType(), "switch")
        self.assertEqual(self.graph.redraws, before)

    def test_a_change_cycles_the_sibling_modes_off_and_on(self):
        """The old editor has to be torn down before the new one loads."""
        self.mode.setViewType(1)
        self.assertEqual(self.activations, [False, True])

    def test_no_cycling_when_nothing_changed(self):
        self.mode.setViewType(0)
        self.assertEqual(self.activations, [])

    def test_redraws_on_change(self):
        before = self.graph.redraws
        self.mode.setViewType(2)
        self.assertEqual(self.graph.redraws, before + 1)


class TestUpdateUIWithoutPanel(FolderTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
