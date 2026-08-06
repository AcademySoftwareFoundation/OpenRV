"""Gate 5 — `loadUI()` for the eight edit modes that own a `.ui` panel.

`loadUI` is the one method in each sibling that can fail purely on a string. It loads
a Qt Designer file, pulls every widget out of it by `objectName`, connects each one to
a slot, and hands the tree to the session manager under an editor name. A misspelled
`objectName` does not raise: `findChild` returns None, the connect that follows raises
inside RV's event dispatch, and the editor tab is simply missing — which the golden
scenarios cannot see, because they capture the panel only for view types the harness
can create headlessly.

Nothing here is mocked away except the session manager itself, which is a small
recorder standing in for the mode that would normally own the tab strip. The `.ui`
files are the real ones from the package, so a widget renamed in Designer without the
matching rename in the port fails here.

The end of `loadUI` calls `updateUI()`, so these also drive each mode's panel refresh
against real widgets rather than against `_ui = None` early returns.
"""
from __future__ import annotations

import os
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


class _Event:
    def __init__(self, contents=""):
        self._contents = contents
        self.rejected = False

    def contents(self):
        return self._contents

    def reject(self):
        self.rejected = True


class _FakeManager:
    """The slice of SessionManagerMode the siblings call into."""

    def __init__(self):
        self.added = []      # (editor name, widget)
        self.used = []       # editor names
        self.reloads = 0

    def auxFilePath(self, name):
        return os.path.join(_rv_stubs.PKG_DIR, name)

    def addEditor(self, name, widget):
        self.added.append((name, widget))

    def useEditor(self, name):
        self.used.append(name)

    def reloadEditorTab(self):
        self.reloads += 1


class LoadUITest(unittest.TestCase):
    MODULE = None
    CLASS = None
    EDITOR = None
    WIDGETS = ()

    def setUp(self):
        if self.MODULE is None:
            self.skipTest("base class")
        self.mod, self.graph = _rv_stubs.importPort(self.MODULE)
        self.manager = _FakeManager()
        self.mod._sessionManagerMode = lambda: self.manager
        self.mode = self.mod.createMode()
        self.seed()

    def tearDown(self):
        if getattr(self.mode, "_ui", None) is not None:
            self.mode._ui.setParent(None)

    def seed(self):
        pass

    def load(self):
        event = _Event()
        self.mode.loadUI(event)
        return event


class LoadUIContract:
    """The assertions every panel-owning sibling has to satisfy."""

    def test_it_builds_the_panel(self):
        self.load()
        self.assertIsNotNone(self.mode._ui)

    def test_every_widget_is_found_by_object_name(self):
        """findChild returns None on a typo; the connect after it is what raises."""
        self.load()
        for name in self.WIDGETS:
            with self.subTest(widget=name):
                self.assertIsNotNone(getattr(self.mode, name),
                                     "%s not found in the .ui" % name)

    def test_it_registers_the_editor_under_the_mu_name(self):
        """The name is the tab label and the key useEditor() looks up."""
        self.load()
        self.assertEqual([n for n, _w in self.manager.added], [self.EDITOR])

    def test_it_hands_the_panel_it_built_to_the_manager(self):
        self.load()
        self.assertIs(self.manager.added[0][1], self.mode._ui)

    def test_it_selects_the_editor(self):
        self.load()
        self.assertEqual(self.manager.used, [self.EDITOR])

    def test_loading_twice_reuses_the_same_panel(self):
        """Mu guards on `_ui == nil`; without it every view change adds a tab."""
        self.load()
        first = self.mode._ui
        self.load()
        self.assertIs(self.mode._ui, first)
        self.assertEqual(len(self.manager.added), 1)

    def test_loading_twice_still_reselects_the_editor(self):
        self.load()
        self.load()
        self.assertEqual(self.manager.used, [self.EDITOR, self.EDITOR])

    def test_it_retains_the_session_window(self):
        """Dropping the wrapper takes the widgets parented to it down with it."""
        self.load()
        self.assertIsNotNone(self.mode._mainWindow)

    def test_without_a_session_manager_nothing_is_built(self):
        self.mod._sessionManagerMode = lambda: None
        self.load()
        self.assertIsNone(self.mode._ui)


