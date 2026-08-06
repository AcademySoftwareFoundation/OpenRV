"""Gate 5 — the two dialogs, the folder actions, and the drop handlers.

The **Create Image** and **New Node by Type** dialogs are modal, so a golden scenario
cannot get past them: `COVERAGE.md` records both under headless limitations and pins
only the settings they read. What is not pinned there is the part that runs *before*
the dialog appears — building it, finding its widgets, and switching its fields and
artwork for each of the seven image types the Add menu offers. That is all reachable
here, because `loadUIFile` and the `.ui` files work headlessly; only `exec` does not.

**`newFolderSlot`** is the Add ▸ Folder family. One slot serves three menu entries via
its `which` argument — new empty folder, folder from the selection, folder from a copy
of the selection — and the difference between them is whether the originals are
unlinked from where they were. Getting `which` wrong loses nodes from the session.

**`dropEvent`** is the tail of a drag: it records which drop action Qt chose so
`viewItemChanged` can tell a move from a copy, then clears it again. A leaked action
makes the *next* rename take the move branch and detach the node from its folder.
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


class _Timer:
    def __init__(self):
        self.starts = []
        self.stops = 0

    def start(self, ms):
        self.starts.append(ms)

    def stop(self):
        self.stops += 1


class DialogTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._disableUpdates = False
        self.mode._previewsEnabled = False
        self.mode._newNodeDialog = None
        self.mode._createImageDialog = None
        self.mode._lazyUpdateTimer = _Timer()
        self.mode._darkUI = False
        self.mode._typeIcons = []

        self.model = QtGui.QStandardItemModel()
        self.view = self.sm.NodeTreeView(None)
        self.view.setModel(self.model)
        self.mode._viewModel = self.model
        self.mode._viewTreeView = self.view
        #  The tree view keeps its own handle on the model; selectedNodePaths and
        #  filteredDraggedPaths read rows through it, not through the mode.
        self.view._viewModel = self.model

    def tearDown(self):
        for name in ("_newNodeDialog", "_createImageDialog"):
            dialog = getattr(self.mode, name, None)
            if dialog is not None:
                dialog.setParent(None)
        self.view.setParent(None)


class TestNewNodeByTypeDialog(DialogTest):
    TYPES = ["RVColor", "RVSequenceGroup", "RVStackGroup"]

    def setUp(self):
        super().setUp()
        self.sm.commands.nodeTypes = lambda userVisible=True: list(self.TYPES)

    def test_it_builds_the_dialog_on_first_use(self):
        self.mode.addNodeByTypeName()
        self.assertIsNotNone(self.mode._newNodeDialog)

    def test_the_combo_is_found_and_filled_from_node_types(self):
        self.mode.addNodeByTypeName()
        combo = self.mode._nodeTypeCombo
        self.assertIsNotNone(combo)
        self.assertEqual([combo.itemText(i) for i in range(combo.count())],
                         self.TYPES)

    def test_the_dialog_is_built_once(self):
        """Rebuilding would reconnect accepted() and create two nodes per click."""
        self.mode.addNodeByTypeName()
        first = self.mode._newNodeDialog
        self.mode.addNodeByTypeName()
        self.assertIs(self.mode._newNodeDialog, first)

    def test_the_type_list_is_not_appended_to_on_reuse(self):
        self.mode.addNodeByTypeName()
        self.mode.addNodeByTypeName()
        self.assertEqual(self.mode._nodeTypeCombo.count(), len(self.TYPES))

    def test_accepting_creates_the_selected_type(self):
        made = []
        self.mode.addNodeOfType = lambda t: made.append(t)
        self.mode.addNodeByTypeName()
        self.mode._nodeTypeCombo.setCurrentIndex(1)

        self.mode._newNodeDialog.accepted.emit()

        self.assertEqual(made, ["RVSequenceGroup"])

    def test_rejecting_creates_nothing(self):
        made = []
        self.mode.addNodeOfType = lambda t: made.append(t)
        self.mode.addNodeByTypeName()

        self.mode._newNodeDialog.rejected.emit()

        self.assertEqual(made, [])

    def test_the_dialog_is_shown(self):
        self.mode.addNodeByTypeName()
        self.assertFalse(self.mode._newNodeDialog.isHidden())


class TestCreateImageDialog(DialogTest):
    """`addMovieProc` reuses one dialog for all seven Add ▸ Image entries."""

    SPECS = {
        "srgbcolorchart": "SRGBMacbethColorChart",
        "acescolorchart": "ACESMacbethColorChart",
        "smptebars": "SMTPEColorBars",
        "blank": "Blank",
    }

    def open(self, spec="solid,%s.movieproc"):
        self.mode.addMovieProc(spec)
        return self.mode._createImageDialog

    def test_it_builds_the_dialog_on_first_use(self):
        self.assertIsNotNone(self.open())

    def test_every_field_is_found_in_the_ui(self):
        self.open()
        for name in ("_cidWidth", "_cidHeight", "_cidFPS", "_cidLength",
                     "_cidPic", "_cidGroupBox", "_cidColorButton",
                     "_cidColorLabel"):
            with self.subTest(widget=name):
                self.assertIsNotNone(getattr(self.mode, name))

    def test_the_fps_field_defaults_from_the_general_setting(self):
        self.graph.settings[("General", "fps")] = 48.0
        self.open()
        self.assertEqual(self.mode._cidFPS.text(), "48")

    def test_the_fps_field_falls_back_to_24(self):
        self.open()
        self.assertEqual(self.mode._cidFPS.text(), "24")

    def test_the_dialog_is_built_once(self):
        first = self.open()
        self.assertIs(self.open(), first)

    def test_each_spec_names_the_source_it_will_create(self):
        for spec, name in self.SPECS.items():
            with self.subTest(spec=spec):
                self.open("%s,%%s.movieproc" % spec)
                self.assertEqual(self.mode._cidName, name)

    def test_the_fixed_charts_hide_the_colour_picker(self):
        """Their colour is defined by the chart, so offering one would mislead."""
        for spec in self.SPECS:
            with self.subTest(spec=spec):
                self.open("%s,%%s.movieproc" % spec)
                self.assertFalse(self.mode._cidColorButton.isVisibleTo(
                    self.mode._createImageDialog))

    def test_a_solid_colour_offers_the_colour_picker(self):
        self.open("solid,%s.movieproc")
        self.assertTrue(self.mode._cidColorButton.isVisibleTo(
            self.mode._createImageDialog))

    def test_a_blank_source_hides_the_size_fields(self):
        self.open("blank,%s.movieproc")
        self.assertFalse(self.mode._cidWidth.isVisibleTo(
            self.mode._createImageDialog))

    def test_accepting_adds_a_source_named_for_the_spec(self):
        self.open("smptebars,%s.movieproc")
        before = set(self.graph.nodes)

        self.mode._createImageDialog.accepted.emit()

        made = set(self.graph.nodes) - before
        self.assertTrue(made)
        self.assertIn("SMTPEColorBars", self.graph.uiNames.values())

    def test_the_movieproc_carries_the_fields_from_the_dialog(self):
        seen = []
        self.sm.commands.addSourceVerbose = lambda media, tag="": (
            seen.append(media[0]) or self.graph.addSourceVerbose(media))

        self.open("solid,%s.movieproc")
        self.mode._cidWidth.setText("1280")
        self.mode._cidHeight.setText("720")
        self.mode._cidLength.setText("50")
        self.mode._cidFPS.setText("30")
        self.mode._cidColor = QtGui.QColor(255, 0, 0)

        self.mode._createImageDialog.accepted.emit()

        self.assertEqual(len(seen), 1)
        self.assertIn("width=1280", seen[0])
        self.assertIn("height=720", seen[0])
        self.assertIn("fps=30", seen[0])
        self.assertIn("end=50", seen[0])
        self.assertIn("red=1", seen[0])
        self.assertIn("green=0", seen[0])

    def test_the_colour_button_opens_the_picker(self):
        opened = []

        class _Dialog:
            def open(self):
                opened.append(1)

            def setCurrentColor(self, color):
                pass

        self.open("solid,%s.movieproc")
        self.mode._colorDialog = _Dialog()
        self.mode._cidColorButton.setEnabled(True)
        self.mode._cidColorButton.click()

        self.assertEqual(opened, [1])


class TestNewFolderSlot(DialogTest):
    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA")
        self.graph.addSourceGroup("srcB")
        self.graph.viewNode = "srcA"
        self.mode.renameByType = lambda node, inputs: None

        #  updateTree() files every node row under a category heading, and the
        #  heading carries no node. selectedNodePaths() walks up to it, so a
        #  top-level source has the two-element path ["srcA", ""] — which is what
        #  newFolderSlot's `first[1]` reads. Rows parented straight to the root
        #  would give a one-element path and index out of range, in Mu as well.
        self.category = QtGui.QStandardItem("SOURCES")
        self.category.setData("", Qt.UserRole + 2)
        self.model.invisibleRootItem().appendRow(
            [self.category, QtGui.QStandardItem(""), QtGui.QStandardItem("")])

    def row(self, text, node, parent=""):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        item.setData(parent, Qt.UserRole + 1)
        rest = [QtGui.QStandardItem(""), QtGui.QStandardItem("")]
        host = self.category
        if parent:
            host = self.folderRow(parent)
        host.appendRow([item] + rest)
        return item

    def folderRow(self, node):
        """The row for a folder, created under the heading on first use."""
        existing = self.sm.itemOfNode(self.model, node)
        if existing is not None:
            return existing
        item = QtGui.QStandardItem(node)
        item.setData(node, Qt.UserRole + 2)
        item.setData("", Qt.UserRole + 1)
        self.category.appendRow(
            [item, QtGui.QStandardItem(""), QtGui.QStandardItem("")])
        return item

    def select(self, items):
        smodel = self.view.selectionModel()
        smodel.clearSelection()
        for item in items:
            smodel.select(self.model.indexFromItem(item),
                          QtCore.QItemSelectionModel.Select
                          | QtCore.QItemSelectionModel.Rows)

    def folders(self):
        return [n for n in self.graph.nodes
                if self.graph.nodeType(n) == "RVFolderGroup"]

    def test_an_empty_folder_is_created_with_nothing_selected(self):
        self.mode.newFolderSlot(False, 0)
        self.assertEqual(len(self.folders()), 1)
        self.assertEqual(self.graph.nodeConnections(self.folders()[0])[0], [])

    def test_which_1_makes_an_empty_folder_even_with_a_selection(self):
        """Add ▸ Folder always makes an empty one; the other two take the
        selection."""
        self.select([self.row("A", "srcA")])
        self.mode.newFolderSlot(False, 1)
        self.assertEqual(self.graph.nodeConnections(self.folders()[0])[0], [])

    def test_which_0_puts_the_selection_in_the_folder(self):
        self.select([self.row("A", "srcA"), self.row("B", "srcB")])
        self.mode.newFolderSlot(False, 0)
        self.assertEqual(self.graph.nodeConnections(self.folders()[0])[0],
                         ["srcA", "srcB"])

    def test_which_0_leaves_the_originals_where_they_were(self):
        """"Folder from copy": the sources stay in their old parent as well."""
        self.graph.addNode("old", "RVFolderGroup", inputs=["srcA"])
        self.select([self.row("A", "srcA", parent="old")])

        self.mode.newFolderSlot(False, 0)

        self.assertIn("srcA", self.graph.nodeConnections("old")[0])

    def test_which_2_unlinks_the_originals(self):
        """"Folder from selection": the sources move rather than being copied."""
        self.graph.addNode("old", "RVFolderGroup", inputs=["srcA"])
        self.select([self.row("A", "srcA", parent="old")])

        self.mode.newFolderSlot(False, 2)

        self.assertNotIn("srcA", self.graph.nodeConnections("old")[0])

    def test_the_new_folder_takes_the_place_of_the_first_selection(self):
        self.graph.addNode("old", "RVFolderGroup", inputs=["srcA"])
        self.select([self.row("A", "srcA", parent="old")])

        self.mode.newFolderSlot(False, 2)

        folder = [f for f in self.folders() if f != "old"][0]
        self.assertIn(folder, self.graph.nodeConnections("old")[0])

    def test_the_new_folder_becomes_the_view(self):
        self.select([self.row("A", "srcA")])
        self.mode.newFolderSlot(False, 0)
        self.assertEqual(self.graph.viewNode, self.folders()[0])

    def test_an_empty_folder_does_not_change_the_view(self):
        self.mode.newFolderSlot(False, 0)
        self.assertEqual(self.graph.viewNode, "srcA")

    def test_the_folder_is_renamed_by_type(self):
        renames = []
        self.mode.renameByType = lambda node, inputs: renames.append((node, inputs))
        self.select([self.row("A", "srcA")])

        self.mode.newFolderSlot(False, 0)

        self.assertEqual(renames, [(self.folders()[0], ["srcA"])])

    def test_an_empty_folder_is_renamed_with_no_inputs(self):
        renames = []
        self.mode.renameByType = lambda node, inputs: renames.append(inputs)
        self.select([self.row("A", "srcA")])

        self.mode.newFolderSlot(False, 1)

        self.assertEqual(renames, [[]])

    def test_a_rejected_connection_deletes_the_folder_again(self):
        """setInputs fails on a cycle; a half-made folder must not be left behind."""
        self.sm.commands.testNodeInputs = lambda node, inputs: "would cycle"
        self.select([self.row("A", "srcA")])

        self.mode.newFolderSlot(False, 0)

        self.assertEqual(self.folders(), [])

    def test_the_update_freeze_is_lifted_afterwards(self):
        self.select([self.row("A", "srcA")])
        self.mode.newFolderSlot(False, 0)
        self.assertFalse(self.mode._disableUpdates)


class TestViewItemChanged(DialogTest):
    """One signal, three meanings: a rename, a drag-copy, or a drag-move."""

    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA")
        self.graph.addNode("folder", "RVFolderGroup")
        self.graph.addNode("other", "RVFolderGroup")
        self.graph.viewNode = "srcA"
        self.view._dropAction = Qt.IgnoreAction
        self.view._draggedNodePaths = []
        self.view._sortFolders = []

        self.category = QtGui.QStandardItem("SOURCES")
        self.category.setData("", Qt.UserRole + 2)
        self.model.invisibleRootItem().appendRow(
            [self.category, QtGui.QStandardItem(""), QtGui.QStandardItem("")])

    def row(self, text, node, parent=""):
        """A tree row. `parent` names a folder row to nest it under: the method
        reads the new parent off the item's position in the tree, not off a role,
        because that position is what the drop has just changed."""
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        item.setData(parent, Qt.UserRole + 1)
        item.setData(self.sm.NotASubComponent, Qt.UserRole + 4)
        rest = [QtGui.QStandardItem(""), QtGui.QStandardItem("")]
        host = self.category
        if parent:
            host = QtGui.QStandardItem(parent)
            host.setData(parent, Qt.UserRole + 2)
            self.category.appendRow(
                [host, QtGui.QStandardItem(""), QtGui.QStandardItem("")])
        host.appendRow([item] + rest)
        return item

    def test_an_edit_outside_a_drag_renames_the_node(self):
        item = self.row("New Name", "srcA")
        self.mode.viewItemChanged(item)
        self.assertEqual(self.graph.uiName("srcA"), "New Name")

    def test_a_rename_runs_with_updates_disabled(self):
        """The rename fires a property change that would rebuild the tree and
        destroy the item Qt is still editing."""
        seen = []
        item = self.row("New Name", "srcA")
        self.graph.setUIName = lambda node, name: seen.append(
            self.mode._disableUpdates)
        self.sm.extra_commands.setUIName = self.graph.setUIName

        self.mode.viewItemChanged(item)

        self.assertEqual(seen, [True])
        self.assertFalse(self.mode._disableUpdates)

    def test_a_rename_that_fails_still_lifts_the_freeze(self):
        def boom(node, name):
            raise RuntimeError("read-only")

        self.sm.extra_commands.setUIName = boom
        self.mode.viewItemChanged(self.row("New Name", "srcA"))
        self.assertFalse(self.mode._disableUpdates)

    def test_a_copy_drop_links_the_node_into_its_new_parent(self):
        item = self.row("A", "srcA", parent="folder")
        self.view._dropAction = Qt.CopyAction

        self.mode.viewItemChanged(item)

        self.assertIn("srcA", self.graph.nodeConnections("folder")[0])

    def test_a_copy_drop_does_not_rename_anything(self):
        item = self.row("Different", "srcA", parent="folder")
        self.view._dropAction = Qt.CopyAction

        self.mode.viewItemChanged(item)

        self.assertEqual(self.graph.uiName("srcA"), "srcA")

    def test_a_copy_drop_twice_does_not_double_the_input(self):
        item = self.row("A", "srcA", parent="folder")
        self.view._dropAction = Qt.CopyAction

        self.mode.viewItemChanged(item)
        self.mode.viewItemChanged(item)

        self.assertEqual(self.graph.nodeConnections("folder")[0], ["srcA"])

    def test_a_move_drop_unlinks_the_old_parent(self):
        self.graph.setNodeInputs("other", ["srcA"])
        item = self.row("A", "srcA", parent="folder")
        self.view._dropAction = Qt.MoveAction
        self.view._draggedNodePaths = [["srcA", "other"]]

        self.mode.viewItemChanged(item)

        self.assertIn("srcA", self.graph.nodeConnections("folder")[0])
        self.assertEqual(self.graph.nodeConnections("other")[0], [])

    def test_a_move_onto_the_same_parent_keeps_the_link(self):
        """A reorder within one folder is reported as a move to the same place."""
        self.graph.setNodeInputs("folder", ["srcA"])
        item = self.row("A", "srcA", parent="folder")
        self.view._dropAction = Qt.MoveAction
        self.view._draggedNodePaths = [["srcA", "folder"]]

        self.mode.viewItemChanged(item)

        self.assertEqual(self.graph.nodeConnections("folder")[0], ["srcA"])

    def test_a_move_to_the_top_level_only_unlinks(self):
        self.graph.setNodeInputs("other", ["srcA"])
        item = self.row("A", "srcA", parent="")
        self.view._dropAction = Qt.MoveAction
        self.view._draggedNodePaths = [["srcA", "other"]]

        self.mode.viewItemChanged(item)

        self.assertEqual(self.graph.nodeConnections("other")[0], [])


class TestDropEvent(DialogTest):
    """The tail of a drag: what the tree records for viewItemChanged to read."""

    class _Drop:
        def __init__(self, action):
            self._action = action

        def dropAction(self):
            return self._action

    def setUp(self):
        super().setUp()
        self.view._draggedNodePaths = [["srcA", "folder"]]
        self.view._sortFolders = []
        self.seen = []
        self.view._sortTimer = _Timer()
        #  QTreeView.dropEvent needs a real QDropEvent; the port's own bookkeeping
        #  is what is under test, so the base call is stood aside.
        self.baseDrop = QtWidgets.QTreeView.dropEvent
        QtWidgets.QTreeView.dropEvent = lambda view, event: self.seen.append(
            view._dropAction)
        self.addCleanup(setattr, QtWidgets.QTreeView, "dropEvent", self.baseDrop)

    def test_the_action_is_visible_to_the_base_handler(self):
        """viewItemChanged fires from inside the base call and reads it there."""
        self.view.dropEvent(self._Drop(Qt.MoveAction))
        self.assertEqual(self.seen, [Qt.MoveAction])

    def test_a_copy_is_recorded_as_a_copy(self):
        self.view.dropEvent(self._Drop(Qt.CopyAction))
        self.assertEqual(self.seen, [Qt.CopyAction])

    def test_the_action_is_cleared_afterwards(self):
        """Left set, the next plain rename takes the move branch and detaches
        the node from its folder."""
        self.view.dropEvent(self._Drop(Qt.MoveAction))
        self.assertEqual(self.view._dropAction, Qt.IgnoreAction)

    def test_the_dragged_paths_are_cleared_afterwards(self):
        self.view.dropEvent(self._Drop(Qt.MoveAction))
        self.assertEqual(self.view._draggedNodePaths, [])

    def test_a_resort_is_queued(self):
        self.view.dropEvent(self._Drop(Qt.MoveAction))
        self.assertEqual(len(self.view._sortTimer.starts), 1)


class TestInputsViewDropEvent(DialogTest):
    """The inputs list forces a copy for drops arriving from the tree."""

    class _Drop:
        def __init__(self, source):
            self._source = source
            self.action = None

        def source(self):
            return self._source

        def setDropAction(self, action):
            self.action = action

    def setUp(self):
        super().setUp()
        self.cleanups = []
        self.inputs = self.sm.InputsView(self.view, None,
                                         lambda: self.cleanups.append(1))
        self.addCleanup(self.inputs.setParent, None)
        self.base = QtWidgets.QListView.dropEvent
        QtWidgets.QListView.dropEvent = lambda view, event: None
        self.addCleanup(setattr, QtWidgets.QListView, "dropEvent", self.base)

    def test_a_drop_from_the_tree_queues_a_tree_refresh(self):
        """The tree has to redraw: the node now appears in the inputs list too."""
        self.inputs.dropEvent(self._Drop(self.view))
        self.assertTrue(self.inputs._dropTimer.isActive())

    def test_a_drop_from_elsewhere_does_not(self):
        self.inputs.dropEvent(self._Drop(None))
        self.assertFalse(self.inputs._dropTimer.isActive())

    def test_a_drag_from_the_tree_is_forced_to_a_copy(self):
        """A move would take the node out of the tree it was dragged from."""
        base = QtWidgets.QAbstractItemView.dragEnterEvent
        QtWidgets.QAbstractItemView.dragEnterEvent = lambda view, event: None
        self.addCleanup(setattr, QtWidgets.QAbstractItemView,
                        "dragEnterEvent", base)

        event = self._Drop(self.view)
        self.inputs.dragEnterEvent(event)

        self.assertEqual(event.action, Qt.CopyAction)

    def test_a_drag_from_elsewhere_keeps_its_action(self):
        base = QtWidgets.QAbstractItemView.dragEnterEvent
        QtWidgets.QAbstractItemView.dragEnterEvent = lambda view, event: None
        self.addCleanup(setattr, QtWidgets.QAbstractItemView,
                        "dragEnterEvent", base)

        event = self._Drop(None)
        self.inputs.dragEnterEvent(event)

        self.assertIsNone(event.action)


if __name__ == "__main__":
    unittest.main()
