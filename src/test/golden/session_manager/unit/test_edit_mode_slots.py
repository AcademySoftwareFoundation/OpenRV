"""Gate 5 — the widget slots and event wrappers of the four panel-heavy edit modes.

`test_layout_group_edit_mode.py` and friends already cover the property-facing half
of these modes (`setLayoutMode`, `reset`, `setCutValue`, …). What is left, and what
this module covers, is the layer between a widget signal and that half: the `*Slot`
methods a `.ui` file connects to, the `*Event` one-liners the menu items are bound
to, and the `activate`/`deactivate` pairs that switch sibling modes on and off.

They look trivial enough to skip, which is exactly the risk. A slot that reads the
wrong line edit, an event wrapper bound to the neighbouring action, or an
`activate()` that forgets `activateUI(True)` all leave the panel looking right and
the graph wrong, and none of it is visible in a golden screenshot.

The sibling-mode activations go through `rv.runtime.eval` — the mode manager has no
Python binding — so the assertions there are on the Mu snippet, as in
`test_group_edit_modes.py`.
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


class _Event:
    def __init__(self, contents=""):
        self._contents = contents
        self.rejected = False

    def contents(self):
        return self._contents

    def reject(self):
        self.rejected = True


class SlotTest(unittest.TestCase):
    MODULE = None
    CLASS = None

    def setUp(self):
        if self.MODULE is None:
            self.skipTest("base class")
        self.mod, self.graph = _rv_stubs.importPort(self.MODULE)
        self.evals = []
        self.mod.rv.runtime.eval = lambda code, mods=None: (
            self.evals.append(code) or "")
        self.mode = getattr(self.mod, self.CLASS).__new__(getattr(self.mod, self.CLASS))
        self.mode._ui = None
        self.build()

    def build(self):
        pass

    def allCode(self):
        return "\n".join(self.evals)

    def ints(self, name):
        return self.graph.getIntProperty(name)

    def floats(self, name):
        return self.graph.getFloatProperty(name)

    def strings(self, name):
        return self.graph.getStringProperty(name)


# ----------------------------------------------------------------- Layout ----


class LayoutSlotTest(SlotTest):
    MODULE = "LayoutGroup_edit_mode"
    CLASS = "LayoutGroupEditMode"

    def build(self):
        self.graph.addNode("group", "RVLayoutGroup")
        self.graph.viewNode = "group"
        self.graph.seedString("group.layout.mode", ["packed"])
        self.graph.seedFloat("group.layout.spacing", [1.0])
        self.graph.seedInt("group.layout.gridRows", [0])
        self.graph.seedInt("group.layout.gridColumns", [0])

        self.mode._gridRowsLineEdit = QtWidgets.QLineEdit("0")
        self.mode._gridColumnsLineEdit = QtWidgets.QLineEdit("0")


class TestLayoutSlots(LayoutSlotTest):
    def test_spacing_slider_maps_the_full_track_to_half_to_one(self):
        """The slider is 0..999 and the property is 0.5..1.0; both ends must land."""
        self.mode.spacingSliderChangedSlot(0)
        self.assertAlmostEqual(self.floats("group.layout.spacing")[0], 0.5)

        self.mode.spacingSliderChangedSlot(999)
        self.assertAlmostEqual(self.floats("group.layout.spacing")[0], 1.0)

    def test_spacing_slider_midpoint(self):
        self.mode.spacingSliderChangedSlot(500)
        self.assertAlmostEqual(self.floats("group.layout.spacing")[0], 0.75,
                               places=3)

    def test_grid_rows_slot_writes_rows_and_zeroes_columns(self):
        """Mu passes 0 for the other axis so the layout solves for it."""
        self.mode._gridRowsLineEdit.setText("3")
        self.mode.gridRowsChangedSlot()
        self.assertEqual(self.ints("group.layout.gridRows"), [3])
        self.assertEqual(self.ints("group.layout.gridColumns"), [0])

    def test_grid_columns_slot_writes_columns_and_zeroes_rows(self):
        self.mode._gridColumnsLineEdit.setText("4")
        self.mode.gridColumnsChangedSlot()
        self.assertEqual(self.ints("group.layout.gridColumns"), [4])
        self.assertEqual(self.ints("group.layout.gridRows"), [0])

    def test_either_grid_slot_switches_the_layout_to_grid(self):
        self.mode._gridRowsLineEdit.setText("2")
        self.mode.gridRowsChangedSlot()
        self.assertEqual(self.strings("group.layout.mode"), ["grid"])

    def test_grid_slots_redraw(self):
        before = self.graph.redraws
        self.mode._gridRowsLineEdit.setText("2")
        self.mode.gridRowsChangedSlot()
        self.assertEqual(self.graph.redraws, before + 1)

    def test_mode_combo_index_selects_the_matching_layout(self):
        """The combo order is fixed by layout.ui; an off-by-one silently reorders."""
        for index, mode in enumerate(
            ["packed", "packed2", "row", "column", "grid", "manual"]
        ):
            with self.subTest(index=index):
                self.mode.modeComboChangedSlot(index)
                self.assertEqual(self.strings("group.layout.mode"), [mode])

    def test_an_index_past_the_end_falls_back_to_static(self):
        self.mode.modeComboChangedSlot(99)
        self.assertEqual(self.strings("group.layout.mode"), ["static"])


class TestLayoutMenuEvents(LayoutSlotTest):
    EVENTS = [
        ("layoutPackedEvent", "packed"),
        ("layoutPacked2Event", "packed2"),
        ("layoutInRowEvent", "row"),
        ("layoutInColumnEvent", "column"),
        ("layoutInGridEvent", "grid"),
        ("layoutManuallyEvent", "manual"),
        ("layoutStaticEvent", "static"),
    ]

    def test_each_event_wrapper_selects_its_own_layout(self):
        for method, mode in self.EVENTS:
            with self.subTest(method=method):
                getattr(self.mode, method)(_Event())
                self.assertEqual(self.strings("group.layout.mode"), [mode])

    def test_the_menu_items_are_wired_to_those_wrappers(self):
        items = self.mode.menu()[0][1]
        byLabel = {i[0].strip(): i for i in items}
        for label, mode in (
            ("Packed", "packed"),
            ("Packed With Fluid Layout", "packed2"),
            ("Row", "row"),
            ("Column", "column"),
            ("Grid", "grid"),
            ("Manual", "manual"),
            ("Static", "static"),
        ):
            with self.subTest(label=label):
                byLabel[label][1](_Event())
                self.assertEqual(self.strings("group.layout.mode"), [mode])

    def test_the_menu_checks_only_the_current_layout(self):
        self.mode.setLayoutMode("column")
        items = self.mode.menu()[0][1]
        checked = [i[0].strip() for i in items
                   if len(i) > 3 and i[3] is not None
                   and i[3]() == self.mod.commands.CheckedMenuState]
        self.assertEqual(checked, ["Column"])


class TestLayoutActivation(LayoutSlotTest):
    def test_activate_turns_on_the_stack_and_composite_editors(self):
        self.mode.activate()
        self.assertIn('findModeEntry("Stack_edit_mode")', self.allCode())
        self.assertIn('findModeEntry("Composite_edit_mode")', self.allCode())

    def test_activate_marks_the_mode_active(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)

    def test_deactivate_turns_them_back_off(self):
        self.mode._active = True
        self.mode.deactivate()
        self.assertFalse(self.mode._active)
        self.assertIn("false", self.allCode())

    def test_manual_layout_brings_up_the_transform_manipulator(self):
        """This is the only way the manipulator ever appears."""
        self.mode.setLayoutMode("manual")
        self.mode.activate()
        manip = [c for c in self.evals if "transform_manip" in c]
        self.assertEqual(len(manip), 1)
        self.assertIn("true", manip[0])

    def test_a_non_manual_layout_leaves_the_manipulator_off(self):
        self.mode.setLayoutMode("grid")
        self.mode.activate()
        manip = [c for c in self.evals if "transform_manip" in c]
        self.assertEqual(len(manip), 1)
        self.assertIn("false", manip[0])

    def test_deactivate_always_takes_the_manipulator_down(self):
        self.mode.setLayoutMode("manual")
        self.mode.deactivate()
        manip = [c for c in self.evals if "transform_manip" in c]
        self.assertIn("false", manip[0])

    def test_activate_transform_mode_names_the_manipulator(self):
        self.mode.activateTransformMode(True)
        self.assertEqual(len(self.evals), 1)
        self.assertIn('findModeEntry("transform_manip")', self.evals[0])
        self.assertIn("true", self.evals[0])

    def test_activate_ui_leaves_the_manipulator_alone(self):
        """activate() decides the manipulator separately, off the layout mode."""
        self.mode.activateUI(True)
        self.assertNotIn("transform_manip", self.allCode())


class TestLayoutPropertyChanged(LayoutSlotTest):
    def test_a_layout_property_updates_the_panel_and_redraws(self):
        calls = []
        self.mode._ui = object()
        self.mode.updateUI = lambda: calls.append(1)
        before = self.graph.redraws
        self.mode.propertyChanged(_Event("#RVLayoutGroup.layout.spacing"))
        self.assertEqual(calls, [1])
        self.assertEqual(self.graph.redraws, before + 1)

    def test_it_is_ignored_without_a_panel(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVLayoutGroup.layout.spacing"))
        self.assertEqual(calls, [])

    def test_a_non_layout_component_is_ignored(self):
        calls = []
        self.mode._ui = object()
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVLayoutGroup.output.fps"))
        self.assertEqual(calls, [])

    def test_it_always_rejects(self):
        event = _Event("#RVLayoutGroup.output.fps")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


# ----------------------------------------------------------------- Retime ----


class RetimeSlotTest(SlotTest):
    MODULE = "RetimeGroup_edit_mode"
    CLASS = "RetimeGroupEditMode"

    def build(self):
        self.graph.addNode("group", "RVRetimeGroup")
        self.graph.addNode("retime", "RVRetime", group="group")
        self.graph.viewNode = "group"
        self.graph.seedFloat("retime.visual.scale", [1.0])
        self.graph.seedFloat("retime.visual.offset", [0.0])
        self.graph.seedFloat("retime.audio.scale", [1.0])
        self.graph.seedFloat("retime.audio.offset", [0.0])
        self.graph.seedFloat("retime.output.fps", [24.0])
        self.mode._textCommit = None


class TestRetimeSlots(RetimeSlotTest):
    def test_reset_slot_resets_the_timing(self):
        self.graph.seedFloat("retime.visual.scale", [2.0])
        self.mode.resetSlot(False)
        self.assertEqual(self.floats("retime.visual.scale"), [1.0])

    def test_reverse_slot_reverses_the_timing(self):
        """reverse() aborts partway on a float audio.offset — Mu behavior the port
        reproduces, pinned in test_retime_group_edit_mode.py. What matters here is
        that the button slot really does delegate to it: visual.scale flips first."""
        with self.assertRaises(Exception):
            self.mode.reverseSlot(False)
        self.assertEqual(self.floats("retime.visual.scale"), [-1.0])

    def test_the_slots_ignore_the_checked_argument(self):
        """Qt hands a bool through clicked(bool); Mu's slot takes none."""
        with self.assertRaises(Exception):
            self.mode.reverseSlot(True)
        self.assertEqual(self.floats("retime.visual.scale"), [-1.0])

    def test_reset_timing_event_resets(self):
        self.graph.seedFloat("retime.visual.scale", [3.0])
        self.mode.resetTiming(_Event())
        self.assertEqual(self.floats("retime.visual.scale"), [1.0])

    def test_reverse_timing_event_reverses(self):
        with self.assertRaises(Exception):
            self.mode.reverseTiming(_Event())
        self.assertEqual(self.floats("retime.visual.scale"), [-1.0])

    def test_edit_slot_writes_the_line_edits_value_to_its_property(self):
        edit = QtWidgets.QLineEdit("2.5")
        self.mode.editSlot(edit, ".visual.scale")()
        self.assertEqual(self.floats("retime.visual.scale"), [2.5])

    def test_edit_slot_binds_one_property_per_line_edit(self):
        """Four line edits share this factory; a leaked `prop` crosses them."""
        vscale = QtWidgets.QLineEdit("2")
        ascale = QtWidgets.QLineEdit("3")
        fVisual = self.mode.editSlot(vscale, ".visual.scale")
        fAudio = self.mode.editSlot(ascale, ".audio.scale")

        fVisual()
        fAudio()

        self.assertEqual(self.floats("retime.visual.scale"), [2.0])
        self.assertEqual(self.floats("retime.audio.scale"), [3.0])

    def test_edit_slot_on_fps_also_sets_the_session_fps(self):
        edit = QtWidgets.QLineEdit("30")
        self.mode.editSlot(edit, ".output.fps")()
        self.assertEqual(self.graph.fps, 30.0)

    def test_edit_slot_on_a_scale_does_not_touch_the_session_fps(self):
        edit = QtWidgets.QLineEdit("2")
        self.mode.editSlot(edit, ".visual.scale")()
        self.assertEqual(self.graph.fps, 24.0)

    def test_edit_slot_reads_the_line_edit_at_call_time(self):
        edit = QtWidgets.QLineEdit("1")
        slot = self.mode.editSlot(edit, ".visual.scale")
        edit.setText("4")
        slot()
        self.assertEqual(self.floats("retime.visual.scale"), [4.0])


