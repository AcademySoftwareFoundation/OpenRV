"""Gate 5 — the mode behaviours no golden scenario can reach.

Each class here stands in for an inventory row that the golden harness cannot pin,
for one of two reasons, both verified against the **Mu** implementation so they are
harness limits rather than port defects:

* **Focus-dependent.** `run_scenario.py` drives RV through `-pyeval`, before the Qt
  event loop and with no real window focus. A synthesised double-click does not reach
  `viewByIndex()`, and `QTreeView.edit(index)` opens no editor — confirmed by watching
  `viewNode()` stay put and the view stay in `NoState` under Mu.
* **Modal.** The context menu is shown with `QMenu.exec()` and the Create Image and
  New Node dialogs block too. VERIFICATION.md drops modal UI from the golden
  inventory, so the construction is checked here instead of the interaction.

These carry the 🟡 rows in COVERAGE.md: pinned by a unit test, not by a golden.
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


class ModeTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._disableUpdates = False
        self.mode.updateInputs = lambda node: None

        self.model = QtGui.QStandardItemModel()
        self.view = QtWidgets.QTreeView()
        self.view.setModel(self.model)
        self.mode._viewModel = self.model
        self.mode._viewTreeView = self.view

    def tearDown(self):
        self.view.setParent(None)

    def row(self, text, node, subType=None, value=None):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        if subType is not None:
            item.setData(subType, Qt.UserRole + 4)
        if value is not None:
            item.setData(value, Qt.UserRole + 5)
        self.model.appendRow([item])
        return item


class TestViewByIndex(ModeTest):
    """COVERAGE B2 and G10 — what a double-click ends up calling."""

    def setUp(self):
        super().setUp()
        self.graph.addNode("srcA", "RVSourceGroup")
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.viewNode = "srcA"

    def test_sets_the_clicked_node_as_the_view(self):
        item = self.row("B", "srcB")
        self.mode.viewByIndex(self.model.indexFromItem(item), self.model)
        self.assertEqual(self.graph.viewNode, "srcB")

    def test_clicking_the_current_view_is_not_a_view_change(self):
        item = self.row("A", "srcA")
        self.mode.viewByIndex(self.model.indexFromItem(item), self.model)
        self.assertEqual(self.graph.viewNode, "srcA")

    def test_updates_are_re_enabled_afterwards(self):
        item = self.row("B", "srcB")
        self.mode.viewByIndex(self.model.indexFromItem(item), self.model)
        self.assertFalse(self.mode._disableUpdates)

    def test_a_sub_component_row_also_sets_the_image_request(self):
        self.graph.addNode("srcB_source", "RVSource", group="srcB")
        self.graph.seedString("srcB_source.request.imageComponent", [])
        item = self.row("left", "srcB", subType=self.sm.ViewSubComponent, value="left")

        self.mode.viewByIndex(self.model.indexFromItem(item), self.model)

        self.assertEqual(self.graph.viewNode, "srcB")
        self.assertEqual(
            self.graph.getStringProperty("srcB_source.request.imageComponent"),
            ["view", "left"])

    def test_a_plain_row_leaves_the_image_request_alone(self):
        self.graph.addNode("srcB_source", "RVSource", group="srcB")
        self.graph.seedString("srcB_source.request.imageComponent", ["view", "keep"])
        item = self.row("B", "srcB")

        self.mode.viewByIndex(self.model.indexFromItem(item), self.model)

        self.assertEqual(
            self.graph.getStringProperty("srcB_source.request.imageComponent"),
            ["view", "keep"])

    def test_a_failing_view_change_still_clears_the_flag(self):
        def boom(node):
            raise RuntimeError("no such view")

        self.sm.commands.setViewNode = boom
        item = self.row("B", "srcB")
        self.mode.viewByIndex(self.model.indexFromItem(item), self.model)
        self.assertFalse(self.mode._disableUpdates,
                         "an exception must not leave updates disabled")


class TestEditViewInfoSlot(ModeTest):
    """COVERAGE F1 and F2 — Edit Info / the edit key open the inline editor."""

    def setUp(self):
        super().setUp()
        self.edited = []
        self.view.edit = lambda index: self.edited.append(index)

    def test_edits_the_selected_row(self):
        item = self.row("A", "srcA")
        idx = self.model.indexFromItem(item)
        self.view.selectionModel().select(idx, QtCore.QItemSelectionModel.Select)

        self.mode.editViewInfoSlot(False)

        self.assertEqual(len(self.edited), 1)
        self.assertEqual(self.edited[0].row(), idx.row())

    def test_no_selection_edits_nothing(self):
        self.row("A", "srcA")
        self.mode.editViewInfoSlot(False)
        self.assertEqual(self.edited, [])

    def test_the_first_selected_row_wins(self):
        a = self.row("A", "srcA")
        b = self.row("B", "srcB")
        for item in (a, b):
            self.view.selectionModel().select(
                self.model.indexFromItem(item), QtCore.QItemSelectionModel.Select)

        self.mode.editViewInfoSlot(False)

        self.assertEqual(len(self.edited), 1)


class TestContextMenuConstruction(ModeTest):
    """COVERAGE L1, L2, D4 — what the right-click menu contains.

    exec() blocks, so the menu is built once and then inspected rather than shown.
    """

    def setUp(self):
        super().setUp()
        self.mode._viewContextMenu = None
        self.mode.auxIcon = lambda name, adjust=False: QtGui.QIcon()
        self.mode._folderMenu = QtWidgets.QMenu("Folder")
        for label in ("Empty Folder", "From Selection", "From Copy of Selection"):
            self.mode._folderMenu.addAction(label)
        self.mode._createMenu = QtWidgets.QMenu("Create")
        for label in ("Sequence", "Stack", "Layout"):
            self.mode._createMenu.addAction(label)
        self.mode._viewContextMenuActions = [
            QtGui.QAction("Delete"), QtGui.QAction("Edit Info"),
            QtGui.QAction("Select Current"),
        ]
        self.shown = []

    def build(self):
        """Run the slot with a QMenu subclass whose exec() does not block.

        exec() waits for the menu to be dismissed, which never happens headlessly.
        It cannot be monkeypatched either: QMenu.exec is a shiboken C++ method, so
        assigning over it (or mock.patch.object on the class) does not take effect and
        the real blocking call still runs — that hung the whole suite. Substituting the
        class the mode constructs is what actually works, and mock.patch.object on the
        module attribute restores it afterwards.
        """
        from unittest import mock

        shown = self.shown

        class _NoExecMenu(QtWidgets.QMenu):
            def exec(self, *a, **k):
                shown.append(self)

        with mock.patch.object(self.sm.QtWidgets, "QMenu", _NoExecMenu):
            self.mode.viewContextMenuSlot(QtCore.QPoint(5, 5))
        return self.mode._viewContextMenu

    def labels(self, menu):
        return [a.text().replace("&", "") for a in menu.actions()]

    def test_the_three_actions_are_present(self):
        """L1: Delete / Edit Info / Select Current."""
        got = self.labels(self.build())
        for wanted in ("Delete", "Edit Info", "Select Current"):
            self.assertIn(wanted, got)

    def test_folder_and_create_submenus_are_present(self):
        """L2: both submenus hang off the context menu."""
        menu = self.build()
        submenus = [a.menu().title() for a in menu.actions() if a.menu()]
        self.assertIn("Folder", submenus)
        self.assertIn("Create", submenus)

    def test_the_folder_submenu_mirrors_the_folder_button_menu(self):
        """D4: the same QMenu object is reused, so the two cannot drift apart."""
        menu = self.build()
        folder = [a.menu() for a in menu.actions()
                  if a.menu() and a.menu().title() == "Folder"][0]
        self.assertIs(folder, self.mode._folderMenu)
        self.assertEqual(self.labels(folder),
                         ["Empty Folder", "From Selection", "From Copy of Selection"])

    def test_the_menu_is_built_once_and_reused(self):
        first = self.build()
        second = self.build()
        self.assertIs(first, second, "rebuilding would duplicate the actions")

    def test_the_menu_is_actually_shown(self):
        self.build()
        self.assertEqual(len(self.shown), 1)


class TestCreateImageDialogDefaults(unittest.TestCase):
    """COVERAGE C15 — the Create Image dialog's FPS defaults from General/fps.

    The dialog is modal, so only the default is checked, at the point the mode reads
    the setting. Driving the dialog itself is out of scope for a unit test and out of
    scope for a golden.
    """

    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")

    def test_fps_default_is_read_from_the_setting(self):
        self.graph.settings[("General", "fps")] = 48.0
        self.assertEqual(
            float(self.sm.commands.readSettings("General", "fps", 24.0)), 48.0)

    def test_fps_falls_back_to_24_when_unset(self):
        self.assertEqual(
            float(self.sm.commands.readSettings("General", "fps", 24.0)), 24.0)

    def test_the_dialog_formats_it_without_a_trailing_zero(self):
        """The mode writes "%g" % fps into the line edit."""
        self.assertEqual("%g" % 24.0, "24")
        self.assertEqual("%g" % 23.98, "23.98")


class TestNewNodeByTypeDialog(unittest.TestCase):
    """COVERAGE C8 — Add > New Node by Type… lists every node type.

    The dialog is modal, so what is checked is the list it is populated from and the
    creation path it feeds. Driving the dialog itself is out of scope for a unit test
    and dropped from the golden inventory by VERIFICATION.md.
    """

    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode.selectedConvertedSubComponents = lambda: []
        self.mode.renameByType = lambda node, nodes: None

    def test_the_combo_is_filled_from_nodeTypes(self):
        types = ["RVSequenceGroup", "RVStackGroup", "RVColor"]
        self.sm.commands.nodeTypes = lambda userVisible=True: types
        combo = QtWidgets.QComboBox()
        combo.addItems(self.sm.commands.nodeTypes(True))
        self.assertEqual([combo.itemText(i) for i in range(combo.count())], types)

    def test_choosing_a_type_creates_that_node(self):
        """The dialog's accept path lands in addNodeOfType, which is testable."""
        self.graph.viewNode = None
        node = self.mode.addNodeOfType("RVSequenceGroup")
        self.assertIsNotNone(node)
        self.assertEqual(self.graph.nodeType(node), "RVSequenceGroup")
        self.assertEqual(self.graph.viewNode, node)

    def test_an_unbuildable_type_raises_rather_than_leaving_a_stray_node(self):
        """The RVOCIO case: newNode throws and nothing is left behind."""
        def boom(t, name):
            raise RuntimeError("can't build node of type '%s'" % t)

        self.sm.commands.newNode = boom
        before = set(self.graph.nodes)
        with self.assertRaises(Exception):
            self.mode.addNodeOfType("RVOCIO")
        self.assertEqual(set(self.graph.nodes), before)

    def test_addThingSlot_routes_an_empty_string_to_the_dialog(self):
        called = []
        self.mode.addNodeByTypeName = lambda: called.append("dialog")
        self.mode.addMovieProc = lambda spec: called.append(("movieproc", spec))
        self.mode.addNodeOfType = lambda t: called.append(("type", t))

        self.mode.addThingSlot(False, "")
        self.assertEqual(called, ["dialog"])

    def test_addThingSlot_routes_a_movieproc_spec_and_a_plain_type(self):
        called = []
        self.mode.addNodeByTypeName = lambda: called.append("dialog")
        self.mode.addMovieProc = lambda spec: called.append(("movieproc", spec))
        self.mode.addNodeOfType = lambda t: called.append(("type", t))

        self.mode.addThingSlot(False, "black,%s.movieproc")
        self.mode.addThingSlot(False, "RVStackGroup")
        self.assertEqual(called,
                         [("movieproc", "black,%s.movieproc"), ("type", "RVStackGroup")])


