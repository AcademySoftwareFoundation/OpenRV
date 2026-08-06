"""Gate 5 — the per-view menus the sibling edit modes contribute.

Each of these modes adds one submenu to the menu bar while its view type is current,
and every item in it is a `(label, func, key, stateFunc)` tuple built by the
`menuItem()` shim. Two things can go wrong silently: the item can be wired to the
wrong toggle, and its state function can report the wrong check mark. Neither shows
up in a golden — the panel screenshot does not include the menu bar, and RV's menus
cannot be opened headlessly (`QMenu.exec` blocks), which is why COVERAGE.md lists the
context menu under headless limitations. So the menu tables are walked here directly.

The toggles themselves are the other half: `alignStartFrames`, `useCutInfo`,
`strictFrameRanges`, `autoRetimeInputs` and `autoEDL` are one-line property flips, and
the bug they are prone to is flipping the wrong property or writing an absolute value
instead of the complement — both of which are invisible until a session is reloaded.
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


class _Event:
    def __init__(self, contents=""):
        self._contents = contents
        self.rejected = False

    def contents(self):
        return self._contents

    def reject(self):
        self.rejected = True


def labels(items):
    """Item labels of a menu, separators included as their "_" marker."""
    return [i[0] for i in items]


def itemNamed(items, label):
    for i in items:
        if i[0] == label:
            return i
    raise AssertionError("no menu item %r in %r" % (label, labels(items)))


class MenuTest(unittest.TestCase):
    MODULE = None
    CLASS = None
    NODE_TYPE = None
    GROUP_TYPE = None

    def setUp(self):
        if self.MODULE is None:
            self.skipTest("base class")
        self.mod, self.graph = _rv_stubs.importPort(self.MODULE)
        self.mode = getattr(self.mod, self.CLASS).__new__(getattr(self.mod, self.CLASS))
        self.mode._ui = None
        self.mode._uiInFlux = False

        self.graph.addNode("group", self.GROUP_TYPE)
        self.graph.addNode("node", self.NODE_TYPE, group="group")
        self.graph.viewNode = "group"

    def submenu(self):
        menu = self.mode.menu()
        self.assertEqual(len(menu), 1, "each mode contributes exactly one submenu")
        return menu[0][0], menu[0][1]

    def ints(self, name):
        return self.graph.getIntProperty(name)


class TestStackMenu(MenuTest):
    MODULE = "Stack_edit_mode"
    CLASS = "StackEditMode"
    GROUP_TYPE = "RVStackGroup"
    NODE_TYPE = "RVStack"

    def test_submenu_is_titled_stack_for_a_stack_view(self):
        title, _items = self.submenu()
        self.assertEqual(title, "Stack")

    def test_submenu_is_titled_layout_for_a_layout_view(self):
        """The same mode serves both view types; Mu picks the label off nodeType."""
        self.graph.addNode("lay", "RVLayoutGroup")
        self.graph.viewNode = "lay"
        title, _items = self.submenu()
        self.assertEqual(title, "Layout")

    def test_the_four_toggles_are_present_in_order(self):
        _title, items = self.submenu()
        self.assertEqual(labels(items), [
            "_",
            "Align Start Frames",
            "Use Source Cut Info",
            "Automatically Retime Inputs",
            "Use Strict Frame Ranges",
        ])

    def test_each_item_activates_its_own_toggle(self):
        _title, items = self.submenu()
        for label, prop in (
            ("Align Start Frames", "node.mode.alignStartFrames"),
            ("Use Source Cut Info", "node.mode.useCutInfo"),
            ("Use Strict Frame Ranges", "node.mode.strictFrameRanges"),
        ):
            with self.subTest(label=label):
                self.graph.seedInt(prop, [0])
                itemNamed(items, label)[1](_Event())
                self.assertEqual(self.ints(prop), [1])

    def test_retime_item_flips_the_view_timing_property(self):
        self.graph.seedInt("group.timing.retimeInputs", [0])
        _title, items = self.submenu()
        itemNamed(items, "Automatically Retime Inputs")[1](_Event())
        self.assertEqual(self.ints("group.timing.retimeInputs"), [1])

    def test_state_functions_report_the_current_flag(self):
        _title, items = self.submenu()
        self.graph.seedInt("node.mode.alignStartFrames", [0])
        self.assertEqual(itemNamed(items, "Align Start Frames")[3](),
                         self.mod.commands.UncheckedMenuState)
        self.graph.seedInt("node.mode.alignStartFrames", [1])
        self.assertEqual(itemNamed(items, "Align Start Frames")[3](),
                         self.mod.commands.CheckedMenuState)

    def test_state_functions_are_not_shared_between_items(self):
        """One `name` captured by reference would make all four report the same."""
        _title, items = self.submenu()
        self.graph.seedInt("node.mode.alignStartFrames", [1])
        self.graph.seedInt("node.mode.useCutInfo", [0])
        self.assertEqual(itemNamed(items, "Align Start Frames")[3](),
                         self.mod.commands.CheckedMenuState)
        self.assertEqual(itemNamed(items, "Use Source Cut Info")[3](),
                         self.mod.commands.UncheckedMenuState)

    def test_align_start_frames_toggles_rather_than_sets(self):
        self.graph.seedInt("node.mode.alignStartFrames", [1])
        self.mode.alignStartFrames(_Event())
        self.assertEqual(self.ints("node.mode.alignStartFrames"), [0])

    def test_strict_frame_ranges_toggles_rather_than_sets(self):
        self.graph.seedInt("node.mode.strictFrameRanges", [1])
        self.mode.strictFrameRanges(_Event())
        self.assertEqual(self.ints("node.mode.strictFrameRanges"), [0])

    def test_use_cut_info_toggles_rather_than_sets(self):
        self.graph.seedInt("node.mode.useCutInfo", [1])
        self.mode.useCutInfo(_Event())
        self.assertEqual(self.ints("node.mode.useCutInfo"), [0])

    def test_auto_retime_inputs_toggles_rather_than_sets(self):
        self.graph.seedInt("group.timing.retimeInputs", [1])
        self.mode.autoRetimeInputs(_Event())
        self.assertEqual(self.ints("group.timing.retimeInputs"), [0])

    def test_retime_state_reads_the_view_not_the_stack(self):
        self.graph.seedInt("group.timing.retimeInputs", [1])
        self.assertEqual(self.mode.retimeState(),
                         self.mod.commands.CheckedMenuState)
        self.graph.seedInt("group.timing.retimeInputs", [0])
        self.assertEqual(self.mode.retimeState(),
                         self.mod.commands.UncheckedMenuState)

    def test_state_func_names_the_property_it_is_given(self):
        self.graph.seedInt("node.mode.useCutInfo", [1])
        self.assertEqual(self.mode.stateFunc("useCutInfo")(),
                         self.mod.commands.CheckedMenuState)

    def test_update_menu_installs_the_current_menu(self):
        """The label depends on the view type, so the menu is rebuilt, not cached."""
        self.mode._menu = None
        self.mode.updateMenu()
        self.assertEqual(self.mode._menu[0][0], "Stack")

        self.graph.addNode("lay", "RVLayoutGroup")
        self.graph.viewNode = "lay"
        self.mode.updateMenu()
        self.assertEqual(self.mode._menu[0][0], "Layout")

    def test_disabled_category_blocks_the_toggle(self):
        """menuItem() gates on the event category; live review turns it off."""
        self.graph.enabledCategories = []
        self.graph.seedInt("node.mode.alignStartFrames", [0])
        _title, items = self.submenu()
        itemNamed(items, "Align Start Frames")[1](_Event())
        self.assertEqual(self.ints("node.mode.alignStartFrames"), [0])
        self.assertEqual(itemNamed(items, "Align Start Frames")[3](),
                         self.mod.commands.DisabledMenuState)

    def test_update_ui_event_rejects_then_updates(self):
        """Rejecting first is what lets the other range-changed handlers run."""
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        event = _Event()
        self.mode.updateUIEvent(event)
        self.assertTrue(event.rejected)
        self.assertEqual(calls, [1])

    def test_property_change_on_a_watched_name_updates(self):
        calls = []
        self.mode._ui = object()
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVStack.mode.alignStartFrames"))
        self.assertEqual(calls, [1])

    def test_property_change_is_ignored_without_a_panel(self):
        calls = []
        self.mode._ui = None
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVStack.mode.alignStartFrames"))
        self.assertEqual(calls, [])

    def test_property_change_on_an_unwatched_name_is_ignored(self):
        calls = []
        self.mode._ui = object()
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVStack.mode.somethingElse"))
        self.assertEqual(calls, [])

    def test_property_change_always_rejects(self):
        self.mode._ui = None
        event = _Event("#RVStack.mode.alignStartFrames")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)

    def test_activate_marks_the_mode_active(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)


class TestSwitchMenu(MenuTest):
    MODULE = "Switch_edit_mode"
    CLASS = "SwitchEditMode"
    GROUP_TYPE = "RVSwitchGroup"
    NODE_TYPE = "RVSwitch"

    def test_submenu_is_titled_switch(self):
        title, _items = self.submenu()
        self.assertEqual(title, "Switch")

    def test_it_has_the_two_toggles_and_no_separator(self):
        """Unlike Stack and Sequence, Mu's Switch menu opens with no menuSeparator."""
        _title, items = self.submenu()
        self.assertEqual(labels(items),
                         ["Align Start Frames", "Use Source Cut Info"])

    def test_each_item_activates_its_own_toggle(self):
        _title, items = self.submenu()
        for label, prop in (
            ("Align Start Frames", "node.mode.alignStartFrames"),
            ("Use Source Cut Info", "node.mode.useCutInfo"),
        ):
            with self.subTest(label=label):
                self.graph.seedInt(prop, [0])
                itemNamed(items, label)[1](_Event())
                self.assertEqual(self.ints(prop), [1])

    def test_toggles_flip_rather_than_set(self):
        self.graph.seedInt("node.mode.alignStartFrames", [1])
        self.mode.alignStartFrames(_Event())
        self.assertEqual(self.ints("node.mode.alignStartFrames"), [0])

        self.graph.seedInt("node.mode.useCutInfo", [1])
        self.mode.useCutInfo(_Event())
        self.assertEqual(self.ints("node.mode.useCutInfo"), [0])

    def test_state_func_tracks_the_flag(self):
        self.graph.seedInt("node.mode.useCutInfo", [0])
        self.assertEqual(self.mode.stateFunc("useCutInfo")(),
                         self.mod.commands.UncheckedMenuState)
        self.graph.seedInt("node.mode.useCutInfo", [1])
        self.assertEqual(self.mode.stateFunc("useCutInfo")(),
                         self.mod.commands.CheckedMenuState)

    def test_retime_state_reads_the_view_timing_property(self):
        self.graph.seedInt("group.timing.retimeInputs", [1])
        self.assertEqual(self.mode.retimeState(),
                         self.mod.commands.CheckedMenuState)

    def test_update_menu_installs_the_menu(self):
        self.mode._menu = None
        self.mode.updateMenu()
        self.assertEqual(self.mode._menu[0][0], "Switch")

    def test_update_ui_event_rejects_then_updates(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        event = _Event()
        self.mode.updateUIEvent(event)
        self.assertTrue(event.rejected)
        self.assertEqual(calls, [1])

    def test_property_change_on_a_watched_name_updates(self):
        calls = []
        self.mode._ui = object()
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVSwitch.mode.alignStartFrames"))
        self.assertEqual(calls, [1])

    def test_property_change_on_an_unwatched_name_is_ignored(self):
        calls = []
        self.mode._ui = object()
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVSwitch.mode.unrelated"))
        self.assertEqual(calls, [])

    def test_property_change_always_rejects(self):
        event = _Event("#RVSwitch.mode.unrelated")
        self.mode._ui = object()
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)

    def test_activate_marks_the_mode_active(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)

    def test_size_edits_write_one_axis_each(self):
        self.graph.seedInt("node.output.size", [100, 50])
        self.mode._outputWidthEdit = QtWidgets.QLineEdit("640")
        self.mode._outputHeightEdit = QtWidgets.QLineEdit("480")

        self.mode.widthChanged()
        self.assertEqual(self.ints("node.output.size"), [640, 50])

        self.mode.heightChanged()
        self.assertEqual(self.ints("node.output.size"), [640, 480])


