"""Gate 5 — the sibling editors' updateUI(), driving real widgets from the graph.

updateUI() is the read direction of every editor panel: it takes the node's
properties and pushes them into the widgets. The golden scenarios never see it,
because they compare the session graph and the session-manager tree, not the editor
tab's contents — so a panel that silently shows stale or wrong values would pass
every gate.

Each test builds the real widgets the port expects, points the mode at them, and
checks what updateUI() put there. `_uiInFlux` matters throughout: updateUI()
repopulates combos and checkboxes, which fires the very signals the write direction
listens to, and the flag is what stops that feeding back into the graph.
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


class EditorTest(unittest.TestCase):
    MODULE = None
    CLASS = None

    def setUp(self):
        if self.MODULE is None:
            self.skipTest("base class")
        self.mod, self.graph = _rv_stubs.importPort(self.MODULE)
        self.mode = getattr(self.mod, self.CLASS).__new__(getattr(self.mod, self.CLASS))
        self.mode._uiInFlux = False
        self.mode._disableUpdates = False
        self.mode._ui = object()          # non-None: "the panel is loaded"

    def widgets(self, **kw):
        for name, w in kw.items():
            setattr(self.mode, name, w)


class TestStackUpdateUI(EditorTest):
    MODULE = "Stack_edit_mode"
    CLASS = "StackEditMode"

    def setUp(self):
        super().setUp()
        self.graph.addNode("stackGroup", "RVStackGroup")
        self.graph.addNode("stack", "RVStack", group="stackGroup")
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.connections["stackGroup"] = ["srcA", "srcB"]
        self.graph.viewNode = "stackGroup"
        self.graph.uiNames.update({"srcA": "Source A", "srcB": "Source B"})

        for name, value in (
            ("stack.mode.alignStartFrames", 1),
            ("stack.mode.strictFrameRanges", 0),
            ("stack.mode.useCutInfo", 1),
            ("stack.output.autoSize", 0),
            ("stack.output.interactiveSize", 0),
        ):
            self.graph.seedInt(name, [value])
        self.graph.seedInt("stack.output.size", [1280, 720])
        self.graph.seedFloat("stack.output.fps", [24.0])
        self.graph.seedString("stack.output.chosenAudioInput", [".first."])

        self.widgets(
            _alignCheckBox=QtWidgets.QCheckBox(),
            _strictRangesCheckBox=QtWidgets.QCheckBox(),
            _useCutInfoCheckBox=QtWidgets.QCheckBox(),
            _autoSizeCheckBox=QtWidgets.QCheckBox(),
            _interactiveSizeCheckBox=QtWidgets.QCheckBox(),
            _retimeCheckBox=QtWidgets.QCheckBox(),
            _chosenAudioInputCombo=QtWidgets.QComboBox(),
            _outputFPSEdit=QtWidgets.QLineEdit(),
            _outputWidthEdit=QtWidgets.QLineEdit(),
            _outputHeightEdit=QtWidgets.QLineEdit(),
        )

    def test_checkboxes_follow_the_properties(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._alignCheckBox.checkState(), Qt.Checked)
        self.assertEqual(self.mode._strictRangesCheckBox.checkState(), Qt.Unchecked)
        self.assertEqual(self.mode._useCutInfoCheckBox.checkState(), Qt.Checked)

    def test_size_and_fps_are_formatted(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._outputWidthEdit.text(), "1280")
        self.assertEqual(self.mode._outputHeightEdit.text(), "720")
        self.assertEqual(self.mode._outputFPSEdit.text(), "24")

    def test_size_edits_disabled_when_auto_size_is_on(self):
        self.graph.seedInt("stack.output.autoSize", [1])
        self.mode.updateUI()
        self.assertFalse(self.mode._outputWidthEdit.isEnabled())
        self.assertFalse(self.mode._outputHeightEdit.isEnabled())

    def test_size_edits_enabled_when_auto_size_is_off(self):
        self.mode.updateUI()
        self.assertTrue(self.mode._outputWidthEdit.isEnabled())

    def test_audio_combo_has_the_three_fixed_entries_first(self):
        self.mode.updateUI()
        combo = self.mode._chosenAudioInputCombo
        self.assertEqual(combo.itemData(0), ".all.")
        self.assertEqual(combo.itemData(1), ".first.")
        self.assertEqual(combo.itemData(2), ".topmost.")

    def test_audio_combo_lists_the_inputs_by_ui_name(self):
        self.mode.updateUI()
        combo = self.mode._chosenAudioInputCombo
        self.assertEqual(combo.itemText(3), "Source A")
        self.assertEqual(combo.itemData(3), "srcA")
        self.assertEqual(combo.itemText(4), "Source B")

    def test_current_audio_selection_is_restored(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._chosenAudioInputCombo.currentIndex(), 1)

    def test_a_node_audio_selection_is_restored_at_its_offset(self):
        self.graph.seedString("stack.output.chosenAudioInput", ["srcB"])
        self.mode.updateUI()
        self.assertEqual(self.mode._chosenAudioInputCombo.currentIndex(), 4)

    def test_ui_in_flux_is_cleared_afterwards(self):
        self.mode.updateUI()
        self.assertFalse(self.mode._uiInFlux,
                         "leaving it set would deafen the panel to real user edits")

    def test_repopulating_does_not_write_back_to_the_graph(self):
        before = dict(self.graph.props)
        self.mode.updateUI()
        self.assertEqual(self.graph.props, before)


class TestSwitchUpdateUI(EditorTest):
    MODULE = "Switch_edit_mode"
    CLASS = "SwitchEditMode"

    def setUp(self):
        super().setUp()
        self.graph.addNode("switchGroup", "RVSwitchGroup")
        self.graph.addNode("switch", "RVSwitch", group="switchGroup")
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.connections["switchGroup"] = ["srcA", "srcB"]
        self.graph.viewNode = "switchGroup"
        self.graph.uiNames.update({"srcA": "Source A", "srcB": "Source B"})

        for name, value in (
            ("switch.mode.alignStartFrames", 0),
            ("switch.mode.useCutInfo", 1),
            ("switch.output.autoSize", 1),
        ):
            self.graph.seedInt(name, [value])
        self.graph.seedInt("switch.output.size", [640, 480])
        self.graph.seedString("switch.output.input", ["srcB"])

        self.widgets(
            _alignCheckBox=QtWidgets.QCheckBox(),
            _useCutInfoCheckBox=QtWidgets.QCheckBox(),
            _autoSizeCheckBox=QtWidgets.QCheckBox(),
            _selectedInputCombo=QtWidgets.QComboBox(),
            _outputWidthEdit=QtWidgets.QLineEdit(),
            _outputHeightEdit=QtWidgets.QLineEdit(),
        )

    def test_checkboxes_follow_the_properties(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._alignCheckBox.checkState(), Qt.Unchecked)
        self.assertEqual(self.mode._useCutInfoCheckBox.checkState(), Qt.Checked)
        self.assertEqual(self.mode._autoSizeCheckBox.checkState(), Qt.Checked)

    def test_input_combo_lists_only_the_inputs(self):
        """Unlike Stack, Switch has no fixed entries — index 0 is the first input."""
        self.mode.updateUI()
        combo = self.mode._selectedInputCombo
        self.assertEqual(combo.count(), 2)
        self.assertEqual(combo.itemData(0), "srcA")
        self.assertEqual(combo.itemText(0), "Source A")

    def test_selected_input_is_restored(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._selectedInputCombo.currentIndex(), 1)

    def test_size_is_formatted_and_disabled_under_auto_size(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._outputWidthEdit.text(), "640")
        self.assertEqual(self.mode._outputHeightEdit.text(), "480")
        self.assertFalse(self.mode._outputWidthEdit.isEnabled())

    def test_no_view_node_is_a_noop(self):
        self.graph.viewNode = None
        self.mode._selectedInputCombo.addItem("stale", "stale")
        self.mode.updateUI()
        self.assertEqual(self.mode._selectedInputCombo.count(), 1)


class TestSequenceUpdateUI(EditorTest):
    MODULE = "SequenceGroup_edit_mode"
    CLASS = "SequenceGroupEditMode"

    def setUp(self):
        super().setUp()
        self.graph.addNode("sequenceGroup", "RVSequenceGroup")
        self.graph.addNode("sequence", "RVSequence", group="sequenceGroup")
        self.graph.viewNode = "sequenceGroup"

        for name, value in (
            ("sequence.mode.autoEDL", 1),
            ("sequence.mode.useCutInfo", 0),
            ("sequenceGroup.timing.retimeInputs", 1),
            ("sequence.output.autoSize", 0),
            ("sequence.output.interactiveSize", 0),
        ):
            self.graph.seedInt(name, [value])
        self.graph.seedInt("sequence.output.size", [1920, 1080])
        self.graph.seedFloat("sequence.output.fps", [23.98])

        self.widgets(
            _autoEDLCheckBox=QtWidgets.QCheckBox(),
            _useCutInfoCheckBox=QtWidgets.QCheckBox(),
            _retimeCheckBox=QtWidgets.QCheckBox(),
            _autoSizeCheckBox=QtWidgets.QCheckBox(),
            _interactiveSizeCheckBox=QtWidgets.QCheckBox(),
            _outputFPSEdit=QtWidgets.QLineEdit(),
            _outputWidthEdit=QtWidgets.QLineEdit(),
            _outputHeightEdit=QtWidgets.QLineEdit(),
        )

    def test_checkboxes_follow_the_properties(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._autoEDLCheckBox.checkState(), Qt.Checked)
        self.assertEqual(self.mode._useCutInfoCheckBox.checkState(), Qt.Unchecked)
        self.assertEqual(self.mode._retimeCheckBox.checkState(), Qt.Checked)

    def test_non_integer_fps_is_not_rounded(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._outputFPSEdit.text(), "23.98")

    def test_size_is_formatted(self):
        self.mode.updateUI()
        self.assertEqual(self.mode._outputWidthEdit.text(), "1920")
        self.assertEqual(self.mode._outputHeightEdit.text(), "1080")

    def test_frozen_updates_are_skipped(self):
        """A session read fires many property changes; the panel must not rebuild."""
        self.mode._disableUpdates = True
        self.mode._outputFPSEdit.setText("sentinel")
        self.mode.updateUI()
        self.assertEqual(self.mode._outputFPSEdit.text(), "sentinel")

    def test_missing_properties_bail_out_quietly(self):
        self.graph.deleteProperty("sequence.mode.autoEDL")
        self.mode._outputFPSEdit.setText("sentinel")
        self.mode.updateUI()
        self.assertEqual(self.mode._outputFPSEdit.text(), "sentinel")


if __name__ == "__main__":
    unittest.main()
