"""Gate 5 — SessionManagerMode.renameByType on the port itself.

This is the name a user sees on every node the Add and Folder menus create, and it is
one of the few places the Mu source builds a string by hand with branch-dependent
punctuation, so each arm is pinned separately. The method is called on a mode built
with object.__new__: renameByType touches no widget state, only nodeType() and the
uiName commands, so constructing the full dock would add nothing but a dependency on
a live RV.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class RenameTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)

    def rename(self, nodeType, inputs, uiNames=None):
        self.graph.addNode("target", nodeType)
        for name in inputs:
            self.graph.addNode(name, "RVSourceGroup")
        if uiNames:
            self.graph.uiNames.update(uiNames)
        self.mode.renameByType("target", inputs)
        return self.graph.uiName("target")


class TestRenameByType(RenameTest):
    def test_empty_sequence(self):
        self.assertEqual(self.rename("RVSequenceGroup", []), "Empty Sequence")

    def test_empty_stack(self):
        self.assertEqual(self.rename("RVStackGroup", []), "Empty Stack")

    def test_empty_folder(self):
        self.assertEqual(self.rename("RVFolderGroup", []), "Empty Folder")

    def test_rv_prefix_and_group_suffix_are_both_stripped(self):
        self.assertEqual(self.rename("RVSwitchGroup", []), "Empty Switch")

    def test_type_without_group_suffix_keeps_the_rest(self):
        self.assertEqual(self.rename("RVStack", []), "Empty Stack")

    def test_type_without_rv_prefix_is_left_alone(self):
        self.assertEqual(self.rename("CustomGroup", []), "Empty Custom")

    def test_one_input(self):
        self.assertEqual(
            self.rename("RVSequenceGroup", ["a"], {"a": "SrcA"}),
            "Sequence of SrcA",
        )

    def test_two_inputs_are_joined_with_and(self):
        self.assertEqual(
            self.rename("RVSequenceGroup", ["a", "b"], {"a": "SrcA", "b": "SrcB"}),
            "Sequence of SrcA and SrcB",
        )

    def test_three_or_more_inputs_are_counted(self):
        """Note the trailing space; it is in the Mu format string and is kept."""
        self.assertEqual(
            self.rename("RVStackGroup", ["a", "b", "c"]),
            "Stack of 3 views ",
        )

    def test_many_inputs_use_the_count_form(self):
        self.assertEqual(
            self.rename("RVLayoutGroup", ["a", "b", "c", "d", "e"]),
            "Layout of 5 views ",
        )

    def test_uses_ui_names_not_node_names(self):
        got = self.rename(
            "RVSequenceGroup", ["nodeA", "nodeB"],
            {"nodeA": "Renamed A", "nodeB": "Renamed B"},
        )
        self.assertEqual(got, "Sequence of Renamed A and Renamed B")


if __name__ == "__main__":
    unittest.main()
