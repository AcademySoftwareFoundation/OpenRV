"""Gate 5 — the session manager's panel slots: editors, selection, reordering.

Three families live here.

The **editor tab strip** (`addEditor`/`useEditor`/`reloadEditorTab`) is what the eleven
sibling modes push their `.ui` panels into. Each sibling has now been checked to hand
over the right widget under the right name; this is the other side of that contract,
and the part a golden can only see for view types the harness can build.

The **selection readers** (`selectedItems`, `selectedConvertedSubComponents`) turn a Qt
selection into a node list, and every destructive action — delete, folder, reorder —
is driven from that list. Selection spans all three columns of the tree, so the
column-0 filter is the difference between deleting one node and deleting it three
times.

**`reorderSelected`** is the largest piece of arithmetic in the package: it moves a
possibly-discontiguous selection up or down one row and rewrites the inputs list to
match. A drag reorder is not reproducible headlessly, so the button path is the only
way to pin it.
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


class _Event:
    def __init__(self, contents=""):
        self._contents = contents
        self.rejected = False

    def contents(self):
        return self._contents

    def reject(self):
        self.rejected = True


class _Timer:
    def __init__(self):
        self.starts = []
        self.stops = 0

    def start(self, ms):
        self.starts.append(ms)

    def stop(self):
        self.stops += 1


class SlotTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._disableUpdates = False
        self.mode._inputOrderLock = False
        self.mode._previewsEnabled = False
        self.mode._progressiveLoadingInProgress = False
        self.mode._editors = []
        self.mode._lazyUpdateTimer = _Timer()
        self.mode._lazySetInputsTimer = _Timer()

        self.model = QtGui.QStandardItemModel()
        self.view = QtWidgets.QTreeView()
        self.view.setModel(self.model)
        self.view.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.mode._viewModel = self.model
        self.mode._viewTreeView = self.view

        self.inputsModel = QtGui.QStandardItemModel()
        self.inputsView = QtWidgets.QListView()
        self.inputsView.setModel(self.inputsModel)
        self.inputsView.setSelectionMode(
            QtWidgets.QAbstractItemView.ExtendedSelection)
        self.mode._inputsModel = self.inputsModel
        self.mode._inputsView = self.inputsView

    def tearDown(self):
        self.view.setParent(None)
        self.inputsView.setParent(None)

    def treeRow(self, text, node, parentItem=None, subType=None, value=None):
        """A three-column tree row, as newNodeRow() builds it."""
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        if subType is not None:
            item.setData(subType, Qt.UserRole + 4)
        if value is not None:
            item.setData(value, Qt.UserRole + 5)
        rest = [QtGui.QStandardItem(""), QtGui.QStandardItem("")]
        (parentItem or self.model.invisibleRootItem()).appendRow([item] + rest)
        return item

    def selectTree(self, items):
        smodel = self.view.selectionModel()
        smodel.clearSelection()
        for item in items:
            smodel.select(self.model.indexFromItem(item),
                          QtCore.QItemSelectionModel.Select
                          | QtCore.QItemSelectionModel.Rows)


class TestEditorTabs(SlotTest):
    def setUp(self):
        super().setUp()
        self.tree = QtWidgets.QTreeWidget()
        self.mode._uiTreeWidget = self.tree

    def tearDown(self):
        self.tree.setParent(None)
        super().tearDown()

    def panel(self, name="Stack"):
        widget = QtWidgets.QWidget()
        self.mode.addEditor(name, widget)
        return widget

    def test_adding_an_editor_creates_a_top_level_row(self):
        self.panel()
        self.assertEqual(self.tree.topLevelItemCount(), 1)
        self.assertEqual(self.tree.topLevelItem(0).text(0), "Stack")

    def test_the_widget_is_hosted_under_the_row(self):
        """The panel goes on a child row, not the labelled one, so the label
        stays visible above it."""
        widget = self.panel()
        item = self.tree.topLevelItem(0)
        self.assertEqual(item.childCount(), 1)
        self.assertIs(self.tree.itemWidget(item.child(0), 0), widget)

    def test_the_row_starts_expanded(self):
        self.panel()
        self.assertTrue(self.tree.topLevelItem(0).isExpanded())

    def test_the_widget_fills_its_background(self):
        """Without this the panel is transparent over the tree's alternating rows."""
        widget = self.panel()
        self.assertTrue(widget.autoFillBackground())

    def test_the_label_row_is_not_selectable(self):
        self.panel()
        self.assertEqual(self.tree.topLevelItem(0).flags(), Qt.ItemIsEnabled)

    def test_each_editor_is_remembered(self):
        self.panel("Stack")
        self.panel("Composite Function")
        self.assertEqual([e.text(0) for e in self.mode._editors],
                         ["Stack", "Composite Function"])

    def test_use_editor_unhides_only_the_named_one(self):
        self.panel("Stack")
        self.panel("Composite Function")
        for e in self.mode._editors:
            e.setHidden(True)

        self.mode.useEditor("Composite Function")

        hidden = {e.text(0): e.isHidden() for e in self.mode._editors}
        self.assertEqual(hidden, {"Stack": True, "Composite Function": False})

    def test_use_editor_with_an_unknown_name_shows_nothing(self):
        self.panel("Stack")
        self.mode._editors[0].setHidden(True)
        self.mode.useEditor("Nonexistent")
        self.assertTrue(self.mode._editors[0].isHidden())

    def test_reload_hides_everything_and_reasks_the_siblings(self):
        """Switching a folder's view type has to swap one editor for another; the
        siblings answer the event and unhide themselves."""
        self.panel("Stack")
        self.panel("Layout")
        self.graph.addNode("folder", "RVFolderGroup")
        self.graph.viewNode = "folder"

        self.mode.reloadEditorTab()

        self.assertTrue(all(e.isHidden() for e in self.mode._editors))
        self.assertIn(("session-manager-load-ui", "folder"), self.graph.events)