class RejectingLoadUI:
    """Most loadUI handlers reject so the other handlers for the event still run."""

    def test_it_rejects_the_event(self):
        event = self.load()
        self.assertTrue(event.rejected)

    def test_it_rejects_even_without_a_session_manager(self):
        self.mod._sessionManagerMode = lambda: None
        event = self.load()
        self.assertTrue(event.rejected)


class NonRejectingLoadUI:
    """Retime and Source are the two that do not reject, in Mu and in the port.

    Six of the eight end `loadUI` with `event.reject()`; these two just fall off the
    end of the method. Every mode is bound to the same `session-manager-load-ui`
    event, so this is not cosmetic — pinning it means the difference cannot be
    "tidied up" in either direction without a test saying so.
    """

    def test_it_does_not_reject_the_event(self):
        event = self.load()
        self.assertFalse(event.rejected)


class TestCompositeLoadUI(LoadUITest, LoadUIContract, RejectingLoadUI):
    MODULE = "Composite_edit_mode"
    CLASS = "CompositeEditMode"
    EDITOR = "Composite Function"
    WIDGETS = ("_comboBox", "_dissolveLineEdit", "_dissolveLabel", "_dissolveSlider")

    def seed(self):
        self.graph.addNode("group", "RVStackGroup")
        self.graph.addNode("stack", "RVStack", group="group")
        self.graph.viewNode = "group"
        self.graph.seedString("stack.composite.type", ["over"])
        self.graph.seedFloat("stack.composite.dissolveAmount", [0.5])

    def test_the_dissolve_row_is_hidden_for_a_non_dissolve_op(self):
        self.load()
        self.assertFalse(self.mode._dissolveSlider.isVisibleTo(self.mode._ui))

    def test_the_dissolve_row_is_shown_for_dissolve(self):
        self.graph.seedString("stack.composite.type", ["dissolve"])
        self.load()
        self.assertTrue(self.mode._dissolveSlider.isVisibleTo(self.mode._ui))

    def test_the_combo_lands_on_the_current_operation(self):
        self.graph.seedString("stack.composite.type", ["replace"])
        self.load()
        self.assertEqual(self.mode._comboBox.currentIndex(),
                         self.mod._OP_NAMES.index("replace"))


class TestFolderLoadUI(LoadUITest, LoadUIContract, RejectingLoadUI):
    MODULE = "FolderGroup_edit_mode"
    CLASS = "FolderGroupEditMode"
    EDITOR = "Folder View"
    WIDGETS = ("_viewTypeCombo",)

    def seed(self):
        self.graph.addNode("folder", "RVFolderGroup")
        self.graph.viewNode = "folder"
        self.graph.seedString("folder.mode.viewType", ["layout"])
        self.mod.rv.runtime.eval = lambda code, mods=None: ""

    def test_the_combo_offers_the_three_view_types(self):
        self.load()
        combo = self.mode._viewTypeCombo
        self.assertEqual([combo.itemText(i) for i in range(combo.count())],
                         ["Switch", "Layout", "Stack"])

    def test_the_combo_items_carry_the_property_values(self):
        self.load()
        combo = self.mode._viewTypeCombo
        self.assertEqual([combo.itemData(i) for i in range(combo.count())],
                         ["switch", "layout", "stack"])

    def test_the_combo_lands_on_the_current_view_type(self):
        self.graph.seedString("folder.mode.viewType", ["stack"])
        self.load()
        self.assertEqual(self.mode._viewTypeCombo.currentIndex(), 2)

    def test_choosing_a_type_from_the_combo_writes_the_property(self):
        """The connection is the point: nothing else drives setViewType."""
        self.load()
        self.mode._viewTypeCombo.setCurrentIndex(0)
        self.assertEqual(self.graph.getStringProperty("folder.mode.viewType"),
                         ["switch"])
        self.assertEqual(self.manager.reloads, 1)