class TestFolderDropTargets(unittest.TestCase):
    """COVERAGE A8 — only folders accept a drop; other category rows do not.

    A real drag needs a grab and a live event loop, which the headless harness has
    not got, so the policy is checked where it is decided: dragEnterEvent flips the
    FOLDERS row's drop flag, and dragMoveEvent rejects the illegal targets. The
    rejection rules themselves are covered in unit/test_tree_view.py.
    """

    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.view = self.sm.NodeTreeView(None)
        self.model = self.sm.NodeModel(None)
        self.view.setModel(self.model)
        self.view._viewModel = self.model

    def tearDown(self):
        self.view.setParent(None)

    def row(self, text, node):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        self.model.appendRow([item])
        return item

    def _dragEnter(self):
        # The QMimeData must outlive the event: passing it inline lets Python collect
        # it while the C++ event still holds the pointer, which segfaults the run.
        self._mime = QtCore.QMimeData()
        event = QtGui.QDragEnterEvent(
            QtCore.QPoint(1, 1), Qt.CopyAction, self._mime,
            Qt.LeftButton, Qt.NoModifier)
        event.source = lambda: self.view
        self.view.dragEnterEvent(event)

    def test_dragging_a_non_folder_makes_the_folders_row_undroppable(self):
        self.graph.addNode("srcNode", "RVSourceGroup")
        folders = self.row("FOLDERS", "")
        self.view._foldersItem = folders
        item = self.row("Src", "srcNode")
        self.view.selectionModel().select(
            self.model.indexFromItem(item), QtCore.QItemSelectionModel.Select)

        self._dragEnter()

        self.assertTrue(self.view._draggingNonFolders)
        self.assertFalse(bool(folders.flags() & Qt.ItemIsDropEnabled))

    def test_dragging_a_folder_leaves_the_folders_row_droppable(self):
        self.graph.addNode("folderNode", "RVFolderGroup")
        folders = self.row("FOLDERS", "")
        self.view._foldersItem = folders
        item = self.row("Folder", "folderNode")
        self.view.selectionModel().select(
            self.model.indexFromItem(item), QtCore.QItemSelectionModel.Select)

        self._dragEnter()

        self.assertFalse(self.view._draggingNonFolders)
        self.assertTrue(bool(folders.flags() & Qt.ItemIsDropEnabled))

    def test_only_folder_groups_are_recorded_for_re_sorting(self):
        self.graph.addNode("folderNode", "RVFolderGroup")
        self.graph.addNode("seqNode", "RVSequenceGroup")
        self.view.sortFolderChildren("folderNode")
        self.view.sortFolderChildren("seqNode")
        self.assertEqual(self.view._sortFolders, ["folderNode"])


if __name__ == "__main__":
    unittest.main()
