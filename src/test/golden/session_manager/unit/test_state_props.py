"""Gate 5 — the sm_state.* property helpers, exercised on the port itself.

These are the functions that persist tree state into the session graph, so each test
drives the real function and then reads the FakeGraph to check what landed in the
property — the same thing the behavioral gate diffs out of session.rv, at one
function's granularity.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QStandardItem


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class StateTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")

    def strings(self, name):
        return self.graph.getStringProperty(name)

    def ints(self, name):
        return self.graph.getIntProperty(name)


class TestExpandedInParent(StateTest):
    PROP = "src.sm_state.expandState"

    def test_absent_property_reads_false(self):
        self.assertFalse(self.sm.isExpandedInParent("src", "folder"))

    def test_first_write_creates_scalar_property(self):
        """Mu passes the bare parent, not a one-element array, on the create path."""
        self.sm.setExpandedInParent("src", "folder", True)
        self.assertEqual(self.strings(self.PROP), ["folder"])
        self.assertTrue(self.sm.isExpandedInParent("src", "folder"))

    def test_collapse_removes_only_that_parent(self):
        self.sm.setExpandedInParent("src", "folderA", True)
        self.sm.setExpandedInParent("src", "folderB", True)
        self.assertEqual(self.strings(self.PROP), ["folderA", "folderB"])

        self.sm.setExpandedInParent("src", "folderA", False)
        self.assertEqual(self.strings(self.PROP), ["folderB"])
        self.assertFalse(self.sm.isExpandedInParent("src", "folderA"))
        self.assertTrue(self.sm.isExpandedInParent("src", "folderB"))

    def test_expanding_twice_does_not_duplicate(self):
        self.sm.setExpandedInParent("src", "folder", True)
        self.sm.setExpandedInParent("src", "folder", True)
        self.assertEqual(self.strings(self.PROP), ["folder"])

    def test_collapsing_an_absent_parent_is_a_noop(self):
        self.sm.setExpandedInParent("src", "folderA", True)
        self.sm.setExpandedInParent("src", "folderB", False)
        self.assertEqual(self.strings(self.PROP), ["folderA"])

    def test_top_level_parent_is_the_empty_string(self):
        """A node sitting directly under a category has "" for its parent node.

        Worth pinning: the property is created with an empty value, which is exactly
        the phantom write that appeared when mapItems() returned a sub-component
        first and scrollTo() expanded the node row.
        """
        self.sm.setExpandedInParent("src", "", True)
        self.assertEqual(self.strings(self.PROP), [""])


class TestSubComponentExpanded(StateTest):
    PROP = "src.sm_state.expandedSubState"

    def _viewItem(self):
        # The parent has to be kept alive: appendRow() hands ownership of the child
        # to it, so letting it go out of scope deletes the child's C++ object and any
        # later item.data() raises "Internal C++ object already deleted" — the same
        # ownership trap that the session-window wrapper hit in the port itself.
        media = QStandardItem("m.exr")
        media.setData(self.sm.MediaSubComponent, Qt.UserRole + 4)
        media.setData("m.exr", Qt.UserRole + 5)
        view = QStandardItem("left")
        view.setData(self.sm.ViewSubComponent, Qt.UserRole + 4)
        view.setData("left", Qt.UserRole + 5)
        media.appendRow([view])
        self._retain = media
        return view

    def test_absent_property_reads_false(self):
        self.assertFalse(self.sm.isSubComponentExpanded("src", self._viewItem()))

    def test_round_trip_uses_the_item_hash_as_key(self):
        item = self._viewItem()
        self.sm.setSubComponentExpanded("src", item, True)
        self.assertEqual(self.strings(self.PROP), [self.sm.hashedSubComponent(item)])
        self.assertTrue(self.sm.isSubComponentExpanded("src", item))

    def test_collapse_removes_the_key(self):
        item = self._viewItem()
        self.sm.setSubComponentExpanded("src", item, True)
        self.sm.setSubComponentExpanded("src", item, False)
        self.assertEqual(self.strings(self.PROP), [])
        self.assertFalse(self.sm.isSubComponentExpanded("src", item))

    def test_first_write_is_a_one_element_array(self):
        """Unlike expandState, Mu creates this one with string[]{key}."""
        item = self._viewItem()
        self.sm.setSubComponentExpanded("src", item, True)
        self.assertEqual(len(self.strings(self.PROP)), 1)


class TestToolTipProp(StateTest):
    def test_missing_returns_none(self):
        self.assertIsNone(self.sm.toolTipFromProp("src"))

    def test_round_trip(self):
        self.sm.setToolTipProp("src", "some tip")
        self.assertEqual(self.sm.toolTipFromProp("src"), "some tip")

    def test_overwrite(self):
        self.sm.setToolTipProp("src", "first")
        self.sm.setToolTipProp("src", "second")
        self.assertEqual(self.sm.toolTipFromProp("src"), "second")


class TestSortKey(StateTest):
    KEY = "src.sm_state.sortKey"
    PARENT = "src.sm_state.sortKeyParent"

    def test_missing_returns_undefined_marker(self):
        self.assertEqual(self.sm.sortKeyInParent("src", "folder"),
                         self.sm.UNDEFINED_SORT_KEY)
        self.assertEqual(self.sm.UNDEFINED_SORT_KEY, 2 ** 31 - 1 - 100)

    def test_first_write_creates_both_properties(self):
        self.sm.setSortKeyInParent("src", "folder", 3)
        self.assertEqual(self.strings(self.PARENT), ["folder"])
        self.assertEqual(self.ints(self.KEY), [3])
        self.assertEqual(self.sm.sortKeyInParent("src", "folder"), 3)

    def test_second_parent_appends_a_pair(self):
        self.sm.setSortKeyInParent("src", "folderA", 1)
        self.sm.setSortKeyInParent("src", "folderB", 2)
        self.assertEqual(self.strings(self.PARENT), ["folderA", "folderB"])
        self.assertEqual(self.ints(self.KEY), [1, 2])
        self.assertEqual(self.sm.sortKeyInParent("src", "folderA"), 1)
        self.assertEqual(self.sm.sortKeyInParent("src", "folderB"), 2)

    def test_rewriting_a_known_parent_updates_in_place(self):
        self.sm.setSortKeyInParent("src", "folderA", 1)
        self.sm.setSortKeyInParent("src", "folderB", 2)
        self.sm.setSortKeyInParent("src", "folderA", 9)
        self.assertEqual(self.strings(self.PARENT), ["folderA", "folderB"])
        self.assertEqual(self.ints(self.KEY), [9, 2])

    def test_unknown_parent_reads_undefined(self):
        self.sm.setSortKeyInParent("src", "folderA", 1)
        self.assertEqual(self.sm.sortKeyInParent("src", "other"),
                         self.sm.UNDEFINED_SORT_KEY)

    def test_length_mismatch_reads_undefined(self):
        """A half-written pair must not be trusted; Mu falls back to the marker."""
        self.sm.setSortKeyInParent("src", "folderA", 1)
        self.sm.setStringProp(self.PARENT, ["folderA", "folderB"])
        self.assertEqual(self.sm.sortKeyInParent("src", "folderB"),
                         self.sm.UNDEFINED_SORT_KEY)


class TestAssignSortOrder(StateTest):
    def test_numbers_children_in_current_order(self):
        root = QStandardItem("folder")
        root.setData("folderNode", Qt.UserRole + 2)
        for name in ("a", "b", "c"):
            child = QStandardItem(name)
            child.setData(name, Qt.UserRole + 2)
            root.appendRow([child])

        self.sm.assignSortOrder(root)

        self.assertEqual(self.sm.sortKeyInParent("a", "folderNode"), 0)
        self.assertEqual(self.sm.sortKeyInParent("b", "folderNode"), 1)
        self.assertEqual(self.sm.sortKeyInParent("c", "folderNode"), 2)

    def test_none_root_is_a_noop(self):
        self.sm.assignSortOrder(None)   # must not raise

    def test_empty_root_writes_nothing(self):
        root = QStandardItem("folder")
        root.setData("folderNode", Qt.UserRole + 2)
        before = dict(self.graph.props)
        self.sm.assignSortOrder(root)
        self.assertEqual(self.graph.props, before)


if __name__ == "__main__":
    unittest.main()