class TestLayoutLoadUI(LoadUITest, LoadUIContract, RejectingLoadUI):
    MODULE = "LayoutGroup_edit_mode"
    CLASS = "LayoutGroupEditMode"
    EDITOR = "Layout"
    WIDGETS = ("_modeCombo", "_spacingSlider", "_gridRowsLineEdit",
               "_gridColumnsLineEdit")

    def seed(self):
        self.graph.addNode("group", "RVLayoutGroup")
        self.graph.viewNode = "group"
        self.graph.seedString("group.layout.mode", ["grid"])
        self.graph.seedFloat("group.layout.spacing", [1.0])
        self.graph.seedInt("group.layout.gridRows", [3])
        self.graph.seedInt("group.layout.gridColumns", [4])
        self.mod.rv.runtime.eval = lambda code, mods=None: ""

    def test_the_grid_fields_show_the_current_grid(self):
        self.load()
        self.assertEqual(self.mode._gridRowsLineEdit.text(), "3")
        self.assertEqual(self.mode._gridColumnsLineEdit.text(), "4")

    def test_the_combo_lands_on_the_current_layout(self):
        self.load()
        self.assertEqual(self.mode._modeCombo.currentIndex(), 4)

    def test_dragging_the_spacing_slider_writes_the_property(self):
        """Mu connects sliderMoved, not valueChanged, and the distinction matters:
        updateUI() calls setValue() itself, so valueChanged would make every panel
        refresh write the spacing back and fight the property it just read."""
        self.load()
        self.mode._spacingSlider.sliderMoved.emit(0)
        self.assertAlmostEqual(
            self.graph.getFloatProperty("group.layout.spacing")[0], 0.5)

    def test_a_programmatic_set_value_does_not_write_back(self):
        self.load()
        before = self.graph.getFloatProperty("group.layout.spacing")
        self.mode._spacingSlider.setValue(0)
        self.assertEqual(self.graph.getFloatProperty("group.layout.spacing"), before)


class TestRetimeLoadUI(LoadUITest, LoadUIContract, NonRejectingLoadUI):
    MODULE = "RetimeGroup_edit_mode"
    CLASS = "RetimeGroupEditMode"
    EDITOR = "Retime"
    WIDGETS = ("_fpsEdit", "_ascaleEdit", "_vscaleEdit", "_aoffsetEdit",
               "_voffsetEdit", "_resetButton", "_reverseButton")

    def seed(self):
        self.graph.addNode("group", "RVRetimeGroup")
        self.graph.addNode("retime", "RVRetime", group="group")
        self.graph.viewNode = "group"
        self.graph.seedFloat("retime.output.fps", [24.0])
        self.graph.seedFloat("retime.visual.scale", [2.0])
        self.graph.seedFloat("retime.visual.offset", [0.0])
        self.graph.seedFloat("retime.audio.scale", [1.0])
        self.graph.seedFloat("retime.audio.offset", [0.0])

    def test_the_fields_show_the_current_timing(self):
        self.load()
        self.assertEqual(self.mode._fpsEdit.text(), "24")
        self.assertEqual(self.mode._vscaleEdit.text(), "2")

    def test_the_reset_button_resets_the_timing(self):
        self.load()
        self.mode._resetButton.click()
        self.assertEqual(self.graph.getFloatProperty("retime.visual.scale"), [1.0])


