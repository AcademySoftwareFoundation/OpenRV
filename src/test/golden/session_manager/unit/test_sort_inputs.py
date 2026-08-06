"""Gate 5 — SessionManagerMode.sortInputs on the port itself.

Mu sorts with a hand-written insertion pass rather than a library sort, and the
inputs panel order it produces is a primary outcome (#7). The method is driven on an
instance built with object.__new__ with the two collaborators it actually reaches
stubbed, so what is under test is the ordering and the folder sort-key writeback.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class SortTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._inputOrderLock = False
        self.mode.updateInputs = lambda node: None
        self.treeUpdates = []
        self.mode.updateTree = lambda: self.treeUpdates.append(1)

    def makeParent(self, nodeType, inputs, uiNames=None):
        for n in inputs:
            self.graph.addNode(n, "RVSourceGroup")
        self.graph.addNode("parent", nodeType, inputs=inputs)
        self.graph.viewNode = "parent"
        if uiNames:
            self.graph.uiNames.update(uiNames)

    def order(self):
        return self.graph.nodeConnections("parent")[0]


class TestSortInputs(SortTest):
    def test_ascending(self):
        self.makeParent("RVSequenceGroup", ["c", "a", "b"],
                        {"a": "Apple", "b": "Banana", "c": "Cherry"})
        self.mode.sortInputs(True, False)
        self.assertEqual(self.order(), ["a", "b", "c"])

    def test_descending(self):
        self.makeParent("RVSequenceGroup", ["a", "c", "b"],
                        {"a": "Apple", "b": "Banana", "c": "Cherry"})
        self.mode.sortInputs(False, False)
        self.assertEqual(self.order(), ["c", "b", "a"])

    def test_already_sorted_is_stable(self):
        self.makeParent("RVSequenceGroup", ["a", "b", "c"],
                        {"a": "Apple", "b": "Banana", "c": "Cherry"})
        self.mode.sortInputs(True, False)
        self.assertEqual(self.order(), ["a", "b", "c"])

    def test_single_input(self):
        self.makeParent("RVSequenceGroup", ["a"], {"a": "Apple"})
        self.mode.sortInputs(True, False)
        self.assertEqual(self.order(), ["a"])

    def test_empty_inputs(self):
        self.makeParent("RVSequenceGroup", [])
        self.mode.sortInputs(True, False)
        self.assertEqual(self.order(), [])

    def test_sorts_on_ui_name_not_node_name(self):
        self.makeParent("RVSequenceGroup", ["n1", "n2"],
                        {"n1": "Zebra", "n2": "Antelope"})
        self.mode.sortInputs(True, False)
        self.assertEqual(self.order(), ["n2", "n1"])

    def test_lock_suppresses_the_sort(self):
        self.makeParent("RVSequenceGroup", ["c", "a"],
                        {"a": "Apple", "c": "Cherry"})
        self.mode._inputOrderLock = True
        self.mode.sortInputs(True, False)
        self.assertEqual(self.order(), ["c", "a"])


class TestSortInputsOnAFolder(SortTest):
    def test_folder_sort_records_sort_keys(self):
        self.makeParent("RVFolderGroup", ["c", "a", "b"],
                        {"a": "Apple", "b": "Banana", "c": "Cherry"})
        self.mode.sortInputs(True, False)

        self.assertEqual(self.sm.sortKeyInParent("a", "parent"), 0)
        self.assertEqual(self.sm.sortKeyInParent("b", "parent"), 1)
        self.assertEqual(self.sm.sortKeyInParent("c", "parent"), 2)

    def test_folder_sort_refreshes_the_tree(self):
        self.makeParent("RVFolderGroup", ["b", "a"], {"a": "Apple", "b": "Banana"})
        self.mode.sortInputs(True, False)
        self.assertEqual(len(self.treeUpdates), 1)

    def test_non_folder_sort_does_not_touch_sort_keys(self):
        self.makeParent("RVSequenceGroup", ["b", "a"], {"a": "Apple", "b": "Banana"})
        self.mode.sortInputs(True, False)
        self.assertEqual(self.sm.sortKeyInParent("a", "parent"),
                         self.sm.UNDEFINED_SORT_KEY)
        self.assertEqual(self.treeUpdates, [])


if __name__ == "__main__":
    unittest.main()