class TestSelectionReaders(SlotTest):
    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.a = self.treeRow("A", "srcA")
        self.b = self.treeRow("B", "srcB")

    def test_no_selection_reads_as_empty(self):
        self.assertEqual(self.mode.selectedItems(), [])
        self.assertEqual(self.mode.selectedConvertedSubComponents(), [])

    def test_one_row_reads_as_one_item_not_three(self):
        """Selection spans all three columns; without the column filter a delete
        would run three times on the same node."""
        self.selectTree([self.a])
        self.assertEqual(len(self.mode.selectedItems()), 1)

    def test_the_item_returned_is_the_name_column(self):
        self.selectTree([self.a])
        self.assertEqual(self.sm.itemNode(self.mode.selectedItems()[0]), "srcA")

    def test_several_rows_read_in_order(self):
        self.selectTree([self.a, self.b])
        self.assertEqual([self.sm.itemNode(i) for i in self.mode.selectedItems()],
                         ["srcA", "srcB"])

    def test_converted_subcomponents_returns_plain_nodes_unchanged(self):
        self.selectTree([self.a, self.b])
        self.assertEqual(self.mode.selectedConvertedSubComponents(),
                         ["srcA", "srcB"])

    def test_a_row_for_a_deleted_node_is_skipped(self):
        """A row can outlive its node between a delete and the next updateTree."""
        ghost = self.treeRow("Gone", "noSuchNode")
        self.selectTree([self.a, ghost])
        self.assertEqual(self.mode.selectedConvertedSubComponents(), ["srcA"])

    def test_a_subcomponent_row_is_converted_to_a_source(self):
        """Acting on a layer row has to act on a real node, not on the row."""
        sub = self.treeRow("layer", "srcA", parentItem=self.a,
                           subType=self.sm.LayerSubComponent, value="R")
        self.mode.sourceFromSubComponent = lambda item, node: "convertedNode"

        self.selectTree([sub])

        self.assertEqual(self.mode.selectedConvertedSubComponents(),
                         ["convertedNode"])

    def test_the_conversion_runs_with_updates_disabled(self):
        """It creates a node, which would otherwise re-enter updateTree and
        invalidate the very items being iterated."""
        seen = []
        sub = self.treeRow("layer", "srcA", parentItem=self.a,
                           subType=self.sm.LayerSubComponent, value="R")
        self.mode.sourceFromSubComponent = lambda item, node: (
            seen.append(self.mode._disableUpdates) or "converted")

        self.selectTree([sub])
        self.mode.selectedConvertedSubComponents()

        self.assertEqual(seen, [True])
        self.assertFalse(self.mode._disableUpdates, "the flag must be cleared again")


