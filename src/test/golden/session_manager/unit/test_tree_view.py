"""Gate 5 — NodeTreeView and InputsView drag/drop policy on the port itself.

Drag and drop is the part of the package the golden scenarios reach least (they drive
the command API), so these tests carry more of the weight. They construct the real
widgets and feed them real QDragMoveEvent/QDropEvent objects rather than mocks, since
what is under test is precisely which events get ignored.
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


class TreeViewTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.model = self.sm.NodeModel(None)
        self.view = self.sm.NodeTreeView(None)
        self.view.setModel(self.model)
        self.view._viewModel = self.model

    def tearDown(self):
        self.view.setParent(None)

    def _row(self, text, node):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        return item


class TestInitialState(TreeViewTest):
    def test_starts_with_no_drop_action_and_no_paths(self):
        self.assertEqual(self.view._dropAction, Qt.IgnoreAction)
        self.assertEqual(self.view._draggedNodePaths, [])
        self.assertFalse(self.view._draggingNonFolders)

    def test_sort_timer_is_single_shot(self):
        self.assertTrue(self.view._sortTimer.isSingleShot())


class TestSortFolderChildren(TreeViewTest):
    def test_records_folder_groups_only(self):
        self.graph.addNode("folder", "RVFolderGroup")
        self.graph.addNode("seq", "RVSequenceGroup")

        self.view.sortFolderChildren("folder")
        self.view.sortFolderChildren("seq")

        self.assertEqual(self.view._sortFolders, ["folder"])

    def test_does_not_record_the_same_folder_twice(self):
        self.graph.addNode("folder", "RVFolderGroup")
        self.view.sortFolderChildren("folder")
        self.view.sortFolderChildren("folder")
        self.assertEqual(self.view._sortFolders, ["folder"])


class TestSelectedNodePaths(TreeViewTest):
    def test_path_runs_from_node_up_through_its_ancestors(self):
        category = self._row("FOLDERS", "")
        folder = self._row("Folder", "folderNode")
        child = self._row("Src", "srcNode")
        folder.appendRow([child])
        category.appendRow([folder])
        self.model.appendRow([category])

        childIndex = self.model.indexFromItem(child)
        self.view.selectionModel().select(
            childIndex, QtCore.QItemSelectionModel.Select
        )

        self.assertEqual(self.view.selectedNodePaths(), [["srcNode", "folderNode", ""]])

    def test_only_column_zero_contributes_a_path(self):
        row = [self._row("Src", "srcNode"), QtGui.QStandardItem("status")]
        self.model.appendRow(row)
        self.view.selectionModel().select(
            self.model.index(0, 1), QtCore.QItemSelectionModel.Select
        )
        self.assertEqual(self.view.selectedNodePaths(), [])

    def test_no_selection_gives_no_paths(self):
        self.model.appendRow([self._row("Src", "srcNode")])
        self.assertEqual(self.view.selectedNodePaths(), [])


class TestFilteredDraggedPaths(TreeViewTest):
    def test_applies_the_predicate(self):
        self.view._draggedNodePaths = [["a", "f"], ["b", ""], ["c", "f"]]
        got = self.view.filteredDraggedPaths(lambda p: p[1] == "f")
        self.assertEqual(got, [["a", "f"], ["c", "f"]])

    def test_empty_when_nothing_is_being_dragged(self):
        self.assertEqual(self.view.filteredDraggedPaths(lambda p: True), [])


class TestSortFolders(TreeViewTest):
    def test_assigns_sort_order_for_each_recorded_folder(self):
        self.graph.addNode("folderNode", "RVFolderGroup")
        folder = self._row("Folder", "folderNode")
        for name in ("a", "b"):
            folder.appendRow([self._row(name, name)])
        self.model.appendRow([folder])

        self.view._sortFolders = ["folderNode"]
        self.view.sortFolders()

        self.assertEqual(self.sm.sortKeyInParent("a", "folderNode"), 0)
        self.assertEqual(self.sm.sortKeyInParent("b", "folderNode"), 1)
        self.assertEqual(self.view._sortFolders, [],
                         "the pending list must be cleared after sorting")

    def test_unknown_folder_is_skipped_without_raising(self):
        self.view._sortFolders = ["notInTheModel"]
        self.view.sortFolders()
        self.assertEqual(self.view._sortFolders, [])


class TestDragMoveEvent(TreeViewTest):
    """dragMoveEvent decides which drops are legal; each rejection is a rule."""

    def _dragMove(self, pos, action=Qt.CopyAction):
        mime = QtCore.QMimeData()
        event = QtGui.QDragMoveEvent(
            pos, action, mime, Qt.LeftButton, Qt.NoModifier
        )
        event.setDropAction(action)
        event.accept()
        self.view.dragMoveEvent(event)
        return event

    def test_drop_outside_any_row_is_ignored(self):
        event = self._dragMove(QtCore.QPoint(5, 5))
        self.assertFalse(event.isAccepted(),
                         "an empty area has no item, so the drop must be ignored")

    def test_drop_on_a_non_folder_sibling_is_ignored(self):
        """Copying onto a sibling under the same parent is a reorder, not a copy."""
        self.graph.addNode("parentSeq", "RVSequenceGroup")
        self.graph.addNode("target", "RVSourceGroup")
        self.graph.connections["target"] = []
        self.graph.connections["parentSeq"] = ["target"]

        parent = self._row("Seq", "parentSeq")
        target = self._row("Target", "target")
        parent.appendRow([target])
        self.model.appendRow([parent])
        self.view.expandAll()

        # nodeConnections(target)[1] is the outputs list; make parentSeq an output.
        self.graph.nodeConnections = lambda n, t=False: (
            list(self.graph.connections.get(n, [])),
            ["parentSeq"] if n == "target" else [],
        )
        self.view._draggedNodePaths = [["dragged", "parentSeq"]]
        self.graph.addNode("dragged", "RVSourceGroup")

        rect = self.view.visualRect(self.model.indexFromItem(target))
        event = self._dragMove(rect.center())
        self.assertFalse(event.isAccepted())

    def test_drop_on_the_dragged_items_own_parent_is_ignored(self):
        self.graph.addNode("folderNode", "RVFolderGroup")
        self.graph.addNode("dragged", "RVSourceGroup")
        folder = self._row("Folder", "folderNode")
        self.model.appendRow([folder])

        self.view._draggedNodePaths = [["dragged", "folderNode"]]

        rect = self.view.visualRect(self.model.indexFromItem(folder))
        event = self._dragMove(rect.center())
        self.assertFalse(event.isAccepted(),
                         "re-dropping into the parent it already sits in is a no-op")


class TestDragEnterEvent(TreeViewTest):
    def test_self_drag_records_paths_and_flags_non_folders(self):
        self.graph.addNode("srcNode", "RVSourceGroup")
        item = self._row("Src", "srcNode")
        self.model.appendRow([item])
        self.view.selectionModel().select(
            self.model.indexFromItem(item), QtCore.QItemSelectionModel.Select
        )

        mime = QtCore.QMimeData()
        event = QtGui.QDragEnterEvent(
            QtCore.QPoint(1, 1), Qt.CopyAction, mime, Qt.LeftButton, Qt.NoModifier
        )
        # QDragEnterEvent has no source(); the view reads event.source(), so stand in.
        event.source = lambda: self.view

        self.view.dragEnterEvent(event)

        self.assertEqual(self.view._draggedNodePaths, [["srcNode"]])
        self.assertTrue(self.view._draggingNonFolders)

    def test_dragging_only_folders_leaves_the_flag_clear(self):
        self.graph.addNode("folderNode", "RVFolderGroup")
        item = self._row("Folder", "folderNode")
        self.model.appendRow([item])
        self.view.selectionModel().select(
            self.model.indexFromItem(item), QtCore.QItemSelectionModel.Select
        )

        mime = QtCore.QMimeData()
        event = QtGui.QDragEnterEvent(
            QtCore.QPoint(1, 1), Qt.CopyAction, mime, Qt.LeftButton, Qt.NoModifier
        )
        event.source = lambda: self.view
        self.view.dragEnterEvent(event)

        self.assertFalse(self.view._draggingNonFolders)

    def test_folders_category_becomes_undroppable_for_non_folder_drags(self):
        """H4: the FOLDERS section stops accepting drops when non-folders are dragged."""
        self.graph.addNode("srcNode", "RVSourceGroup")
        foldersItem = self._row("FOLDERS", "")
        self.model.appendRow([foldersItem])
        self.view._foldersItem = foldersItem

        item = self._row("Src", "srcNode")
        self.model.appendRow([item])
        self.view.selectionModel().select(
            self.model.indexFromItem(item), QtCore.QItemSelectionModel.Select
        )

        mime = QtCore.QMimeData()
        event = QtGui.QDragEnterEvent(
            QtCore.QPoint(1, 1), Qt.CopyAction, mime, Qt.LeftButton, Qt.NoModifier
        )
        event.source = lambda: self.view
        self.view.dragEnterEvent(event)

        self.assertFalse(bool(foldersItem.flags() & Qt.ItemIsDropEnabled))


class TestInputsView(TreeViewTest):
    def setUp(self):
        super().setUp()
        self.cleanupCalls = []
        self.inputs = self.sm.InputsView(
            self.view, None, dropCleanup=lambda: self.cleanupCalls.append(1)
        )

    def tearDown(self):
        self.inputs.setParent(None)
        super().tearDown()

    def test_drag_from_the_tree_is_forced_to_copy(self):
        """Recorded at the moment the port sets it.

        QAbstractItemView.dragEnterEvent() runs afterwards and may set the action
        again from the event's own proposed actions, so reading it back after the
        call would test Qt rather than the override.
        """
        mime = QtCore.QMimeData()
        event = QtGui.QDragEnterEvent(
            QtCore.QPoint(1, 1), Qt.MoveAction, mime, Qt.LeftButton, Qt.NoModifier
        )
        event.source = lambda: self.view

        seen = []
        realSet = event.setDropAction
        event.setDropAction = lambda a: (seen.append(a), realSet(a))[1]

        self.inputs.dragEnterEvent(event)

        self.assertIn(Qt.CopyAction, seen,
                      "a drag out of the tree must never move the node")

    def test_drag_from_elsewhere_keeps_its_action(self):
        mime = QtCore.QMimeData()
        event = QtGui.QDragEnterEvent(
            QtCore.QPoint(1, 1), Qt.MoveAction, mime, Qt.LeftButton, Qt.NoModifier
        )
        event.source = lambda: None
        event.setDropAction(Qt.MoveAction)

        self.inputs.dragEnterEvent(event)

        self.assertEqual(event.dropAction(), Qt.MoveAction)

    def test_drop_timer_is_single_shot(self):
        self.assertTrue(self.inputs._dropTimer.isSingleShot())


if __name__ == "__main__":
    unittest.main()
