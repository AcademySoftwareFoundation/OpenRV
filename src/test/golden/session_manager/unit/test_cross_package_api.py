"""Gate 5 — the API other packages reach this one through.

`rvnuke` and `maya_tools` used to `require session_manager` and call
`theMode().selectedNodes()`. A Mu require cannot resolve a Python module, so both now
go through `selectedNodeLines()` (via Mu's python module) and fall back to the
`session-manager-selected-nodes` internal event. Nothing else covers this: no golden
scenario sends the event, and both entry points survived mutation to `return None`
with the rest of the suite green.

The distinction that matters most here is *when* the selection is readable. RV only
dispatches internal events to active modes, and session_manager is `load: delay`, so
an event-only implementation reported an empty selection whenever the panel was
closed — which silently flipped rvnuke's and maya_tools' menu states. Reading the
mode object directly restores the original behavior, and the tests below pin it.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtCore, QtGui, QtWidgets
    from PySide6.QtCore import Qt

_app = None


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)
    global _app
    _app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])


class CrossPackageTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)

        self.model = QtGui.QStandardItemModel()
        self.view = QtWidgets.QTreeView()
        self.view.setModel(self.model)
        self.view.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.mode._viewModel = self.model
        self.mode._viewTreeView = self.view

        self.category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([self.category])

        # The module-level mode selectedNodeLines() reads.
        self.sm._theMode = self.mode

    def tearDown(self):
        self.sm._theMode = None
        self.view.setParent(None)

    def addNode(self, node):
        self.graph.addNode(node, "RVSourceGroup")
        item = QtGui.QStandardItem(node)
        item.setData(node, Qt.UserRole + 2)
        status = QtGui.QStandardItem("")
        self.category.appendRow([item, status])
        return item

    def select(self, *items):
        model = self.view.selectionModel()
        model.clearSelection()
        for item in items:
            model.select(self.model.indexFromItem(item),
                         QtCore.QItemSelectionModel.Select)


class TestSelectedNodes(CrossPackageTest):
    def test_reports_the_selected_node(self):
        a = self.addNode("srcA")
        self.select(a)
        self.assertEqual(self.mode.selectedNodes(), ["srcA"])

    def test_reports_several_in_tree_order(self):
        a = self.addNode("srcA")
        b = self.addNode("srcB")
        self.select(a, b)
        self.assertEqual(self.mode.selectedNodes(), ["srcA", "srcB"])

    def test_empty_when_nothing_is_selected(self):
        self.addNode("srcA")
        self.assertEqual(self.mode.selectedNodes(), [])

    def test_skips_rows_whose_node_no_longer_exists(self):
        """A row can outlive its node between a delete and the next updateTree."""
        a = self.addNode("srcA")
        self.select(a)
        self.graph.deleteNode("srcA")
        self.assertEqual(self.mode.selectedNodes(), [])

    def test_structural_rows_contribute_nothing(self):
        self.select(self.category)
        self.assertEqual(self.mode.selectedNodes(), [])


class TestSelectedNodeLines(CrossPackageTest):
    """The Mu-facing entry point: one node per line."""

    def test_single_node(self):
        a = self.addNode("srcA")
        self.select(a)
        self.assertEqual(self.sm.selectedNodeLines(), "srcA")

    def test_several_nodes_are_newline_separated(self):
        a = self.addNode("srcA")
        b = self.addNode("srcB")
        self.select(a, b)
        self.assertEqual(self.sm.selectedNodeLines(), "srcA\nsrcB")

    def test_empty_selection_is_the_empty_string(self):
        self.addNode("srcA")
        self.assertEqual(self.sm.selectedNodeLines(), "")

    def test_no_mode_loaded_is_the_empty_string(self):
        """"" is the signal that makes the Mu side fall back to the event."""
        self.sm._theMode = None
        self.assertEqual(self.sm.selectedNodeLines(), "")

    def test_round_trips_through_the_mu_split(self):
        """The Mu helper does content.split("\\n") and drops empties."""
        a = self.addNode("srcA")
        b = self.addNode("srcB")
        self.select(a, b)

        content = self.sm.selectedNodeLines()
        recovered = [n for n in content.split("\n") if n != ""]

        self.assertEqual(recovered, self.mode.selectedNodes())

    def test_empty_round_trip_yields_no_nodes(self):
        self.addNode("srcA")
        recovered = [n for n in self.sm.selectedNodeLines().split("\n") if n != ""]
        self.assertEqual(recovered, [])

    def test_node_names_never_contain_a_newline(self):
        """The encoding is only unambiguous because RV node names cannot."""
        a = self.addNode("srcA")
        self.select(a)
        self.assertNotIn("\n", self.mode.selectedNodes()[0])


class TestSelectedNodesEvent(CrossPackageTest):
    """The fallback path, used when the Mu implementation is the loaded one."""

    class _Event:
        def __init__(self):
            self.returned = None

        def setReturnContent(self, content):
            self.returned = content

    def test_answers_with_the_same_encoding(self):
        a = self.addNode("srcA")
        b = self.addNode("srcB")
        self.select(a, b)

        event = self._Event()
        self.mode.selectedNodesEvent(event)

        self.assertEqual(event.returned, "srcA\nsrcB")

    def test_answers_empty_for_no_selection(self):
        self.addNode("srcA")
        event = self._Event()
        self.mode.selectedNodesEvent(event)
        self.assertEqual(event.returned, "")

    def test_agrees_with_selectedNodeLines(self):
        """Both entry points must never disagree, or the two callers diverge."""
        a = self.addNode("srcA")
        self.select(a)

        event = self._Event()
        self.mode.selectedNodesEvent(event)

        self.assertEqual(event.returned, self.sm.selectedNodeLines())


class TestMuCallerGlueIsWired(unittest.TestCase):
    """The Mu side must actually call the entry point, and keep its fallback."""

    CALLERS = (
        "src/plugins/rv-packages/rvnuke/rvnuke_mode.mu.in",
        "src/plugins/rv-packages/maya_tools/maya_tools.mu.in",
    )

    def _read(self, rel):
        import os

        root = os.path.abspath(
            os.path.join(_rv_stubs.PKG_DIR, "..", "..", "..", "..")
        )
        return open(os.path.join(root, rel)).read()

    def test_neither_caller_requires_session_manager_any_more(self):
        for rel in self.CALLERS:
            self.assertNotIn("require session_manager;", self._read(rel), rel)

    def test_both_callers_ask_the_python_mode_first(self):
        for rel in self.CALLERS:
            self.assertIn("selectedNodeLines", self._read(rel), rel)

    def test_both_callers_keep_the_event_fallback(self):
        for rel in self.CALLERS:
            self.assertIn("session-manager-selected-nodes", self._read(rel), rel)

    def test_both_callers_split_on_newline(self):
        for rel in self.CALLERS:
            self.assertIn('content.split("\\n")', self._read(rel), rel)


if __name__ == "__main__":
    unittest.main()