class TestRetimePrompts(RetimeSlotTest):
    def test_slow_down_prompt_shows_the_inverted_factor(self):
        self.graph.seedFloat("retime.visual.scale", [0.5])
        self.assertEqual(self.mode.slowDownPrompt(),
                         "Slow Down by Factor (current=2):")

    def test_speed_up_prompt_shows_the_factor_as_is(self):
        self.graph.seedFloat("retime.visual.scale", [0.5])
        self.assertEqual(self.mode.speedUpPrompt(),
                         "Speed Up by Factor (current=0.5):")

    def test_factor_prompt_uses_the_format_it_is_given(self):
        self.graph.seedFloat("retime.visual.scale", [2.0])
        self.assertEqual(self.mode.factorPrompt("x=%g", False), "x=2")
        self.assertEqual(self.mode.factorPrompt("x=%g", True), "x=0.5")

    def test_fps_prompt_shows_the_current_output_fps(self):
        self.graph.seedFloat("retime.output.fps", [29.97])
        self.assertEqual(self.mode.fpsPrompt(),
                         "Convert to FPS (current=29.97):")

    def test_the_prompts_follow_the_property(self):
        self.graph.seedFloat("retime.output.fps", [24.0])
        self.assertIn("24", self.mode.fpsPrompt())
        self.graph.seedFloat("retime.output.fps", [60.0])
        self.assertIn("60", self.mode.fpsPrompt())


