"""Gate 5 — Stack_edit_mode on the port itself.

checkBoxSlot is the method the Qt.CheckState enum defect lived in: PySide6 delivers
stateChanged as a plain int, so the direct ``state == Qt.Checked`` comparison Mu uses
was always False and every toggle wrote 0. Those writes are what the behavioral gate
saw as retimeInputs/useCutInfo/alignStartFrames silently reverting, so both polarities
are pinned here as well as at the scenario level.
"""
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


class StackTest(unittest.TestCase):
    NODE = "stack"

    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("Stack_edit_mode")
        self.mode = self.mod.StackEditMode.__new__(self.mod.StackEditMode)
        self.mode._ui = None
        self.mode._uiInFlux = False

        self.graph.addNode("stackGroup", "RVStackGroup")
        self.graph.addNode(self.NODE, "RVStack", group="stackGroup")
        self.graph.viewNode = "stackGroup"

    def ints(self, name):
        return self.graph.getIntProperty(name)


class TestCheckBoxSlot(StackTest):
    PROP = "stack.mode.alignStartFrames"

    def test_checked_writes_one(self):
        self.graph.seedInt(self.PROP, [0])
        self.mode.checkBoxSlot(2, self.PROP)
        self.assertEqual(self.ints(self.PROP), [1])

    def test_unchecked_writes_zero(self):
        self.graph.seedInt(self.PROP, [1])
        self.mode.checkBoxSlot(0, self.PROP)
        self.assertEqual(self.ints(self.PROP), [0])

    def test_checked_accepts_the_enum_too(self):
        self.graph.seedInt(self.PROP, [0])
        self.mode.checkBoxSlot(Qt.Checked, self.PROP)
        self.assertEqual(self.ints(self.PROP), [1])

    def test_no_write_when_already_in_that_state(self):
        self.graph.seedInt(self.PROP, [1])
        before = dict(self.graph.props)
        self.mode.checkBoxSlot(2, self.PROP)
        self.assertEqual(self.graph.props, before)

    def test_a_real_checkbox_signal_reaches_the_property(self):
        """End to end through Qt, which is where the enum mismatch actually bit."""
        self.graph.seedInt(self.PROP, [0])
        box = QtWidgets.QCheckBox()
        box.stateChanged.connect(
            lambda s: self.mode.checkBoxSlot(s, self.PROP)
        )
        box.setCheckState(Qt.Checked)
        self.assertEqual(self.ints(self.PROP), [1])

    def test_a_real_checkbox_can_clear_it_again(self):
        self.graph.seedInt(self.PROP, [1])
        box = QtWidgets.QCheckBox()
        box.setCheckState(Qt.Checked)
        box.stateChanged.connect(
            lambda s: self.mode.checkBoxSlot(s, self.PROP)
        )
        box.setCheckState(Qt.Unchecked)
        self.assertEqual(self.ints(self.PROP), [0])


class TestSetChosenAudioInput(StackTest):
    PROP = "stack.output.chosenAudioInput"

    def setUp(self):
        super().setUp()
        self.graph.seedString(self.PROP, [".all."])
        self.combo = QtWidgets.QComboBox()
        for label, data in (
            ("All Inputs Mixed", ".all."),
            ("First Input Only", ".first."),
            ("First Visible Input", ".topmost."),
            ("SourceA", "sourceGroupA"),
        ):
            self.combo.addItem(label, data)
        self.mode._chosenAudioInputCombo = self.combo

    def strings(self):
        return self.graph.getStringProperty(self.PROP)

    def test_selecting_first_only(self):
        self.mode.setChosenAudioInput(1)
        self.assertEqual(self.strings(), [".first."])

    def test_selecting_topmost(self):
        self.mode.setChosenAudioInput(2)
        self.assertEqual(self.strings(), [".topmost."])

    def test_node_entries_start_at_index_three(self):
        self.mode.setChosenAudioInput(3)
        self.assertEqual(self.strings(), ["sourceGroupA"])

    def test_out_of_range_falls_back_to_all(self):
        self.graph.seedString(self.PROP, [".first."])
        self.mode.setChosenAudioInput(99)
        self.assertEqual(self.strings(), [".all."])

    def test_reselecting_the_current_value_writes_nothing(self):
        before = self.graph.redraws
        self.mode.setChosenAudioInput(0)
        self.assertEqual(self.graph.redraws, before)

    def test_ui_in_flux_suppresses_the_write(self):
        """updateUI() repopulates the combo; those signals must not write back."""
        self.mode._uiInFlux = True
        self.mode.setChosenAudioInput(1)
        self.assertEqual(self.strings(), [".all."])


class TestSizeEdits(StackTest):
    PROP = "stack.output.size"

    def setUp(self):
        super().setUp()
        self.graph.seedInt(self.PROP, [1920, 1080])
        self.mode._outputWidthEdit = QtWidgets.QLineEdit()
        self.mode._outputHeightEdit = QtWidgets.QLineEdit()

    def test_width_change_keeps_height(self):
        self.mode._outputWidthEdit.setText("1280")
        self.mode.widthChanged()
        self.assertEqual(self.ints(self.PROP), [1280, 1080])

    def test_height_change_keeps_width(self):
        self.mode._outputHeightEdit.setText("720")
        self.mode.heightChanged()
        self.assertEqual(self.ints(self.PROP), [1920, 720])

    def test_the_two_edits_compose(self):
        self.mode._outputWidthEdit.setText("1280")
        self.mode.widthChanged()
        self.mode._outputHeightEdit.setText("720")
        self.mode.heightChanged()
        self.assertEqual(self.ints(self.PROP), [1280, 720])

    def test_a_float_string_is_truncated(self):
        self.mode._outputWidthEdit.setText("1280.7")
        self.mode.widthChanged()
        self.assertEqual(self.ints(self.PROP), [1280, 1080])


class TestFpsChanged(StackTest):
    PROP = "stack.output.fps"

    def test_writes_the_new_rate(self):
        self.graph.seedFloat(self.PROP, [24.0])
        self.mode._outputFPSEdit = QtWidgets.QLineEdit("48")
        self.mode.fpsChanged()
        self.assertEqual(self.graph.getFloatProperty(self.PROP), [48.0])

    def test_redraws_even_when_the_write_fails(self):
        """fpsChanged swallows the property error but must still redraw."""
        self.graph.seedInt(self.PROP, [24])   # wrong type on purpose
        self.mode._outputFPSEdit = QtWidgets.QLineEdit("48")
        before = self.graph.redraws
        self.mode.fpsChanged()
        self.assertEqual(self.graph.redraws, before + 1)


class TestUpdateUIWithoutPanel(StackTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
