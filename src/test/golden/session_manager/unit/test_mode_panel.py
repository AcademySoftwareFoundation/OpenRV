"""Gate 5 — mode methods that drive the panel widgets.

Constructing the whole mode headlessly is not an option: `SessionManagerMode(name)`
gets as far as parenting its dock and then **segfaults** under the offscreen platform
(exit 139, reproducible), somewhere in the dock/WebEngine path. So each test builds
the two or three real widgets the method under test actually touches and attaches them
to an instance made with `object.__new__` — the same approach as test_mode.py, applied
to the panel-facing half of the mode.

What that buys over a mock: these are genuine QStandardItemModel / QTreeView /
QListView objects, so selection, row layout and index arithmetic behave exactly as
they do in RV, and only the RV graph underneath is faked.
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


class PanelTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._disableUpdates = False
        self.mode._inputOrderLock = False
        self.mode._previewsEnabled = False
        # updateInputs() checks this before rebuilding; the real constructor seeds it
        # from commands.loadTotal(), which object.__new__ skips.
        self.mode._progressiveLoadingInProgress = False

        self.model = QtGui.QStandardItemModel()
        self.view = QtWidgets.QTreeView()
        self.view.setModel(self.model)
        self.mode._viewModel = self.model
        self.mode._viewTreeView = self.view

        self.inputsModel = QtGui.QStandardItemModel()
        self.inputsView = QtWidgets.QListView()
        self.inputsView.setModel(self.inputsModel)
        self.mode._inputsModel = self.inputsModel
        self.mode._inputsView = self.inputsView

        self.mode._viewLabel = QtWidgets.QLabel()
        self.mode._prevViewButton = QtWidgets.QToolButton()
        self.mode._nextViewButton = QtWidgets.QToolButton()

    def tearDown(self):
        self.view.setParent(None)
        self.inputsView.setParent(None)

    def treeRow(self, text, node, parentItem=None):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        rest = [QtGui.QStandardItem(""), QtGui.QStandardItem("")]
        (parentItem or self.model.invisibleRootItem()).appendRow([item] + rest)
        return item


class TestUpdateNavUI(PanelTest):
    def setUp(self):
        super().setUp()
        for n in ("a", "b", "c"):
            self.graph.addNode(n, "RVSourceGroup")
        self.graph.viewNode = "b"
        self.graph.uiNames["b"] = "Middle"

    def test_label_shows_the_view_nodes_ui_name(self):
        self.mode.updateNavUI()
        self.assertEqual(self.mode._viewLabel.text(), "Middle")

    def test_both_buttons_enabled_when_both_neighbours_exist(self):
        self.sm.commands.previousViewNode = lambda: "a"
        self.sm.commands.nextViewNode = lambda: "c"
        self.mode.updateNavUI()
        self.assertTrue(self.mode._prevViewButton.isEnabled())
        self.assertTrue(self.mode._nextViewButton.isEnabled())

    def test_prev_disabled_at_the_start(self):
        self.sm.commands.previousViewNode = lambda: None
        self.sm.commands.nextViewNode = lambda: "c"
        self.mode.updateNavUI()
        self.assertFalse(self.mode._prevViewButton.isEnabled())
        self.assertTrue(self.mode._nextViewButton.isEnabled())

    def test_next_disabled_at_the_end(self):
        self.sm.commands.previousViewNode = lambda: "a"
        self.sm.commands.nextViewNode = lambda: None
        self.mode.updateNavUI()
        self.assertTrue(self.mode._prevViewButton.isEnabled())
        self.assertFalse(self.mode._nextViewButton.isEnabled())

    def test_no_view_node_leaves_the_label_alone(self):
        self.graph.viewNode = None
        self.mode._viewLabel.setText("sentinel")
        self.mode.updateNavUI()
        self.assertEqual(self.mode._viewLabel.text(), "sentinel")


class TestUpdateInputs(PanelTest):
    def setUp(self):
        super().setUp()
        for n in ("srcA", "srcB"):
            self.graph.addNode(n, "RVSourceGroup")
        self.graph.addNode("seq", "RVSequenceGroup", inputs=["srcA", "srcB"])
        self.graph.uiNames.update({"srcA": "Source A", "srcB": "Source B"})
        self.graph.viewNode = "seq"
        self.mode.iconForNode = lambda node: QtGui.QIcon()

    def rows(self):
        return [self.inputsModel.item(r).data(Qt.UserRole + 2)
                for r in range(self.inputsModel.rowCount())]

    def test_lists_the_nodes_inputs_in_order(self):
        self.mode.updateInputs("seq")
        self.assertEqual(self.rows(), ["srcA", "srcB"])

    def test_rebuilds_rather_than_appending(self):
        self.mode.updateInputs("seq")
        self.mode.updateInputs("seq")
        self.assertEqual(self.rows(), ["srcA", "srcB"])

    def test_follows_a_connection_change(self):
        self.mode.updateInputs("seq")
        self.graph.setNodeInputs("seq", ["srcB"])
        self.mode.updateInputs("seq")
        self.assertEqual(self.rows(), ["srcB"])

    def test_a_node_with_no_inputs_empties_the_panel(self):
        self.mode.updateInputs("seq")
        self.graph.addNode("lonely", "RVSourceGroup")
        self.mode.updateInputs("lonely")
        self.assertEqual(self.rows(), [])

    def test_rows_are_labelled_with_ui_names(self):
        self.mode.updateInputs("seq")
        texts = [self.inputsModel.item(r).text()
                 for r in range(self.inputsModel.rowCount())]
        self.assertEqual(texts, ["Source A", "Source B"])

    def test_the_order_lock_is_clear_afterwards(self):
        """Left set, every later reorder would be silently ignored."""
        self.mode.updateInputs("seq")
        self.assertFalse(self.mode._inputOrderLock)


class TestSelectViewableNode(PanelTest):
    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.viewNode = "srcB"
        self.mode.updateInputs = lambda node: None
        self.category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([self.category])
        self.a = self.treeRow("A", "srcA", self.category)
        self.b = self.treeRow("B", "srcB", self.category)

    def selected(self):
        return [self.model.itemFromIndex(i).data(Qt.UserRole + 2)
                for i in self.view.selectionModel().selectedIndexes()
                if i.column() == 0]

    def test_selects_the_row_for_the_view_node(self):
        self.mode.selectViewableNode()
        self.assertEqual(self.selected(), ["srcB"])

    def test_selecting_replaces_any_previous_selection(self):
        self.view.selectionModel().select(
            self.model.indexFromItem(self.a), QtCore.QItemSelectionModel.Select)
        self.mode.selectViewableNode()
        self.assertEqual(self.selected(), ["srcB"])

    def test_no_view_node_selects_nothing(self):
        self.graph.viewNode = None
        self.mode.selectViewableNode()
        self.assertEqual(self.selected(), [])

    def test_a_view_node_with_no_row_selects_nothing(self):
        self.graph.addNode("hidden", "RVSourceGroup")
        self.graph.viewNode = "hidden"
        self.mode.selectViewableNode()
        self.assertEqual(self.selected(), [])

    def test_it_does_not_expand_the_node_row(self):
        """The regression that produced a phantom sm_state.expandState.

        selectViewableNode scrolls to the head of mapItems(); if that head were a
        sub-component row, scrollTo would expand its parent and setItemExpandedState
        would write a property Mu never writes.
        """
        sub = QtGui.QStandardItem("media")
        sub.setData("srcB", Qt.UserRole + 2)
        sub.setData(self.sm.MediaSubComponent, Qt.UserRole + 4)
        self.b.appendRow([sub])

        self.mode.selectViewableNode()

        self.assertFalse(self.view.isExpanded(self.model.indexFromItem(self.b)),
                         "the node row must not be expanded as a side effect")


class TestRebuildInputsFromList(PanelTest):
    def setUp(self):
        super().setUp()
        for n in ("srcA", "srcB"):
            self.graph.addNode(n, "RVSourceGroup")
        self.graph.addNode("seq", "RVSequenceGroup", inputs=["srcA", "srcB"])
        self.graph.viewNode = "seq"
        self.mode.updateInputs = lambda node: None

    def putRows(self, nodes):
        self.inputsModel.clear()
        for n in nodes:
            item = QtGui.QStandardItem(n)
            item.setData(n, Qt.UserRole + 2)
            self.inputsModel.appendRow(item)

    def test_writes_the_model_order_back_to_the_graph(self):
        self.putRows(["srcB", "srcA"])
        self.mode.rebuildInputsFromList()
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["srcB", "srcA"])

    def test_dropping_a_row_removes_the_input(self):
        self.putRows(["srcA"])
        self.mode.rebuildInputsFromList()
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["srcA"])

    def test_the_lock_suppresses_the_write(self):
        self.putRows(["srcB", "srcA"])
        self.mode._inputOrderLock = True
        self.mode.rebuildInputsFromList()
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["srcA", "srcB"])

    def test_no_view_node_is_a_noop(self):
        self.putRows(["srcB"])
        self.graph.viewNode = None
        self.mode.rebuildInputsFromList()
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["srcA", "srcB"])

    def test_updates_are_re_enabled_afterwards(self):
        self.putRows(["srcA", "srcB"])
        self.mode.rebuildInputsFromList()
        self.assertFalse(self.mode._disableUpdates)


class TestInputsDeleteSlot(PanelTest):
    def setUp(self):
        super().setUp()
        for n in ("srcA", "srcB", "srcC"):
            self.graph.addNode(n, "RVSourceGroup")
        self.graph.addNode("seq", "RVSequenceGroup", inputs=["srcA", "srcB", "srcC"])
        self.graph.viewNode = "seq"
        self.mode.updateInputs = lambda node: None
        self.items = {}
        for n in ("srcA", "srcB", "srcC"):
            item = QtGui.QStandardItem(n)
            item.setData(n, Qt.UserRole + 2)
            self.inputsModel.appendRow(item)
            self.items[n] = item

    def select(self, *nodes):
        model = self.inputsView.selectionModel()
        model.clearSelection()
        for n in nodes:
            model.select(self.inputsModel.indexFromItem(self.items[n]),
                         QtCore.QItemSelectionModel.Select)

    def test_removes_the_selected_input(self):
        self.select("srcB")
        self.mode.inputsDeleteSlot(False)
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["srcA", "srcC"])

    def test_removes_several_at_once(self):
        self.select("srcA", "srcC")
        self.mode.inputsDeleteSlot(False)
        self.assertEqual(self.graph.nodeConnections("seq")[0], ["srcB"])

    def test_no_selection_changes_nothing(self):
        self.mode.inputsDeleteSlot(False)
        self.assertEqual(self.graph.nodeConnections("seq")[0],
                         ["srcA", "srcB", "srcC"])

    def test_the_nodes_themselves_are_not_deleted(self):
        """Removing an input detaches it; it must not delete the source."""
        self.select("srcB")
        self.mode.inputsDeleteSlot(False)
        self.assertTrue(self.graph.nodeExists("srcB"))


class TestSetItemExpandedState(PanelTest):
    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.mode.updateInputs = lambda node: None

    def test_a_node_row_records_expansion_against_its_parent(self):
        category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([category])
        item = self.treeRow("A", "srcA", category)

        self.mode.setItemExpandedState(self.model.indexFromItem(item), 1)

        self.assertTrue(self.sm.isExpandedInParent("srcA", ""))

    def test_collapsing_clears_it(self):
        category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([category])
        item = self.treeRow("A", "srcA", category)
        idx = self.model.indexFromItem(item)

        self.mode.setItemExpandedState(idx, 1)
        self.mode.setItemExpandedState(idx, 0)

        self.assertFalse(self.sm.isExpandedInParent("srcA", ""))

    def test_a_category_row_records_against_the_session(self):
        category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([category])

        self.mode.setItemExpandedState(self.model.indexFromItem(category), 1)

        self.assertEqual(
            self.graph.getIntProperty("rv.session.sm_view.SOURCES"), [1])

    def test_a_sub_component_row_records_against_the_hash(self):
        category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([category])
        node = self.treeRow("A", "srcA", category)
        sub = QtGui.QStandardItem("left")
        sub.setData("srcA", Qt.UserRole + 2)
        sub.setData(self.sm.ViewSubComponent, Qt.UserRole + 4)
        sub.setData("left", Qt.UserRole + 5)
        node.appendRow([sub])

        self.mode.setItemExpandedState(self.model.indexFromItem(sub), 1)

        self.assertTrue(self.sm.isSubComponentExpanded("srcA", sub))


if __name__ == "__main__":
    unittest.main()