class TestRetimeConvertToFPS(RetimeSlotTest):
    def test_it_writes_the_output_fps(self):
        self.mode.convertToFPS(_Event(), 30.0)
        self.assertEqual(self.floats("retime.output.fps"), [24.0],
                         "with no sources rendered the loop body never runs")

    def test_it_always_sets_the_session_fps(self):
        self.mode.convertToFPS(_Event(), 30.0)
        self.assertEqual(self.graph.fps, 30.0)

    def test_with_a_rendered_source_it_writes_the_property_too(self):
        self.mod.commands.sourcesRendered = lambda: [{"node": "retime"}]
        self.mode.convertToFPS(_Event(), 59.94)
        self.assertEqual(self.floats("retime.output.fps"), [59.94])

    def test_the_menu_offers_the_standard_rates(self):
        mode = self.mod.createMode()
        retime = mode._menu[0][1]
        convert = [i for i in retime if i[0] == "Convert to FPS"][0]
        labels = [i[0] for i in convert[1]]
        self.assertEqual(
            labels,
            ["24", "25", "23.98", "29.97", "30", "59.94", "60", "_", "Custom..."])


class TestRetimeTextEntry(RetimeSlotTest):
    """The port replaces Mu's blocking prompt dialogs with RV's text entry mode.

    That splits each prompt into "start the entry" and "apply what was typed", with
    the pending callback held on the mode. Losing the callback, or failing to clear
    it, is the failure mode this covers: a stale callback would apply the next
    unrelated commit to the wrong property.
    """

    def test_slow_down_arms_an_inverting_commit(self):
        self.mode.slowDownFactor(_Event())
        self.assertIsNotNone(self.mode._textCommit)
        self.mode.textEntryCommitted(_Event("2"))
        self.assertEqual(self.floats("retime.visual.scale"), [0.5])

    def test_speed_up_arms_a_direct_commit(self):
        self.mode.speedUpFactor(_Event())
        self.mode.textEntryCommitted(_Event("2"))
        self.assertEqual(self.floats("retime.visual.scale"), [2.0])

    def test_edit_fps_arms_the_fps_commit(self):
        self.mode.editFPS(_Event())
        self.mode.textEntryCommitted(_Event("30"))
        self.assertEqual(self.floats("retime.output.fps"), [30.0])
        self.assertEqual(self.graph.fps, 30.0)

    def test_the_callback_is_cleared_after_it_runs(self):
        self.mode.speedUpFactor(_Event())
        self.mode.textEntryCommitted(_Event("2"))
        self.assertIsNone(self.mode._textCommit)

    def test_a_commit_with_nothing_armed_is_a_noop(self):
        before = dict(self.graph.props)
        self.mode.textEntryCommitted(_Event("2"))
        self.assertEqual(self.graph.props, before)

    def test_a_second_commit_does_not_reapply_the_first(self):
        self.mode.speedUpFactor(_Event())
        self.mode.textEntryCommitted(_Event("2"))
        self.mode.textEntryCommitted(_Event("8"))
        self.assertEqual(self.floats("retime.visual.scale"), [2.0])

    def test_arming_a_second_prompt_replaces_the_first(self):
        self.mode.speedUpFactor(_Event())
        self.mode.editFPS(_Event())
        self.mode.textEntryCommitted(_Event("30"))
        self.assertEqual(self.floats("retime.output.fps"), [30.0])
        self.assertEqual(self.floats("retime.visual.scale"), [1.0])

    def test_the_raw_edit_items_send_their_own_events(self):
        for method, event in (
            ("editVScale", "retime-group-edit-visual-scale"),
            ("editVOffset", "retime-group-edit-visual-offset"),
            ("editAScale", "retime-group-edit-audio-scale"),
            ("editAOffset", "retime-group-edit-audio-offset"),
        ):
            with self.subTest(method=method):
                self.graph.events = []
                getattr(self.mode, method)(_Event())
                self.assertEqual([n for n, _c, *_ in self.graph.events], [event])