class TestSelectInputsRange(SlotTest):
    def setUp(self):
        super().setUp()
        for name in ("a", "b", "c", "d"):
            item = QtGui.QStandardItem(name)
            item.setData(name, Qt.UserRole + 2)
            self.inputsModel.appendRow(item)

    def selectedRows(self):
        return sorted(i.row()
                      for i in self.inputsView.selectionModel().selectedIndexes())

    def test_it_selects_the_rows_it_is_given(self):
        self.mode.selectInputsRange([1, 2])
        self.assertEqual(self.selectedRows(), [1, 2])

    def test_a_discontiguous_range_is_selected_as_given(self):
        """reorderSelected reuses this to restore a gapped selection after a move."""
        self.mode.selectInputsRange([0, 3])
        self.assertEqual(self.selectedRows(), [0, 3])

    def test_it_adds_to_the_existing_selection(self):
        self.mode.selectInputsRange([0])
        self.mode.selectInputsRange([2])
        self.assertEqual(self.selectedRows(), [0, 2])

    def test_an_empty_list_selects_nothing(self):
        self.mode.selectInputsRange([])
        self.assertEqual(self.selectedRows(), [])


class TestReorderSelected(SlotTest):
    def setUp(self):
        super().setUp()
        self.graph.addNode("a", "RVSourceGroup")
        self.graph.addNode("b", "RVSourceGroup")
        self.graph.addNode("c", "RVSourceGroup")
        self.graph.addNode("d", "RVSourceGroup")
        self.graph.addNode("seq", "RVSequenceGroup",
                           inputs=["a", "b", "c", "d"])
        self.graph.viewNode = "seq"

        for name in ("a", "b", "c", "d"):
            item = QtGui.QStandardItem(name)
            item.setData(name, Qt.UserRole + 2)
            self.inputsModel.appendRow(item)

    def selectRows(self, rows):
        smodel = self.inputsView.selectionModel()
        smodel.clearSelection()
        for row in rows:
            smodel.select(self.inputsModel.index(row, 0),
                          QtCore.QItemSelectionModel.Select)

    def inputs(self):
        return self.graph.nodeConnections("seq")[0]

    def test_moving_one_row_up_swaps_it_with_its_neighbour(self):
        self.selectRows([1])
        self.mode.reorderSelected(True, False)
        self.assertEqual(self.inputs(), ["b", "a", "c", "d"])

    def test_moving_one_row_down_swaps_it_the_other_way(self):
        self.selectRows([1])
        self.mode.reorderSelected(False, False)
        self.assertEqual(self.inputs(), ["a", "c", "b", "d"])

    def test_a_contiguous_block_moves_together(self):
        self.selectRows([1, 2])
        self.mode.reorderSelected(True, False)
        self.assertEqual(self.inputs(), ["b", "c", "a", "d"])

    def test_the_top_row_cannot_move_up(self):
        self.selectRows([0])
        self.mode.reorderSelected(True, False)
        self.assertEqual(self.inputs(), ["a", "b", "c", "d"])

    def test_the_bottom_row_cannot_move_down(self):
        self.selectRows([3])
        self.mode.reorderSelected(False, False)
        self.assertEqual(self.inputs(), ["a", "b", "c", "d"])

    def test_nothing_selected_is_a_noop(self):
        self.mode.reorderSelected(True, False)
        self.assertEqual(self.inputs(), ["a", "b", "c", "d"])

    def test_the_destination_row_is_selected(self):
        """Otherwise a second click on the button moves a different row. In RV the
        model is rebuilt by the inputs-changed event before this runs, which clears
        the old selection; here only the addition is observable."""
        self.selectRows([2])
        self.mode.reorderSelected(True, False)
        rows = {i.row()
                for i in self.inputsView.selectionModel().selectedIndexes()}
        self.assertIn(1, rows)

    def test_the_model_is_left_to_the_inputs_changed_event(self):
        """reorderSelected rewrites the graph only; updateInputs rebuilds the rows
        when RV reports the change back. Rebuilding here as well would double it."""
        self.selectRows([1])
        self.mode.reorderSelected(True, False)
        self.assertEqual(
            [self.inputsModel.item(r).data(Qt.UserRole + 2)
             for r in range(self.inputsModel.rowCount())],
            ["a", "b", "c", "d"])