class TestSequenceLoadUI(LoadUITest, LoadUIContract, RejectingLoadUI):
    MODULE = "SequenceGroup_edit_mode"
    CLASS = "SequenceGroupEditMode"
    EDITOR = "Sequence"
    WIDGETS = ("_autoEDLCheckBox", "_useCutInfoCheckBox", "_retimeCheckBox",
               "_outputFPSEdit", "_outputWidthEdit", "_outputHeightEdit",
               "_autoSizeCheckBox", "_interactiveSizeCheckBox")

    def seed(self):
        self.graph.addNode("group", "RVSequenceGroup")
        self.graph.addNode("seq", "RVSequence", group="group")
        self.graph.viewNode = "group"
        self.graph.seedInt("seq.mode.autoEDL", [1])
        self.graph.seedInt("seq.mode.useCutInfo", [0])
        self.graph.seedInt("group.timing.retimeInputs", [1])
        self.graph.seedFloat("seq.output.fps", [24.0])
        self.graph.seedInt("seq.output.autoSize", [0])
        self.graph.seedInt("seq.output.size", [1920, 1080])
        self.graph.seedInt("seq.output.interactiveSize", [0])

    def test_the_check_boxes_show_the_current_flags(self):
        self.load()
        self.assertTrue(self.mode._autoEDLCheckBox.isChecked())
        self.assertFalse(self.mode._useCutInfoCheckBox.isChecked())
        self.assertTrue(self.mode._retimeCheckBox.isChecked())

    def test_the_size_fields_show_the_output_size(self):
        self.load()
        self.assertEqual(self.mode._outputWidthEdit.text(), "1920")
        self.assertEqual(self.mode._outputHeightEdit.text(), "1080")

    def test_auto_size_disables_the_size_fields(self):
        self.graph.seedInt("seq.output.autoSize", [1])
        self.load()
        self.assertFalse(self.mode._outputWidthEdit.isEnabled())

    def test_ticking_a_box_writes_its_property(self):
        self.load()
        self.mode._useCutInfoCheckBox.setChecked(True)
        self.assertEqual(self.graph.getIntProperty("seq.mode.useCutInfo"), [1])

    def test_load_ui_clears_the_update_freeze(self):
        """A session read that never completed would otherwise leave it frozen."""
        self.mode._disableUpdates = True
        self.load()
        self.assertFalse(self.mode._disableUpdates)

    def test_activate_ui_is_what_load_ui_delegates_to(self):
        self.mode.activateUI()
        self.assertEqual(self.manager.used, [self.EDITOR])

    def test_activate_builds_the_panel_too(self):
        self.mode.activate()
        self.assertTrue(self.mode._active)
        self.assertEqual([n for n, _w in self.manager.added], [self.EDITOR])


class TestSourceLoadUI(LoadUITest, LoadUIContract, NonRejectingLoadUI):
    MODULE = "SourceGroup_edit_mode"
    CLASS = "SourceGroupEditMode"
    EDITOR = "Source"
    WIDGETS = ("_cutInEdit", "_cutOutEdit", "_resetButton", "_syncCheckBox")

    def seed(self):
        self.graph.addNode("group", "RVSourceGroup")
        self.graph.addNode("src", "RVFileSource", group="group")
        self.graph.viewNode = "group"
        self.graph.seedInt("src.cut.in", [10])
        self.graph.seedInt("src.cut.out", [20])
        self.graph.seedInt("src.cut.syncGui", [1])

    def test_the_spin_boxes_show_the_cut_points(self):
        self.load()
        self.assertEqual(self.mode._cutInEdit.value(), 10)
        self.assertEqual(self.mode._cutOutEdit.value(), 20)

    def test_the_sync_box_shows_the_flag(self):
        self.load()
        self.assertTrue(self.mode._syncCheckBox.isChecked())

    def test_the_reset_button_clears_the_cut(self):
        self.load()
        self.mode._resetButton.click()
        self.assertEqual(self.graph.getIntProperty("src.cut.in"),
                         [-self.mod.MU_INT_MAX])

    def test_loading_leaves_the_lock_clear(self):
        self.load()
        self.assertFalse(self.mode._locked)