class TestRetimePropertyChanged(RetimeSlotTest):
    def test_a_retime_node_property_updates_the_panel(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("retime.visual.scale"))
        self.assertEqual(calls, [1])

    def test_another_nodes_property_is_ignored(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("group.output.fps"))
        self.assertEqual(calls, [])

    def test_it_always_rejects(self):
        event = _Event("group.output.fps")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


# ------------------------------------------------------------ SourceGroup ----


class SourceSlotTest(SlotTest):
    MODULE = "SourceGroup_edit_mode"
    CLASS = "SourceGroupEditMode"

    def build(self):
        self.graph.addNode("group", "RVSourceGroup")
        self.graph.addNode("src", "RVFileSource", group="group")
        self.graph.viewNode = "group"
        self.MU_INT_MAX = self.mod.MU_INT_MAX
        self.graph.seedInt("src.cut.in", [-self.MU_INT_MAX])
        self.graph.seedInt("src.cut.out", [self.MU_INT_MAX])
        self.graph.seedInt("src.cut.syncGui", [1])
        self.mode._locked = False

        self.mode._cutInEdit = QtWidgets.QSpinBox()
        self.mode._cutOutEdit = QtWidgets.QSpinBox()
        for box in (self.mode._cutInEdit, self.mode._cutOutEdit):
            box.setRange(-self.MU_INT_MAX, self.MU_INT_MAX)