class TestItemPressed(SlotTest):
    """Clicking the radio column of a sub-component row switches the view to it."""

    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.viewed = []
        self.mode.viewByIndex = lambda index, model: self.viewed.append(
            model.itemFromIndex(index).data(Qt.UserRole + 2))
        self.parent = self.treeRow("A", "srcA")

    def pressed(self, item, column):
        index = self.model.indexFromItem(item).sibling(item.row(), column)
        self.mode.itemPressed(index, self.model)

    def test_the_radio_column_of_a_layer_row_switches_the_view(self):
        sub = self.treeRow("R", "srcA", parentItem=self.parent,
                           subType=self.sm.LayerSubComponent, value="R")
        self.pressed(sub, 1)
        self.assertEqual(self.viewed, ["srcA"])

    def test_the_name_column_does_not(self):
        """Clicking the name starts a rename; it must not also change the view."""
        sub = self.treeRow("R", "srcA", parentItem=self.parent,
                           subType=self.sm.LayerSubComponent, value="R")
        self.pressed(sub, 0)
        self.assertEqual(self.viewed, [])

    def test_a_plain_node_row_does_not(self):
        self.pressed(self.parent, 1)
        self.assertEqual(self.viewed, [])

    def test_a_media_row_does_not(self):
        """The media row is the file heading, not a selectable component."""
        sub = self.treeRow("movie.mov", "srcA", parentItem=self.parent,
                           subType=self.sm.MediaSubComponent, value="movie.mov")
        self.pressed(sub, 1)
        self.assertEqual(self.viewed, [])

    def test_a_view_row_switches_the_view(self):
        sub = self.treeRow("left", "srcA", parentItem=self.parent,
                           subType=self.sm.ViewSubComponent, value="left")
        self.pressed(sub, 1)
        self.assertEqual(self.viewed, ["srcA"])


class TestViewSelectionChanged(SlotTest):
    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.viewed = []
        self.mode.viewByIndex = lambda index, model: self.viewed.append(
            model.itemFromIndex(index).data(Qt.UserRole + 2))

        self.category = QtGui.QStandardItem("SOURCES")
        self.model.appendRow([self.category])
        self.a = self.treeRow("A", "srcA", parentItem=self.category)
        self.b = self.treeRow("B", "srcB", parentItem=self.category)

    def changeTo(self, item):
        smodel = self.view.selectionModel()
        smodel.clearSelection()
        selection = QtCore.QItemSelection(self.model.indexFromItem(item),
                                          self.model.indexFromItem(item))
        smodel.select(selection, QtCore.QItemSelectionModel.Select
                      | QtCore.QItemSelectionModel.Rows)
        self.mode.viewSelectionChanged(selection, QtCore.QItemSelection())

    def test_selecting_a_top_level_row_views_it(self):
        self.changeTo(self.b)
        self.assertEqual(self.viewed, ["srcB"])

    def test_a_nested_row_does_not_change_the_view(self):
        """Sub-component rows are two levels down and have their own radio column;
        merely selecting one must not switch the view."""
        sub = self.treeRow("R", "srcA", parentItem=self.a,
                           subType=self.sm.LayerSubComponent, value="R")
        self.changeTo(sub)
        self.assertEqual(self.viewed, [])

    def test_an_empty_change_is_a_noop(self):
        self.mode.viewSelectionChanged(QtCore.QItemSelection(),
                                       QtCore.QItemSelection())
        self.assertEqual(self.viewed, [])

    def test_the_select_current_view_button_reselects_instead(self):
        calls = []
        self.mode.selectViewableNode = lambda: calls.append(1)
        self.mode.selectCurrentViewSlot(False)
        self.assertEqual(calls, [1])


class TestDeleteViewableSlot(SlotTest):
    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.viewNode = "srcA"

    def row(self, text, node, parent=None):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        if parent is not None:
            item.setData(parent, Qt.UserRole + 1)
        rest = [QtGui.QStandardItem(""), QtGui.QStandardItem("")]
        self.model.invisibleRootItem().appendRow([item] + rest)
        return item

    def test_it_deletes_the_selected_node(self):
        item = self.row("A", "srcA")
        self.selectTree([item])
        self.mode.deleteViewableSlot(False)
        self.assertNotIn("srcA", self.graph.nodes)

    def test_it_deletes_every_selected_node(self):
        a = self.row("A", "srcA")
        b = self.row("B", "srcB")
        self.selectTree([a, b])
        self.mode.deleteViewableSlot(False)
        self.assertEqual(self.graph.deleted, ["srcA", "srcB"])

    def test_it_queues_a_rebuild(self):
        self.selectTree([self.row("A", "srcA")])
        self.mode.deleteViewableSlot(False)
        self.assertEqual(self.mode._lazyUpdateTimer.starts, [0])

    def test_a_node_in_two_folders_is_only_unlinked_from_this_one(self):
        """Deleting it outright would empty the other folder too."""
        self.graph.addNode("f1", "RVFolderGroup", inputs=["srcA"])
        self.graph.addNode("f2", "RVFolderGroup", inputs=["srcA"])
        item = self.row("A", "srcA", parent="f1")

        self.selectTree([item])
        self.mode.deleteViewableSlot(False)

        self.assertIn("srcA", self.graph.nodes)
        self.assertEqual(self.graph.nodeConnections("f1")[0], [])
        self.assertEqual(self.graph.nodeConnections("f2")[0], ["srcA"])

    def test_a_node_in_one_folder_is_deleted_outright(self):
        self.graph.addNode("f1", "RVFolderGroup", inputs=["srcA"])
        item = self.row("A", "srcA", parent="f1")

        self.selectTree([item])
        self.mode.deleteViewableSlot(False)

        self.assertNotIn("srcA", self.graph.nodes)

    def test_a_delete_that_throws_does_not_abort_the_rest(self):
        def boom(node):
            raise RuntimeError("node is in use")

        self.sm.commands.deleteNode = boom
        self.selectTree([self.row("A", "srcA")])
        self.mode.deleteViewableSlot(False)
        self.assertFalse(self.mode._disableUpdates,
                         "the update freeze must be lifted even on failure")


