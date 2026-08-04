"""Gate 5 — the two shims the port needs because Python's bindings differ from Mu's.

Both of these replace something Mu got for free, and both had a defect that no golden
scenario could see:

* ``menuItem`` stands in for app_utils.menuItem, whose only observable effect is the
  event-category gate it wraps around a menu item's callback and state function. The
  sibling ports originally dropped the gate, which is invisible until a category is
  filtered off during live review.
* ``checkStateIsChecked`` exists because PySide6 6.5's ``Qt.CheckState`` is a plain
  ``enum.Enum``: ``2 == Qt.Checked`` is False and ``int(Qt.Checked)`` raises, so the
  direct comparison Mu uses silently evaluated to "unchecked" for every checkbox in
  the package.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6.QtCore import Qt


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class ShimTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")


class TestCheckStateIsChecked(ShimTest):
    def test_raw_int_from_the_signal(self):
        self.assertTrue(self.sm.checkStateIsChecked(2))
        self.assertFalse(self.sm.checkStateIsChecked(0))

    def test_enum_value_also_works(self):
        self.assertTrue(self.sm.checkStateIsChecked(Qt.Checked))
        self.assertFalse(self.sm.checkStateIsChecked(Qt.Unchecked))

    def test_partially_checked_is_not_checked(self):
        self.assertFalse(self.sm.checkStateIsChecked(1))

    def test_the_naive_comparison_really_is_broken(self):
        """Guards the reason this helper exists, so nobody inlines it back.

        If a future PySide6 makes Qt.CheckState an IntEnum this assertion fails and
        the helper can be revisited deliberately rather than by accident.
        """
        self.assertFalse(2 == Qt.Checked)


class TestMenuItem(ShimTest):
    def _item(self, category="viewmode_category"):
        self.calls = []
        return self.sm.menuItem(
            "Label", "", category,
            lambda event: self.calls.append(event),
            lambda: self.sm.commands.CheckedMenuState,
        )

    def test_shape_matches_a_python_menu_entry(self):
        label, func, key, stateFunc = self._item()
        self.assertEqual(label, "Label")
        self.assertIsNone(key, "every session_manager menuItem has no accelerator")
        self.assertTrue(callable(func))
        self.assertTrue(callable(stateFunc))

    def test_enabled_category_passes_the_state_through(self):
        _, _, _, stateFunc = self._item()
        self.assertEqual(stateFunc(), self.sm.commands.CheckedMenuState)

    def test_enabled_category_runs_the_callback(self):
        _, func, _, _ = self._item()
        func("event")
        self.assertEqual(self.calls, ["event"])

    def test_disabled_category_forces_disabled_state(self):
        self.graph.enabledCategories = set()      # nothing enabled
        _, _, _, stateFunc = self._item()
        self.assertEqual(stateFunc(), self.sm.commands.DisabledMenuState)

    def test_disabled_category_blocks_the_callback(self):
        self.graph.enabledCategories = set()
        _, func, _, _ = self._item()
        func("event")
        self.assertEqual(self.calls, [], "a blocked item must not run its action")

    def test_disabled_category_reports_the_block(self):
        self.graph.enabledCategories = set()
        _, func, _, _ = self._item()
        func("event")
        self.assertIn(("category-event-blocked", "viewmode_category"),
                      self.graph.events)

    def test_only_the_named_category_matters(self):
        self.graph.enabledCategories = {"source_category"}
        _, _, _, viewState = self._item("viewmode_category")
        _, _, _, sourceState = self._item("source_category")
        self.assertEqual(viewState(), self.sm.commands.DisabledMenuState)
        self.assertEqual(sourceState(), self.sm.commands.CheckedMenuState)

    def test_state_is_re_evaluated_per_call(self):
        """The gate has to be live: categories are toggled while RV runs."""
        _, _, _, stateFunc = self._item()
        self.assertEqual(stateFunc(), self.sm.commands.CheckedMenuState)
        self.graph.enabledCategories = set()
        self.assertEqual(stateFunc(), self.sm.commands.DisabledMenuState)

    def test_a_non_empty_event_pattern_is_rejected(self):
        """The shim does not implement bind(); it must say so rather than drop it."""
        with self.assertRaises(AssertionError):
            self.sm.menuItem("L", "key-down--x", "viewmode_category",
                             lambda e: None, lambda: 0)


class TestSiblingMenusAreGated(ShimTest):
    """Every ported sibling menu entry must carry the gate, not just some of them."""

    MODULES = (
        "Composite_edit_mode", "LayoutGroup_edit_mode", "RetimeGroup_edit_mode",
        "SequenceGroup_edit_mode", "SourceGroup_edit_mode", "Stack_edit_mode",
        "Switch_edit_mode", "transform_manip",
    )

    def test_each_module_imports_the_shim(self):
        import os
        import re

        missing = []
        for name in self.MODULES:
            path = os.path.join(_rv_stubs.PKG_DIR, name + ".py")
            source = open(path).read()
            if not re.search(r"^from session_manager import .*\bmenuItem\b",
                             source, re.M):
                missing.append(name)
        self.assertEqual(missing, [])

    def test_no_module_builds_a_bare_four_tuple_menu_entry(self):
        """A raw (label, func, key, stateFunc) tuple would be an ungated entry.

        Separators ("_", None) and the disabled text rows are 2- and 4-tuples with a
        None callback, so the check looks for a callable second element written
        inline, which is what an un-migrated menuItem call site looks like.
        """
        import os
        import re

        offenders = []
        for name in self.MODULES:
            path = os.path.join(_rv_stubs.PKG_DIR, name + ".py")
            for lineno, line in enumerate(open(path), 1):
                # (label, self.something, None, self.state) on one line
                if re.search(r'\(\s*"[^"]+"\s*,\s*(self\.|_)\w+\s*,\s*None\s*,', line):
                    offenders.append("%s:%d" % (name, lineno))
        self.assertEqual(offenders, [])


if __name__ == "__main__":
    unittest.main()