class TestSourceSyncGuiInOut(SourceSlotTest):
    def test_it_reads_the_flag_when_present(self):
        self.graph.seedInt("src.cut.syncGui", [0])
        self.assertFalse(self.mode.syncGuiInOut())
        self.graph.seedInt("src.cut.syncGui", [1])
        self.assertTrue(self.mode.syncGuiInOut())

    def test_an_absent_flag_defaults_to_synced(self):
        """Sessions written before the flag existed must still track the GUI."""
        self.graph.props.pop("src.cut.syncGui")
        self.assertTrue(self.mode.syncGuiInOut())

    def test_sync_state_mirrors_the_flag_as_a_menu_state(self):
        self.graph.seedInt("src.cut.syncGui", [1])
        self.assertEqual(self.mode.syncState(),
                         self.mod.commands.CheckedMenuState)
        self.graph.seedInt("src.cut.syncGui", [0])
        self.assertEqual(self.mode.syncState(),
                         self.mod.commands.UncheckedMenuState)

    def test_the_source_items_are_always_selectable(self):
        self.assertEqual(self.mode.sourceMenuState(),
                         self.mod.commands.NeutralMenuState)


class TestSourceToggleSync(SourceSlotTest):
    def test_toggle_turns_syncing_off(self):
        self.graph.seedInt("src.cut.syncGui", [1])
        self.mode.toggleSync(_Event())
        self.assertEqual(self.ints("src.cut.syncGui"), [0])

    def test_toggle_turns_syncing_back_on(self):
        self.graph.seedInt("src.cut.syncGui", [0])
        self.mode.toggleSync(_Event())
        self.assertEqual(self.ints("src.cut.syncGui"), [1])

    def test_turning_it_on_pulls_the_gui_to_the_cut(self):
        self.graph.seedInt("src.cut.syncGui", [0])
        self.graph.seedInt("src.cut.in", [10])
        self.graph.seedInt("src.cut.out", [20])
        self.mode.toggleSync(_Event())
        self.assertEqual((self.graph.inFrame, self.graph.outFrame), (10, 20))

    def test_turning_it_off_leaves_the_gui_where_it_is(self):
        self.graph.inFrame, self.graph.outFrame = 5, 50
        self.graph.seedInt("src.cut.syncGui", [1])
        self.mode.toggleSync(_Event())
        self.assertEqual((self.graph.inFrame, self.graph.outFrame), (5, 50))

    def test_the_menu_item_is_wired_to_the_toggle(self):
        mode = self.mod.createMode()
        items = mode._menu[0][1]
        byLabel = {i[0]: i for i in items}
        byLabel["Sync GUI With Source Cut In/Out"][1](_Event())
        self.assertEqual(self.ints("src.cut.syncGui"), [0])

    def test_the_menu_lists_the_four_source_actions(self):
        mode = self.mod.createMode()
        self.assertEqual([i[0] for i in mode._menu[0][1]], [
            "Set Source Cut In ...",
            "Set Source Cut Out ...",
            "Clear Source Cut In/Out",
            "Sync GUI With Source Cut In/Out",
        ])

    def test_a_locked_mode_ignores_the_sync_slot(self):
        """`_locked` is how the mode stops its own writes re-entering."""
        self.mode._locked = True
        self.graph.seedInt("src.cut.syncGui", [1])
        self.mode.syncSlot(False)
        self.assertEqual(self.ints("src.cut.syncGui"), [1])