class TestSequenceMenu(MenuTest):
    MODULE = "SequenceGroup_edit_mode"
    CLASS = "SequenceGroupEditMode"
    GROUP_TYPE = "RVSequenceGroup"
    NODE_TYPE = "RVSequence"

    def test_submenu_is_titled_sequence(self):
        title, _items = self.submenu()
        self.assertEqual(title, "Sequence")

    def test_it_opens_with_a_separator_then_the_two_toggles(self):
        _title, items = self.submenu()
        self.assertEqual(labels(items),
                         ["_", "Auto EDL", "Use Source Cut Info"])

    def test_auto_edl_item_flips_auto_edl(self):
        self.graph.seedInt("node.mode.autoEDL", [0])
        _title, items = self.submenu()
        itemNamed(items, "Auto EDL")[1](_Event())
        self.assertEqual(self.ints("node.mode.autoEDL"), [1])

    def test_cut_info_item_flips_use_cut_info(self):
        self.graph.seedInt("node.mode.useCutInfo", [0])
        _title, items = self.submenu()
        itemNamed(items, "Use Source Cut Info")[1](_Event())
        self.assertEqual(self.ints("node.mode.useCutInfo"), [1])

    def test_toggles_flip_rather_than_set(self):
        self.graph.seedInt("node.mode.autoEDL", [1])
        self.mode.autoEDL(_Event())
        self.assertEqual(self.ints("node.mode.autoEDL"), [0])

        self.graph.seedInt("node.mode.useCutInfo", [1])
        self.mode.useCutInfo(_Event())
        self.assertEqual(self.ints("node.mode.useCutInfo"), [0])

    def test_state_func_tracks_the_flag(self):
        self.graph.seedInt("node.mode.autoEDL", [1])
        self.assertEqual(self.mode.stateFunc("autoEDL")(),
                         self.mod.commands.CheckedMenuState)

    def test_update_ui_event_rejects_then_updates(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        event = _Event()
        self.mode.updateUIEvent(event)
        self.assertTrue(event.rejected)
        self.assertEqual(calls, [1])

    def test_property_change_on_a_watched_name_updates_and_redraws(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        before = self.graph.redraws
        self.mode.propertyChanged(_Event("#RVSequence.mode.autoEDL"))
        self.assertEqual(calls, [1])
        self.assertEqual(self.graph.redraws, before + 1)

    def test_property_change_on_an_unwatched_name_is_ignored(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        before = self.graph.redraws
        self.mode.propertyChanged(_Event("#RVSequence.mode.unrelated"))
        self.assertEqual(calls, [])
        self.assertEqual(self.graph.redraws, before)

    def test_property_change_always_rejects(self):
        event = _Event("#RVSequence.mode.unrelated")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)

    def test_activate_marks_the_mode_active(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)


class TestCompositeMenu(unittest.TestCase):
    """Composite builds its menu inline in init(), so it is read back off the mode."""

    OPS = ["over", "add", "dissolve", "difference", "-difference", "replace",
           "topmost"]

    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("Composite_edit_mode")
        self.graph.addNode("group", "RVStackGroup")
        self.graph.addNode("node", "RVStack", group="group")
        self.graph.viewNode = "group"
        self.mode = self.mod.createMode()

    def items(self):
        return self.mode._menu[0][1]

    def test_the_submenu_is_added_to_stack(self):
        self.assertEqual(self.mode._menu[0][0], "Stack")

    def test_all_seven_operations_are_listed(self):
        found = [l.strip() for l in labels(self.items()) if l.startswith("   ")]
        self.assertEqual(found, [
            "Over", "Add", "Dissolve", "Difference", "Inverted Difference",
            "Replace", "Topmost",
        ])

    def test_op_state_checks_only_the_current_operation(self):
        self.graph.seedString("node.composite.type", ["dissolve"])
        for op in self.OPS:
            with self.subTest(op=op):
                expected = (self.mod.commands.CheckedMenuState if op == "dissolve"
                            else self.mod.commands.UncheckedMenuState)
                self.assertEqual(self.mode.opState(op)(), expected)

    def test_op_state_follows_a_change(self):
        self.graph.seedString("node.composite.type", ["over"])
        state = self.mode.opState("add")
        self.assertEqual(state(), self.mod.commands.UncheckedMenuState)
        self.graph.seedString("node.composite.type", ["add"])
        self.assertEqual(state(), self.mod.commands.CheckedMenuState)

    def test_each_operation_item_reports_its_own_state(self):
        """Each menuItem closes over its own op name, not the loop variable."""
        self.graph.seedString("node.composite.type", ["replace"])
        checked = [i[0].strip() for i in self.items()
                   if len(i) > 3 and i[3] is not None
                   and i[3]() == self.mod.commands.CheckedMenuState]
        self.assertEqual(checked, ["Replace"])

    def test_the_cycle_items_are_present(self):
        self.assertIn("Cycle Forward", labels(self.items()))
        self.assertIn("Cycle Backward", labels(self.items()))

    def test_property_change_on_type_updates_the_panel(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVStack.composite.type"))
        self.assertEqual(calls, [1])

    def test_property_change_on_dissolve_amount_updates_the_panel(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVStack.composite.dissolveAmount"))
        self.assertEqual(calls, [1])

    def test_property_change_elsewhere_is_ignored(self):
        calls = []
        self.mode.updateUI = lambda: calls.append(1)
        self.mode.propertyChanged(_Event("#RVStack.output.fps"))
        self.assertEqual(calls, [])

    def test_property_change_always_rejects(self):
        event = _Event("#RVStack.output.fps")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


if __name__ == "__main__":
    unittest.main()