class TestPrintRows(SlotTest):
    """Mu's debug dump of the inputs model; the event wrapper keeps it reachable."""

    def setUp(self):
        super().setUp()
        for name in ("a", "b"):
            self.inputsModel.appendRow(QtGui.QStandardItem(name))

    def capture(self, call):
        import contextlib
        import io

        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            call()
        return buffer.getvalue()

    def test_it_prints_one_line_per_row(self):
        out = self.capture(self.mode.printRows)
        self.assertIn("row 0 -> a", out)
        self.assertIn("row 1 -> b", out)

    def test_an_empty_model_prints_no_rows(self):
        self.inputsModel.clear()
        out = self.capture(self.mode.printRows)
        self.assertNotIn("row ", out)

    def test_the_event_wrapper_prints_the_same_thing(self):
        out = self.capture(lambda: self.mode.showRows(_Event()))
        self.assertIn("row 0 -> a", out)


class TestColorSlots(SlotTest):
    """The Create Image dialog's colour swatch."""

    def setUp(self):
        super().setUp()
        self.mode._cidColorButton = QtWidgets.QPushButton()
        self.mode._cidColor = QtGui.QColor("white")
        self.addCleanup(self.mode._cidColorButton.setParent, None)

    def test_a_new_colour_is_remembered(self):
        self.mode.newColorSlot(QtGui.QColor(10, 20, 30))
        self.assertEqual(self.mode._cidColor, QtGui.QColor(10, 20, 30))

    def test_a_new_colour_repaints_the_swatch(self):
        self.mode.newColorSlot(QtGui.QColor(10, 20, 30))
        self.assertIn("rgb(10,20,30)", self.mode._cidColorButton.styleSheet())

    def test_choosing_opens_the_dialog_on_the_current_colour(self):
        opened = []
        current = []

        class _Dialog:
            def open(self):
                opened.append(1)

            def setCurrentColor(self, color):
                current.append(color)

        self.mode._colorDialog = _Dialog()
        self.mode.chooseColorSlot(False)

        self.assertEqual(opened, [1])
        self.assertEqual(current, [QtGui.QColor("white")])


class TestConstructionIsNotUnitTestable(SlotTest):
    """`SessionManagerMode(name)` and `createMode()` have no unit test on purpose.

    Constructing the mode parents a dock widget to the session window and then
    segfaults under the offscreen platform (exit 139, reproducible) somewhere in the
    dock/WebEngine path. Every other method is reachable with `object.__new__`, so
    the constructor is the one symbol that has to be pinned by the golden gates
    instead: gate 3 launches RV with the package loaded, and all 38 scenarios drive
    a constructed mode.

    What can be checked here is the part of the contract the goldens cannot see —
    that the factory RV's loader calls exists and names the class it is supposed to.
    """

    def test_the_module_exposes_the_factory_rv_calls(self):
        self.assertTrue(callable(self.sm.createMode))

    def test_the_factory_builds_the_session_manager_mode(self):
        import inspect

        source = inspect.getsource(self.sm.createMode)
        self.assertIn("SessionManagerMode", source)

    def test_the_mode_accessor_is_unset_until_one_is_constructed(self):
        """theMode() is how the sibling modes and the Mu callers reach it."""
        self.sm._theMode = None
        self.assertIsNone(self.sm.theMode())


if __name__ == "__main__":
    unittest.main()