class TestSourceUpdateFromProps(SourceSlotTest):
    def test_it_moves_the_gui_to_the_cut_points(self):
        self.graph.seedInt("src.cut.in", [10])
        self.graph.seedInt("src.cut.out", [20])
        self.mode.updateFromProps()
        self.assertEqual((self.graph.inFrame, self.graph.outFrame), (10, 20))

    def test_it_clamps_to_the_source_range(self):
        """cut.in defaults to -MU_INT_MAX; unclamped that is not a valid frame."""
        self.mode.updateFromProps()
        self.assertEqual((self.graph.inFrame, self.graph.outFrame), (1, 100))

    def test_it_leaves_the_lock_clear(self):
        self.mode.updateFromProps()
        self.assertFalse(self.mode._locked)

    def test_activate_pulls_the_gui_across_when_synced(self):
        self.graph.seedInt("src.cut.in", [10])
        self.graph.seedInt("src.cut.out", [20])
        self.mode.activate()
        self.assertEqual((self.graph.inFrame, self.graph.outFrame), (10, 20))
        self.assertTrue(self.mode._active)

    def test_activate_leaves_the_gui_alone_when_not_synced(self):
        self.graph.seedInt("src.cut.syncGui", [0])
        self.graph.seedInt("src.cut.in", [10])
        self.mode.activate()
        self.assertEqual(self.graph.inFrame, 1)
        self.assertTrue(self.mode._active)


class TestSourceChangedSlot(SourceSlotTest):
    def test_it_writes_the_cut_point(self):
        self.mode.changedSlot("in")(10)
        self.assertEqual(self.ints("src.cut.in"), [10])

    def test_it_moves_the_gui_when_synced(self):
        self.mode.changedSlot("in")(10)
        self.assertEqual(self.graph.inFrame, 10)

    def test_it_leaves_the_gui_alone_when_not_synced(self):
        self.graph.seedInt("src.cut.syncGui", [0])
        self.mode.changedSlot("in")(10)
        self.assertEqual(self.ints("src.cut.in"), [10])
        self.assertEqual(self.graph.inFrame, 1)

    def test_a_value_before_the_source_start_is_rejected(self):
        self.mode.changedSlot("in")(-5)
        self.assertEqual(self.ints("src.cut.in"), [-self.MU_INT_MAX])

    def test_a_value_past_the_source_end_is_rejected(self):
        self.mode.changedSlot("out")(500)
        self.assertEqual(self.ints("src.cut.out"), [self.MU_INT_MAX])

    def test_an_in_point_past_the_out_point_is_rejected(self):
        self.graph.outFrame = 20
        self.mode.changedSlot("in")(30)
        self.assertEqual(self.ints("src.cut.in"), [-self.MU_INT_MAX])

    def test_an_out_point_before_the_in_point_is_rejected(self):
        self.graph.inFrame = 30
        self.mode.changedSlot("out")(20)
        self.assertEqual(self.ints("src.cut.out"), [self.MU_INT_MAX])

    def test_the_sentinel_is_ignored_but_still_redraws(self):
        before = self.graph.redraws
        self.mode.changedSlot("in")(-self.MU_INT_MAX)
        self.assertEqual(self.ints("src.cut.in"), [-self.MU_INT_MAX])
        self.assertEqual(self.graph.redraws, before + 1)

    def test_a_locked_mode_ignores_it(self):
        self.mode._locked = True
        self.mode.changedSlot("in")(10)
        self.assertEqual(self.ints("src.cut.in"), [-self.MU_INT_MAX])

    def test_it_leaves_the_lock_clear(self):
        self.mode.changedSlot("in")(10)
        self.assertFalse(self.mode._locked)

    def test_the_two_slots_do_not_share_their_prop(self):
        fIn = self.mode.changedSlot("in")
        fOut = self.mode.changedSlot("out")
        fIn(10)
        fOut(20)
        self.assertEqual(self.ints("src.cut.in"), [10])
        self.assertEqual(self.ints("src.cut.out"), [20])