class TestStackLoadUI(LoadUITest, LoadUIContract, RejectingLoadUI):
    MODULE = "Stack_edit_mode"
    CLASS = "StackEditMode"
    EDITOR = "Stack"
    WIDGETS = ("_alignCheckBox", "_strictRangesCheckBox", "_useCutInfoCheckBox",
               "_retimeCheckBox", "_autoSizeCheckBox", "_chosenAudioInputCombo",
               "_outputFPSEdit", "_outputWidthEdit", "_outputHeightEdit",
               "_interactiveSizeCheckBox")

    def seed(self):
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("group", "RVStackGroup", inputs=["srcA"])
        self.graph.addNode("stack", "RVStack", group="group")
        self.graph.viewNode = "group"
        self.graph.uiNames["srcA"] = "Source A"
        self.graph.seedInt("stack.mode.alignStartFrames", [1])
        self.graph.seedInt("stack.mode.strictFrameRanges", [0])
        self.graph.seedInt("stack.mode.useCutInfo", [0])
        self.graph.seedString("stack.output.chosenAudioInput", [".all."])
        self.graph.seedInt("stack.output.autoSize", [0])
        self.graph.seedInt("stack.output.size", [1920, 1080])
        self.graph.seedFloat("stack.output.fps", [24.0])
        self.graph.seedInt("stack.output.interactiveSize", [0])
        self.graph.seedInt("group.timing.retimeInputs", [1])

    def test_the_check_boxes_show_the_current_flags(self):
        self.load()
        self.assertTrue(self.mode._alignCheckBox.isChecked())
        self.assertFalse(self.mode._strictRangesCheckBox.isChecked())
        self.assertTrue(self.mode._retimeCheckBox.isChecked())

    def test_the_audio_combo_opens_with_the_three_mixes_then_the_inputs(self):
        self.load()
        combo = self.mode._chosenAudioInputCombo
        self.assertEqual([combo.itemData(i) for i in range(combo.count())],
                         [".all.", ".first.", ".topmost.", "srcA"])

    def test_the_audio_combo_lands_on_the_chosen_input(self):
        self.graph.seedString("stack.output.chosenAudioInput", ["srcA"])
        self.load()
        self.assertEqual(self.mode._chosenAudioInputCombo.currentIndex(), 3)

    def test_the_ui_flux_flag_is_clear_afterwards(self):
        """Left set, every later combo change would be swallowed."""
        self.load()
        self.assertFalse(self.mode._uiInFlux)

    def test_ticking_a_box_writes_its_property(self):
        self.load()
        self.mode._useCutInfoCheckBox.setChecked(True)
        self.assertEqual(self.graph.getIntProperty("stack.mode.useCutInfo"), [1])


class TestSwitchLoadUI(LoadUITest, LoadUIContract, RejectingLoadUI):
    MODULE = "Switch_edit_mode"
    CLASS = "SwitchEditMode"
    EDITOR = "Switch"
    WIDGETS = ("_alignCheckBox", "_useCutInfoCheckBox", "_autoSizeCheckBox",
               "_selectedInputCombo", "_outputWidthEdit", "_outputHeightEdit")

    def seed(self):
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.addNode("group", "RVSwitchGroup", inputs=["srcA", "srcB"])
        self.graph.addNode("switch", "RVSwitch", group="group")
        self.graph.viewNode = "group"
        self.graph.uiNames.update({"srcA": "Source A", "srcB": "Source B"})
        self.graph.seedInt("switch.mode.alignStartFrames", [1])
        self.graph.seedInt("switch.mode.useCutInfo", [0])
        self.graph.seedString("switch.output.input", ["srcB"])
        self.graph.seedInt("switch.output.autoSize", [0])
        self.graph.seedInt("switch.output.size", [1920, 1080])

    def test_the_input_combo_lists_the_switch_inputs(self):
        self.load()
        combo = self.mode._selectedInputCombo
        self.assertEqual([combo.itemData(i) for i in range(combo.count())],
                         ["srcA", "srcB"])

    def test_the_input_combo_lands_on_the_selected_input(self):
        self.load()
        self.assertEqual(self.mode._selectedInputCombo.currentIndex(), 1)

    def test_the_combo_shows_ui_names_not_node_names(self):
        self.load()
        combo = self.mode._selectedInputCombo
        self.assertEqual([combo.itemText(i) for i in range(combo.count())],
                         ["Source A", "Source B"])

    def test_the_ui_flux_flag_is_clear_afterwards(self):
        self.load()
        self.assertFalse(self.mode._uiInFlux)


if __name__ == "__main__":
    unittest.main()
