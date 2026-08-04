"""Gate 5 — setInputs / removeInput / hasInput / addInput on the port itself.

These four wrap RV's node-connection API and share one rule the tests pin: a
rejected input set must leave the graph untouched and surface an alert rather than
silently half-applying.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class NodeOpTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.graph.addNode("a", "RVSourceGroup")
        self.graph.addNode("b", "RVSourceGroup")
        self.graph.addNode("c", "RVSourceGroup")
        self.graph.addNode("seq", "RVSequenceGroup", inputs=["a", "b"])


class TestSetInputs(NodeOpTest):
    def test_accepted_inputs_are_written(self):
        self.assertTrue(self.sm.setInputs("seq", ["b", "a"]))
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["b", "a"])
        self.assertEqual(self.graph.alerts, [])

    def test_rejected_inputs_alert_and_change_nothing(self):
        self.assertFalse(self.sm.setInputs("seq", ["a", "nonexistent"]))
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["a", "b"])
        self.assertEqual(len(self.graph.alerts), 1)

    def test_empty_input_list_is_allowed(self):
        self.assertTrue(self.sm.setInputs("seq", []))
        self.assertEqual(self.graph.nodeConnections("seq")[0], [])


class TestRemoveInput(NodeOpTest):
    def test_removes_the_named_input(self):
        self.assertTrue(self.sm.removeInput("seq", "a"))
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["b"])

    def test_removing_an_absent_input_leaves_the_list_alone(self):
        self.assertTrue(self.sm.removeInput("seq", "zzz"))
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["a", "b"])

    def test_removes_every_occurrence(self):
        self.graph.setNodeInputs("seq", ["a", "b", "a"])
        self.sm.removeInput("seq", "a")
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["b"])

    def test_empty_node_name_is_a_noop(self):
        self.assertTrue(self.sm.removeInput("", "a"))


class TestHasInput(NodeOpTest):
    def test_present_and_absent(self):
        self.assertTrue(self.sm.hasInput("seq", "a"))
        self.assertFalse(self.sm.hasInput("seq", "c"))

    def test_absent_node_reports_true(self):
        """Mu returns true for a nil/empty node so callers skip the add entirely."""
        self.assertTrue(self.sm.hasInput(None, "a"))
        self.assertTrue(self.sm.hasInput("", "a"))


class TestAddInput(NodeOpTest):
    def test_appends_at_the_end(self):
        self.assertTrue(self.sm.addInput("seq", "c"))
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["a", "b", "c"])

    def test_adding_a_duplicate_appends_again(self):
        """addInput does not dedupe; callers guard with hasInput()."""
        self.sm.addInput("seq", "a")
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["a", "b", "a"])

    def test_nonexistent_target_node_is_a_noop(self):
        self.assertTrue(self.sm.addInput("noSuchNode", "a"))
        self.assertEqual(self.graph.alerts, [])


if __name__ == "__main__":
    unittest.main()