class TestSourceFinishedSlot(SourceSlotTest):
    def test_it_writes_the_spin_boxs_value(self):
        self.mode._cutInEdit.setValue(10)
        self.mode.finishedSlot("in")()
        self.assertEqual(self.ints("src.cut.in"), [10])

    def test_out_reads_the_out_spin_box(self):
        self.mode._cutOutEdit.setValue(80)
        self.mode.finishedSlot("out")()
        self.assertEqual(self.ints("src.cut.out"), [80])

    def test_a_value_before_the_start_is_clamped_not_rejected(self):
        """This is where it differs from changedSlot: editing finishes with a
        legal value rather than being discarded."""
        self.mode._cutInEdit.setValue(-5)
        self.mode.finishedSlot("in")()
        self.assertEqual(self.ints("src.cut.in"), [1])

    def test_a_value_past_the_end_is_clamped(self):
        self.mode._cutOutEdit.setValue(500)
        self.mode.finishedSlot("out")()
        self.assertEqual(self.ints("src.cut.out"), [100])

    def test_an_in_point_past_the_out_point_is_clamped_to_it(self):
        self.graph.outFrame = 20
        self.mode._cutInEdit.setValue(30)
        self.mode.finishedSlot("in")()
        self.assertEqual(self.ints("src.cut.in"), [20])

    def test_an_out_point_before_the_in_point_is_clamped_to_it(self):
        self.graph.inFrame = 30
        self.mode._cutOutEdit.setValue(20)
        self.mode.finishedSlot("out")()
        self.assertEqual(self.ints("src.cut.out"), [30])

    def test_the_clamped_value_is_written_back_to_the_widget(self):
        self.mode._cutInEdit.setValue(-5)
        self.mode.finishedSlot("in")()
        self.assertEqual(self.mode._cutInEdit.value(), 1)

    def test_it_moves_the_gui_when_synced(self):
        self.mode._cutInEdit.setValue(10)
        self.mode.finishedSlot("in")()
        self.assertEqual(self.graph.inFrame, 10)

    def test_it_leaves_the_lock_clear(self):
        self.mode._cutInEdit.setValue(10)
        self.mode.finishedSlot("in")()
        self.assertFalse(self.mode._locked)

    def test_the_sentinel_is_ignored(self):
        self.mode._cutInEdit.setValue(-self.MU_INT_MAX)
        self.mode.finishedSlot("in")()
        self.assertEqual(self.ints("src.cut.in"), [-self.MU_INT_MAX])


