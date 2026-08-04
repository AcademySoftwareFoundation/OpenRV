"""Gate 5 — SequenceGroup_edit_mode on the port itself."""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtWidgets
    from PySide6.QtCore import Qt

_app = None


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)
    global _app
    _app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])


class SequenceTest(unittest.TestCase):
    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("SequenceGroup_edit_mode")
        self.mode = self.mod.SequenceGroupEditMode.__new__(
            self.mod.SequenceGroupEditMode
        )
        self.mode._ui = None
        self.mode._disableUpdates = False

        self.graph.addNode("sequenceGroup", "RVSequenceGroup")
        self.graph.addNode("sequence", "RVSequence", group="sequenceGroup")
        self.graph.viewNode = "sequenceGroup"


class TestCheckBoxSlot(SequenceTest):
    PROP = "sequence.mode.autoEDL"

    def test_checked_writes_one(self):
        self.graph.seedInt(self.PROP, [0])
        self.mode.checkBoxSlot(2, self.PROP)
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1])

    def test_unchecked_writes_zero(self):
        self.graph.seedInt(self.PROP, [1])
        self.mode.checkBoxSlot(0, self.PROP)
        self.assertEqual(self.graph.getIntProperty(self.PROP), [0])

    def test_enum_argument(self):
        self.graph.seedInt(self.PROP, [0])
        self.mode.checkBoxSlot(Qt.Checked, self.PROP)
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1])

    def test_no_write_when_unchanged(self):
        self.graph.seedInt(self.PROP, [1])
        before = dict(self.graph.props)
        self.mode.checkBoxSlot(2, self.PROP)
        self.assertEqual(self.graph.props, before)

    def test_through_a_real_checkbox(self):
        self.graph.seedInt(self.PROP, [0])
        box = QtWidgets.QCheckBox()
        box.stateChanged.connect(lambda s: self.mode.checkBoxSlot(s, self.PROP))
        box.setCheckState(Qt.Checked)
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1])


class TestFpsChanged(SequenceTest):
    PROP = "sequence.output.fps"

    def setUp(self):
        super().setUp()
        self.graph.seedFloat(self.PROP, [24.0])
        self.mode._outputFPSEdit = QtWidgets.QLineEdit()

    def test_writes_a_new_rate(self):
        self.mode._outputFPSEdit.setText("48")
        self.mode.fpsChanged()
        self.assertEqual(self.graph.getFloatProperty(self.PROP), [48.0])

    def test_same_rate_does_not_redraw(self):
        self.mode._outputFPSEdit.setText("24")
        before = self.graph.redraws
        self.mode.fpsChanged()
        self.assertEqual(self.graph.redraws, before)


class TestSizeEdits(SequenceTest):
    PROP = "sequence.output.size"

    def setUp(self):
        super().setUp()
        self.graph.seedInt(self.PROP, [1920, 1080])
        self.mode._outputWidthEdit = QtWidgets.QLineEdit()
        self.mode._outputHeightEdit = QtWidgets.QLineEdit()

    def test_width_keeps_height(self):
        self.mode._outputWidthEdit.setText("1280")
        self.mode.widthChanged()
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1280, 1080])

    def test_height_keeps_width(self):
        self.mode._outputHeightEdit.setText("720")
        self.mode.heightChanged()
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1920, 720])


class TestSessionReadFreeze(SequenceTest):
    class _Event:
        def reject(self):
            pass

    def test_before_read_disables_updates(self):
        self.mode.beforeSessionRead(self._Event())
        self.assertTrue(self.mode._disableUpdates)

    def test_after_read_re_enables_updates(self):
        self.mode.beforeSessionRead(self._Event())
        self.mode.afterSessionRead(self._Event())
        self.assertFalse(self.mode._disableUpdates)

    # The real suppression test needs a loaded panel to observe anything, so it
    # lives in test_editor_update_ui.py::TestSequenceUpdateUI. A version here with
    # _ui still None could not fail: updateUI() returns early on either flag.


class TestUpdateUIWithoutPanel(SequenceTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()


if __name__ == "__main__":
    unittest.main()
