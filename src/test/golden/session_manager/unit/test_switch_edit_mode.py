"""Gate 5 — Switch_edit_mode on the port itself.

Switch's checkBoxSlot writes unconditionally (unlike Stack's, which compares first),
so the enum handling is pinned separately here rather than assumed to match.
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


class SwitchTest(unittest.TestCase):
    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("Switch_edit_mode")
        self.mode = self.mod.SwitchEditMode.__new__(self.mod.SwitchEditMode)
        self.mode._ui = None
        self.mode._uiInFlux = False

        self.graph.addNode("switchGroup", "RVSwitchGroup")
        self.graph.addNode("switch", "RVSwitch", group="switchGroup")
        self.graph.viewNode = "switchGroup"


class TestCheckBoxSlot(SwitchTest):
    PROP = "switch.mode.alignStartFrames"

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

    def test_writes_even_when_unchanged(self):
        """Unlike Stack, Switch has no equality guard; the write always happens."""
        self.graph.seedInt(self.PROP, [1])
        self.mode.checkBoxSlot(2, self.PROP)
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1])

    def test_creates_the_property_when_missing(self):
        self.mode.checkBoxSlot(2, "switch.mode.useCutInfo")
        self.assertEqual(self.graph.getIntProperty("switch.mode.useCutInfo"), [1])

    def test_through_a_real_checkbox(self):
        self.graph.seedInt(self.PROP, [0])
        box = QtWidgets.QCheckBox()
        box.stateChanged.connect(lambda s: self.mode.checkBoxSlot(s, self.PROP))
        box.setCheckState(Qt.Checked)
        self.assertEqual(self.graph.getIntProperty(self.PROP), [1])


class TestSetSelectedInput(SwitchTest):
    PROP = "switch.output.input"

    def setUp(self):
        super().setUp()
        self.graph.seedString(self.PROP, [""])
        self.combo = QtWidgets.QComboBox()
        self.combo.addItem("SourceA", "sourceGroupA")
        self.combo.addItem("SourceB", "sourceGroupB")
        self.mode._selectedInputCombo = self.combo

    def test_selects_the_named_input(self):
        self.mode.setSelectedInput(1)
        self.assertEqual(self.graph.getStringProperty(self.PROP), ["sourceGroupB"])

    def test_out_of_range_writes_the_empty_name(self):
        self.graph.seedString(self.PROP, ["sourceGroupA"])
        self.mode.setSelectedInput(99)
        self.assertEqual(self.graph.getStringProperty(self.PROP), [""])

    def test_reselecting_the_current_value_does_not_redraw(self):
        self.mode.setSelectedInput(0)
        before = self.graph.redraws
        self.mode.setSelectedInput(0)
        self.assertEqual(self.graph.redraws, before)

    def test_ui_in_flux_suppresses_the_write(self):
        self.mode._uiInFlux = True
        self.mode.setSelectedInput(1)
        self.assertEqual(self.graph.getStringProperty(self.PROP), [""])


class TestUpdateUIWithoutPanel(SwitchTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