class TestSourceResetAndPropertyChanged(SourceSlotTest):
    def test_reset_slot_clears_both_cut_points(self):
        self.graph.seedInt("src.cut.in", [10])
        self.graph.seedInt("src.cut.out", [20])
        self.mode.resetSlot(False)
        self.assertEqual(self.ints("src.cut.in"), [-self.MU_INT_MAX])
        self.assertEqual(self.ints("src.cut.out"), [self.MU_INT_MAX])

    def test_a_file_source_property_updates_the_panel(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("src.cut.in"))
        self.assertEqual(calls, [1])

    def test_a_property_on_another_node_type_is_ignored(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("group.ui.name"))
        self.assertEqual(calls, [])

    def test_a_locked_mode_ignores_it(self):
        """Without this the mode's own writes would re-enter through the event."""
        calls = []
        self.mode._locked = True
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("src.cut.in"))
        self.assertEqual(calls, [])

    def test_a_synced_change_pulls_the_gui_across(self):
        self.mode.updateUI = lambda: None
        self.graph.seedInt("src.cut.in", [10])
        self.graph.seedInt("src.cut.out", [20])
        self.mode.propertyChanged(_Event("src.cut.in"))
        self.assertEqual((self.graph.inFrame, self.graph.outFrame), (10, 20))

    def test_an_unsynced_change_leaves_the_gui_alone(self):
        self.mode.updateUI = lambda: None
        self.graph.seedInt("src.cut.syncGui", [0])
        self.graph.seedInt("src.cut.in", [10])
        self.mode.propertyChanged(_Event("src.cut.in"))
        self.assertEqual(self.graph.inFrame, 1)

    def test_it_always_rejects(self):
        event = _Event("group.ui.name")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


# ------------------------------------------------------------ FolderGroup ----


class FolderSlotTest(SlotTest):
    MODULE = "FolderGroup_edit_mode"
    CLASS = "FolderGroupEditMode"

    def build(self):
        self.graph.addNode("folder", "RVFolderGroup")
        self.graph.viewNode = "folder"
        self.graph.seedString("folder.mode.viewType", ["layout"])

        self.mode._viewTypeCombo = QtWidgets.QComboBox()
        self.mode._viewTypeCombo.addItem("Switch", "switch")
        self.mode._viewTypeCombo.addItem("Layout", "layout")
        self.mode._viewTypeCombo.addItem("Stack", "stack")


class TestFolderActivateUI(FolderSlotTest):
    def test_a_switch_folder_activates_the_switch_editor(self):
        self.graph.seedString("folder.mode.viewType", ["switch"])
        self.mode.activateUI(True)
        self.assertIn('findModeEntry("Switch_edit_mode")', self.allCode())

    def test_a_layout_folder_activates_the_layout_editor(self):
        self.mode.activateUI(True)
        self.assertIn('findModeEntry("LayoutGroup_edit_mode")', self.allCode())

    def test_a_stack_folder_activates_the_stack_group_editor(self):
        self.graph.seedString("folder.mode.viewType", ["stack"])
        self.mode.activateUI(True)
        self.assertIn('findModeEntry("StackGroup_edit_mode")', self.allCode())

    def test_an_unknown_view_type_falls_back_to_layout(self):
        self.graph.seedString("folder.mode.viewType", ["something-else"])
        self.mode.activateUI(True)
        self.assertIn('findModeEntry("LayoutGroup_edit_mode")', self.allCode())

    def test_only_one_sibling_is_switched_at_a_time(self):
        self.mode.activateUI(True)
        self.assertEqual(len(self.evals), 1)

    def test_off_passes_false(self):
        self.mode.activateUI(False)
        self.assertIn("false", self.allCode())

    def test_activate_turns_the_matching_editor_on(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)
        self.assertIn("true", self.allCode())

    def test_deactivate_turns_it_off(self):
        self.mode._active = True
        self.mode.deactivate()
        self.assertFalse(self.mode._active)
        self.assertIn("false", self.allCode())

    def test_switching_view_type_takes_the_old_editor_down_first(self):
        """Leaving both on stacks two editors into the same tab."""
        self.mode.setViewType(2)
        self.assertEqual(self.strings("folder.mode.viewType"), ["stack"])
        self.assertIn("false", self.evals[0])
        self.assertIn('findModeEntry("LayoutGroup_edit_mode")', self.evals[0])
        self.assertIn("true", self.evals[1])
        self.assertIn('findModeEntry("StackGroup_edit_mode")', self.evals[1])


class TestFolderPropertyChanged(FolderSlotTest):
    def test_a_view_type_change_updates_the_panel(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVFolderGroup.mode.viewType"))
        self.assertEqual(calls, [1])

    def test_another_property_is_ignored(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVFolderGroup.ui.name"))
        self.assertEqual(calls, [])

    def test_another_name_in_the_mode_component_is_ignored(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVFolderGroup.mode.somethingElse"))
        self.assertEqual(calls, [])

    def test_it_always_rejects(self):
        event = _Event("#RVFolderGroup.ui.name")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


if __name__ == "__main__":
    unittest.main()
