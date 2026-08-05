"""Gate 5 — building the session tree: `updateTree` and the rows it makes.

`updateTree` is the method the whole panel is: it clears the model, sorts the session's
view nodes into the six category headings, and recurses into folders. Every golden
scenario photographs its output, so its overall shape is well pinned — but a golden
only sees the sessions the harness can build headlessly, which is a handful of
sources, sequences and folders. The category assignment for the other node types, the
suppression of folder children at the top level, and the per-row data roles are not
in any of those pictures.

The data roles matter more than they look. Every other method in the package reads a
row through them — `itemNode` is `UserRole + 2`, the sort key is `UserRole + 3`, the
sub-component type is `UserRole + 4` — so a row built with the wrong role is a row
that silently drops out of selection, sorting and drag and drop.

`sourceFromSubComponent` and `newSubComponentNode` are the "view a layer on its own"
path: they create a real source node for a sub-component and file it under a
components folder. That is genuinely destructive to a session and is dropped from the
golden inventory for it, which leaves this as the only place it is checked.
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


class TreeTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._disableUpdates = False
        self.mode._inputOrderLock = False
        self.mode._previewsEnabled = False
        self.mode._progressiveLoadingInProgress = False
        self.mode._darkUI = False
        self.mode._srcNodeKeys = []
        self.mode._grpNodeValues = []
        self.mode._editors = []
        self.mode._lazyUpdateTimer = _Timer()
        fallback = QtGui.QPixmap(8, 8)
        fallback.fill(QtGui.QColor("grey"))
        self.mode._fallbackSourceIcon = QtGui.QIcon(fallback)
        self.mode._viewIcon = QtGui.QIcon()
        self.mode._layerIcon = QtGui.QIcon()
        self.mode._channelIcon = QtGui.QIcon()
        self.mode.selectViewableNode = lambda: None
        self.mode.iconForNode = lambda node: QtGui.QIcon()

        self.model = QtGui.QStandardItemModel()
        self.view = self.sm.NodeTreeView(None)
        self.view.setModel(self.model)
        self.mode._viewModel = self.model
        self.mode._viewTreeView = self.view

        self.inputsModel = QtGui.QStandardItemModel()
        self.inputsView = QtWidgets.QListView()
        self.inputsView.setModel(self.inputsModel)
        self.mode._inputsModel = self.inputsModel
        self.mode._inputsView = self.inputsView

    def tearDown(self):
        self.view.setParent(None)
        self.inputsView.setParent(None)

    def addViewNode(self, node, nodeType, inputs=None):
        """A top-level session node. A source group gets a real source inside it,
        the way RV builds one: newNodeRow reaches through the group for the media
        and the component request."""
        if nodeType == "RVSourceGroup":
            self.graph.addSourceGroup(node)
        else:
            self.graph.addNode(node, nodeType, inputs=inputs)
            self.graph.viewNodes.append(node)
        if inputs is not None:
            self.graph.connections[node] = list(inputs)
        if self.graph.viewNode is None:
            self.graph.viewNode = node
        return node

    def categories(self):
        root = self.model.invisibleRootItem()
        return {root.child(r, 0).text(): root.child(r, 0)
                for r in range(root.rowCount())}

    def childNodes(self, item):
        return [self.sm.itemNode(item.child(r, 0)) for r in range(item.rowCount())]


class TestUpdateTree(TreeTest):
    def test_an_empty_session_leaves_an_empty_model(self):
        self.mode.updateTree()
        self.assertEqual(self.model.rowCount(), 0)

    def test_the_columns_are_named(self):
        self.mode.updateTree()
        self.assertEqual(
            [self.model.horizontalHeaderItem(c).text() for c in range(3)],
            ["Name", "*", "*"])

    def test_a_source_lands_under_sources(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.assertEqual(self.childNodes(self.categories()["SOURCES"]), ["srcA"])

    def test_each_node_type_lands_in_its_own_category(self):
        for node, nodeType, category in (
            ("src", "RVSourceGroup", "SOURCES"),
            ("seq", "RVSequenceGroup", "SEQUENCES"),
            ("stk", "RVStackGroup", "STACKS"),
            ("lay", "RVLayoutGroup", "LAYOUTS"),
            ("fld", "RVFolderGroup", "FOLDERS"),
        ):
            with self.subTest(nodeType=nodeType):
                self.addViewNode(node, nodeType)
        self.mode.updateTree()

        got = {name: self.childNodes(item)
               for name, item in self.categories().items()}
        self.assertEqual(got.get("SOURCES"), ["src"])
        self.assertEqual(got.get("SEQUENCES"), ["seq"])
        self.assertEqual(got.get("STACKS"), ["stk"])
        self.assertEqual(got.get("LAYOUTS"), ["lay"])
        self.assertEqual(got.get("FOLDERS"), ["fld"])

    def test_an_unrecognised_type_lands_under_other(self):
        self.addViewNode("thing", "RVSomethingElse")
        self.mode.updateTree()
        self.assertEqual(self.childNodes(self.categories()["OTHER"]), ["thing"])

    def test_empty_categories_are_not_shown(self):
        """All six exist; only the populated ones are added to the model."""
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.assertEqual(list(self.categories()), ["SOURCES"])

    def test_a_node_inside_a_folder_is_not_also_listed_at_the_top(self):
        """Otherwise every foldered source appears twice, once in each place."""
        self.addViewNode("srcA", "RVSourceGroup")
        self.addViewNode("folder", "RVFolderGroup", inputs=["srcA"])

        self.mode.updateTree()

        categories = self.categories()
        self.assertNotIn("SOURCES", categories)
        self.assertEqual(self.childNodes(categories["FOLDERS"]), ["folder"])

    def test_a_foldered_node_is_listed_under_its_folder(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.addViewNode("folder", "RVFolderGroup", inputs=["srcA"])

        self.mode.updateTree()

        folderRow = self.categories()["FOLDERS"].child(0, 0)
        self.assertEqual(self.childNodes(folderRow), ["srcA"])

    def test_a_second_update_does_not_double_the_rows(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.mode.updateTree()
        self.assertEqual(self.childNodes(self.categories()["SOURCES"]), ["srcA"])

    def test_the_update_freeze_skips_it_entirely(self):
        """Several methods set the freeze while they mutate the graph; rebuilding
        underneath them destroys the items they are still holding."""
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.mode._disableUpdates = True
        self.graph.addNode("srcB", "RVSourceGroup")
        self.graph.viewNodes.append("srcB")

        self.mode.updateTree()

        self.assertEqual(self.childNodes(self.categories()["SOURCES"]), ["srcA"])

    def test_no_view_node_clears_the_model(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.graph.viewNode = None

        self.mode.updateTree()

        self.assertEqual(self.model.rowCount(), 0)

    def test_the_folders_row_is_handed_to_the_tree_view(self):
        """dragEnterEvent flips this row's drop flag; without it no drop target."""
        self.addViewNode("folder", "RVFolderGroup")
        self.mode.updateTree()
        self.assertIsNotNone(self.view._foldersItem)
        self.assertEqual(self.view._foldersItem.text(), "FOLDERS")

    def test_only_the_folders_category_accepts_drops(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.addViewNode("folder", "RVFolderGroup")
        self.mode.updateTree()

        categories = self.categories()
        self.assertTrue(categories["FOLDERS"].flags() & Qt.ItemIsDropEnabled)
        self.assertFalse(categories["SOURCES"].flags() & Qt.ItemIsDropEnabled)

    def test_the_expansion_of_a_category_is_remembered_in_the_session(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.assertEqual(self.graph.getIntProperty("rv.session.sm_view.SOURCES"), [1])

    def test_a_collapsed_category_stays_collapsed(self):
        self.addViewNode("srcA", "RVSourceGroup")
        self.mode.updateTree()
        self.graph.seedInt("#Session.sm_view.SOURCES", [0])

        self.mode.updateTree()

        item = self.categories()["SOURCES"]
        self.assertFalse(self.view.isExpanded(self.model.indexFromItem(item)))

    def test_the_source_node_map_is_rebuilt_not_appended_to(self):
        """updateNodePreviewEvent looks a group up through it; stale pairs point
        the preview at a node that no longer exists."""
        self.graph.addSourceGroup("srcA")
        self.graph.viewNodes = ["srcA"]
        self.graph.viewNode = "srcA"

        self.mode.updateTree()
        self.mode.updateTree()

        self.assertEqual(self.mode._srcNodeKeys, ["srcA_source"])
        self.assertEqual(self.mode._grpNodeValues, ["srcA"])


class TestNewNodeRow(TreeTest):
    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA")
        self.graph.uiNames["srcA"] = "Source A"
        self.graph.viewNode = "srcA"
        self.mode.iconForNode = lambda node: QtGui.QIcon()
        self.root = self.model.invisibleRootItem()

    def row(self, node="srcA", parent="", recursive=False):
        self.mode.newNodeRow(self.root, node, parent, recursive)
        return self.root.child(self.root.rowCount() - 1, 0)

    def test_the_row_is_labelled_with_the_ui_name(self):
        self.assertEqual(self.row().text(), "Source A")

    def test_the_node_is_stored_where_itemNode_reads_it(self):
        self.assertEqual(self.sm.itemNode(self.row()), "srcA")

    def test_the_parent_is_stored_where_itemParentNode_reads_it(self):
        self.graph.addNode("folder", "RVFolderGroup")
        item = self.row(parent="folder")
        self.assertEqual(self.sm.itemParentNode(item), "folder")

    def test_the_row_is_marked_as_a_plain_node(self):
        self.assertEqual(self.sm.itemSubComponentType(self.row()),
                         self.sm.NotASubComponent)

    def test_the_sort_key_is_stored_for_the_models_sort_role(self):
        self.graph.addNode("folder", "RVFolderGroup")
        self.graph.seedString("srcA.sm_state.sortKeyParent", ["folder"])
        self.graph.seedInt("srcA.sm_state.sortKey", [7])

        item = self.row(parent="folder")

        self.assertEqual(item.data(Qt.UserRole + 3), 7)

    def test_a_row_is_three_columns_wide(self):
        self.row()
        self.assertEqual(self.root.columnCount(), 3)

    def test_the_view_node_is_ticked(self):
        item = self.row()
        status = self.root.child(item.row(), 2)
        self.assertEqual(status.text(), "✔")

    def test_a_node_that_is_not_the_view_is_not_ticked(self):
        self.graph.addSourceGroup("srcB")
        item = self.row("srcB")
        self.assertEqual(self.root.child(item.row(), 2).text(), "")

    def test_a_plain_node_is_not_a_drop_target(self):
        self.assertFalse(self.row().flags() & Qt.ItemIsDropEnabled)

    def test_a_folder_is_a_drop_target(self):
        self.graph.addNode("folder", "RVFolderGroup")
        self.assertTrue(self.row("folder").flags() & Qt.ItemIsDropEnabled)

    def test_every_row_is_draggable_and_renamable(self):
        item = self.row()
        self.assertTrue(item.flags() & Qt.ItemIsDragEnabled)
        self.assertTrue(item.isEditable())

    def test_tabs_are_stripped_from_the_tooltip(self):
        """Tabs in tooltips crash Qt on win32; Mu replaces them for that reason."""
        self.graph.seedString("srcA.sm_state.toolTip", ["a\tb"])
        self.assertEqual(self.row().toolTip(), "a b")

    def test_a_node_with_no_tooltip_property_gets_an_empty_one(self):
        self.assertEqual(self.row().toolTip(), "")

    def test_a_folder_recurses_into_its_children(self):
        self.graph.addNode("folder", "RVFolderGroup", inputs=["srcA"])
        item = self.row("folder", recursive=True)
        self.assertEqual(self.childNodes(item), ["srcA"])

    def test_a_folder_does_not_recurse_when_not_asked_to(self):
        self.graph.addNode("folder", "RVFolderGroup", inputs=["srcA"])
        item = self.row("folder")
        self.assertEqual(item.rowCount(), 0)

    def test_a_previously_expanded_row_is_re_expanded(self):
        self.graph.addNode("folder", "RVFolderGroup", inputs=["srcA"])
        self.graph.seedString("folder.sm_state.expandState", [""])
        item = self.row("folder", recursive=True)
        self.assertTrue(self.view.isExpanded(self.model.indexFromItem(item)))

    def test_previews_off_leaves_the_name_visible(self):
        self.graph.addSourceGroup("srcB")
        item = self.row("srcB")
        self.assertNotEqual(item.text(), "")
        self.assertIsNone(
            self.view.indexWidget(self.model.indexFromItem(item)))

    def test_previews_on_swap_the_text_for_a_widget(self):
        """The row becomes a thumbnail plus two labels, so the item's own text has
        to be cleared or it draws behind the widget."""
        self.graph.addSourceGroup("srcB")
        self.mode._previewsEnabled = True

        item = self.row("srcB")

        self.assertEqual(item.text(), "")
        self.assertIsNotNone(
            self.view.indexWidget(self.model.indexFromItem(item)))

    def test_the_status_columns_are_enabled_and_selectable(self):
        items = self.mode.newNodeStatusColumns("srcA")
        self.assertEqual(len(items), 2)
        for item in items:
            self.assertTrue(item.flags() & Qt.ItemIsEnabled)
            self.assertTrue(item.flags() & Qt.ItemIsSelectable)

    def test_the_status_columns_start_empty(self):
        self.assertEqual([i.text() for i in self.mode.newNodeStatusColumns("srcA")],
                         ["", ""])


class TestMakeSourceRowWidget(TreeTest):
    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA", media="/tmp/shot_010.exr")
        self.graph.uiNames["srcA"] = "Shot 010"
        self.graph.viewNode = "srcA"

    def labels(self, widget):
        return [w.text() for w in widget.findChildren(QtWidgets.QLabel)
                if w.objectName() in ("sourceNameLabel", "sourceMetaLabel")]

    def test_it_shows_the_ui_name(self):
        widget = self.mode.makeSourceRowWidget("srcA")
        self.assertIn("Shot 010", self.labels(widget))

    def test_it_shows_the_media_extension_as_the_subtitle(self):
        widget = self.mode.makeSourceRowWidget("srcA")
        self.assertIn("exr", self.labels(widget))

    def test_a_source_with_no_extension_shows_an_em_dash(self):
        self.graph.seedString("srcA_source.media.movie", ["noextension"])
        widget = self.mode.makeSourceRowWidget("srcA")
        self.assertIn("—", self.labels(widget))

    def test_it_carries_a_preview(self):
        widget = self.mode.makeSourceRowWidget("srcA")
        previews = widget.findChildren(self.sm.SourcePreviewWidget)
        self.assertEqual(len(previews), 1)

    def test_the_preview_falls_back_when_no_thumbnail_was_generated(self):
        widget = self.mode.makeSourceRowWidget("srcA")
        preview = widget.findChildren(self.sm.SourcePreviewWidget)[0]
        self.assertFalse(preview._thumbnail.pixmap().isNull())

    def test_a_group_with_no_source_still_builds_a_row(self):
        """Mid-delete the group can outlive its source; the row must not raise."""
        self.graph.addNode("empty", "RVSourceGroup")
        self.graph.uiNames["empty"] = "Empty"
        widget = self.mode.makeSourceRowWidget("empty")
        self.assertIn("Empty", self.labels(widget))


class TestUpdateNodePreviewEvent(TreeTest):
    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA")
        self.graph.viewNodes = ["srcA"]
        self.graph.viewNode = "srcA"
        self.mode._previewsEnabled = True
        self.mode.iconForNode = lambda node: QtGui.QIcon()
        self.mode.updateTree()

    def treeWidget(self):
        item = self.sm.itemOfNode(self.model, "srcA")
        return self.view.indexWidget(self.model.indexFromItem(item))

    def test_it_replaces_the_row_widget_for_the_source(self):
        before = self.treeWidget()
        self.mode.updateNodePreviewEvent(_Event("srcA_source"))
        self.assertIsNot(self.treeWidget(), before)

    def test_it_does_nothing_with_previews_off(self):
        self.mode._previewsEnabled = False
        before = self.treeWidget()
        self.mode.updateNodePreviewEvent(_Event("srcA_source"))
        self.assertIs(self.treeWidget(), before)

    def test_an_unknown_source_is_ignored(self):
        before = self.treeWidget()
        self.mode.updateNodePreviewEvent(_Event("someOtherSource"))
        self.assertIs(self.treeWidget(), before)

    def test_it_rejects(self):
        event = _Event("srcA_source")
        self.mode.updateNodePreviewEvent(event)
        self.assertTrue(event.rejected)


class TestSubComponentRows(TreeTest):
    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA")
        self.graph.uiNames["srcA"] = "Source A"
        self.parent = QtGui.QStandardItem("Source A")
        self.parent.setData("srcA", Qt.UserRole + 2)
        self.model.invisibleRootItem().appendRow(
            [self.parent, QtGui.QStandardItem(""), QtGui.QStandardItem("")])

    def sub(self, subType, media="movie.exr", fullName="R", selected=False):
        return self.mode.newNodeSubComponent(
            subType, self.parent, media, fullName, "srcA", "", selected)

    def test_a_layer_row_is_labelled_with_its_name(self):
        self.assertEqual(self.sub(self.sm.LayerSubComponent).text(), "R")

    def test_a_media_row_is_labelled_with_the_basename(self):
        item = self.sub(self.sm.MediaSubComponent,
                        media="/a/b/shot.exr", fullName="/a/b/shot.exr")
        self.assertEqual(item.text(), "shot.exr")

    def test_an_unnamed_component_reads_as_default_in_italics(self):
        item = self.sub(self.sm.ViewSubComponent, fullName="")
        self.assertEqual(item.text(), "default")
        self.assertTrue(item.font().italic())

    def test_the_roles_the_rest_of_the_package_reads_are_set(self):
        item = self.sub(self.sm.LayerSubComponent)
        self.assertEqual(self.sm.itemNode(item), "srcA")
        self.assertEqual(self.sm.itemSubComponentType(item),
                         self.sm.LayerSubComponent)
        self.assertEqual(self.sm.itemSubComponentValue(item), "R")
        self.assertEqual(self.sm.itemSubComponentMedia(item), "movie.exr")

    def test_the_hash_is_stored_on_the_row(self):
        """componentAndFolderNodeFromHash matches on it to avoid a duplicate node."""
        item = self.sub(self.sm.LayerSubComponent)
        self.assertEqual(item.data(Qt.UserRole + 6),
                         self.sm.hashedSubComponent(item))

    def test_a_selected_component_gets_the_lit_radio_icon(self):
        selected = self.sub(self.sm.LayerSubComponent, selected=True)
        unselected = self.sub(self.sm.LayerSubComponent, fullName="G")
        radioSelected = self.parent.child(selected.row(), 1)
        radioUnselected = self.parent.child(unselected.row(), 1)
        self.assertFalse(radioSelected.icon().isNull())
        self.assertFalse(radioUnselected.icon().isNull())
        self.assertNotEqual(radioSelected.icon().cacheKey(),
                            radioUnselected.icon().cacheKey())

    def test_a_media_row_has_no_radio_button(self):
        """The file heading is not a component you can view on its own."""
        item = self.sub(self.sm.MediaSubComponent, fullName="shot.exr")
        self.assertTrue(self.parent.child(item.row(), 1).icon().isNull())

    def test_a_row_is_added_under_the_parent(self):
        self.sub(self.sm.LayerSubComponent)
        self.assertEqual(self.parent.rowCount(), 1)


class TestSourceFromSubComponent(TreeTest):
    def setUp(self):
        super().setUp()
        self.graph.addSourceGroup("srcA")
        self.graph.uiNames["srcA"] = "Source A"
        self.parent = QtGui.QStandardItem("Source A")
        self.parent.setData("srcA", Qt.UserRole + 2)
        self.model.invisibleRootItem().appendRow(
            [self.parent, QtGui.QStandardItem(""), QtGui.QStandardItem("")])

        self.media = self.mode.newNodeSubComponent(
            self.sm.MediaSubComponent, self.parent, "shot.exr", "shot.exr",
            "srcA", "", False)
        self.layer = self.mode.newNodeSubComponent(
            self.sm.LayerSubComponent, self.media, "shot.exr", "diffuse",
            "srcA", "", False)

    def test_it_creates_a_source_for_the_component(self):
        node = self.mode.sourceFromSubComponent(self.layer, "srcA")
        self.assertTrue(self.graph.nodeExists(node))

    def test_the_new_source_is_named_after_the_component(self):
        node = self.mode.sourceFromSubComponent(self.layer, "srcA")
        self.assertEqual(self.graph.uiName(node), "Source A (Layer diffuse)")

    def test_it_is_filed_under_a_components_folder(self):
        node = self.mode.sourceFromSubComponent(self.layer, "srcA")
        folders = [n for n in self.graph.nodes
                   if self.graph.nodeType(n) == "RVFolderGroup"]
        self.assertEqual(len(folders), 1)
        self.assertIn(node, self.graph.nodeConnections(folders[0])[0])

    def test_the_folder_is_named_after_the_original(self):
        self.mode.sourceFromSubComponent(self.layer, "srcA")
        folder = [n for n in self.graph.nodes
                  if self.graph.nodeType(n) == "RVFolderGroup"][0]
        self.assertEqual(self.graph.uiName(folder), "Components of Source A")

    def test_the_new_source_records_where_it_came_from(self):
        node = self.mode.sourceFromSubComponent(self.layer, "srcA")
        self.assertEqual(
            self.graph.getStringProperty(node + ".sm_state.componentOfNode"),
            ["srcA"])
        self.assertEqual(
            self.graph.getStringProperty(node + ".sm_state.componentHash"),
            [self.sm.hashedSubComponent(self.layer)])

    def test_a_second_component_reuses_the_same_folder(self):
        self.mode.sourceFromSubComponent(self.layer, "srcA")
        other = self.mode.newNodeSubComponent(
            self.sm.LayerSubComponent, self.media, "shot.exr", "specular",
            "srcA", "", False)

        self.mode.sourceFromSubComponent(other, "srcA")

        folders = [n for n in self.graph.nodes
                   if self.graph.nodeType(n) == "RVFolderGroup"]
        self.assertEqual(len(folders), 1)

    def test_both_components_are_filed_under_it(self):
        first = self.mode.sourceFromSubComponent(self.layer, "srcA")
        other = self.mode.newNodeSubComponent(
            self.sm.LayerSubComponent, self.media, "shot.exr", "specular",
            "srcA", "", False)
        second = self.mode.sourceFromSubComponent(other, "srcA")

        folder = [n for n in self.graph.nodes
                  if self.graph.nodeType(n) == "RVFolderGroup"][0]
        self.assertEqual(self.graph.nodeConnections(folder)[0], [first, second])

    def test_the_same_component_twice_makes_a_second_node(self):
        """Mu defect, reproduced deliberately: componentAndFolderNodeFromHash finds
        the existing node and then returns its still-unset `cnode` local instead of
        it, so the dedup never fires. Clicking a layer's radio button twice adds two
        sources. Changing that here would be a behavior change, not a port fix."""
        first = self.mode.sourceFromSubComponent(self.layer, "srcA")
        second = self.mode.sourceFromSubComponent(self.layer, "srcA")
        self.assertNotEqual(second, first)

    def test_the_hash_lookup_reports_no_match_even_when_one_exists(self):
        """The other half of the same defect, stated on its own so that fixing it
        one day fails one focused test rather than a scatter of them."""
        self.mode.sourceFromSubComponent(self.layer, "srcA")
        hash = self.sm.hashedSubComponent(self.layer)

        found, _folder = self.mode.componentAndFolderNodeFromHash(hash, "srcA")
        self.assertIsNone(found)

    def test_a_matching_hash_returns_early_without_the_folder(self):
        """The early return skips the rest of the node scan, so the components
        folder found after it is lost as well."""
        self.mode.sourceFromSubComponent(self.layer, "srcA")
        hash = self.sm.hashedSubComponent(self.layer)
        cnode, folder = self.mode.componentAndFolderNodeFromHash(hash, "srcA")
        self.assertIsNone(cnode)
        self.assertIsNone(folder)

    def test_an_unknown_hash_still_finds_the_components_folder(self):
        self.mode.sourceFromSubComponent(self.layer, "srcA")
        cnode, folder = self.mode.componentAndFolderNodeFromHash("nope", "srcA")
        self.assertIsNone(cnode)
        self.assertIsNotNone(folder)

    def test_nothing_is_found_in_an_untouched_session(self):
        cnode, folder = self.mode.componentAndFolderNodeFromHash("h", "srcA")
        self.assertIsNone(cnode)
        self.assertIsNone(folder)


class TestMapOverItem(TreeTest):
    """mapItems walks the tree; the nested mapOverItem is the recursion itself."""

    def setUp(self):
        super().setUp()
        self.root = self.model.invisibleRootItem()

    def row(self, text, node, parent=None):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        (parent or self.root).appendRow(item)
        return item

    def nodes(self, F=None):
        return [self.sm.itemNode(i)
                for i in self.sm.mapItems(self.model, F or (lambda item: True))]

    def test_it_reaches_every_depth(self):
        a = self.row("A", "a")
        b = self.row("B", "b", a)
        self.row("C", "c", b)
        self.assertEqual(sorted(self.nodes()), ["a", "b", "c"])

    def test_it_returns_children_before_their_parent(self):
        """Mu conses onto the front of the accumulator, so the deepest row comes
        first. Callers that delete rows depend on it: removing a parent first
        invalidates the children still to come."""
        a = self.row("A", "a")
        self.row("B", "b", a)
        self.assertEqual(self.nodes(), ["a", "b"])

    def test_category_rows_are_skipped(self):
        """A heading carries no node, so it is not a row anything can act on."""
        heading = QtGui.QStandardItem("SOURCES")
        heading.setData("", Qt.UserRole + 2)
        self.root.appendRow(heading)
        self.row("A", "a", heading)
        self.assertEqual(self.nodes(), ["a"])

    def test_the_predicate_filters_the_result(self):
        self.row("A", "a")
        self.row("B", "b")
        self.assertEqual(self.nodes(lambda item: self.sm.itemNode(item) == "b"),
                         ["b"])

    def test_a_filtered_out_parent_is_still_descended_into(self):
        a = self.row("A", "a")
        self.row("B", "b", a)
        self.assertEqual(self.nodes(lambda item: self.sm.itemNode(item) == "b"),
                         ["b"])

    def test_an_empty_model_maps_to_nothing(self):
        self.assertEqual(self.nodes(), [])


class TestIcons(TreeTest):
    """The toolbar artwork comes in a light and a dark variant, named `x_48x48.png`
    and `x_out.png`, and lives in RV's compiled Qt resource bundle. That bundle is
    not registered outside a running RV, so every `:images/...` load here yields a
    null QImage and two variants would compare equal no matter which was picked.

    What decides the button's appearance is therefore checked where it is decided:
    which of the two paths the icon is loaded from.
    """

    def setUp(self):
        super().setUp()
        self.loaded = []
        realImage = self.sm.QtGui.QImage

        #  Distinct sizes per path: the resources are absent outside RV, so both
        #  variants would otherwise load as identical null images and the choice
        #  between them would be unobservable. The size survives into the icon.
        sizes = {"out": 8, "48x48": 16}

        def recordingImage(path, fmt=""):
            self.loaded.append(path)
            side = sizes["out"] if "_out" in path else sizes["48x48"]
            image = realImage(side, side, realImage.Format_RGB32)
            image.fill(QtGui.QColor("white"))
            return image

        self.sm.QtGui.QImage = recordingImage
        self.addCleanup(setattr, self.sm.QtGui, "QImage", realImage)

    OUTLINE = 8
    SOLID = 16

    def chosenSide(self, invertSense):
        icon = self.mode.colorAdjustedIcon(":images/new_48x48.png", invertSense)
        return icon.availableSizes(QtGui.QIcon.Normal, QtGui.QIcon.Off)[0].width()

    def test_an_aux_icon_is_loaded_from_the_resource_path(self):
        icon = self.mode.auxIcon("new_48x48.png")
        self.assertIsInstance(icon, QtGui.QIcon)

    def test_a_plain_aux_icon_does_no_colour_adjustment(self):
        self.mode.auxIcon("new_48x48.png")
        self.assertEqual(self.loaded, [])

    def test_a_colour_adjusted_icon_is_also_an_icon(self):
        icon = self.mode.auxIcon("new_48x48.png", True)
        self.assertIsInstance(icon, QtGui.QIcon)

    def test_a_light_ui_uses_the_solid_artwork(self):
        self.mode._darkUI = False
        self.assertEqual(self.chosenSide(False), self.SOLID)

    def test_a_dark_ui_uses_the_outline_artwork(self):
        """The solid variant on a dark panel reads as a black smudge."""
        self.mode._darkUI = True
        self.assertEqual(self.chosenSide(False), self.OUTLINE)

    def test_inverting_the_sense_swaps_both_ways(self):
        """Buttons that draw on a contrasting background pass invertSense=True."""
        self.mode._darkUI = False
        self.assertEqual(self.chosenSide(True), self.OUTLINE)
        self.mode._darkUI = True
        self.assertEqual(self.chosenSide(True), self.SOLID)

    def test_the_selected_state_always_uses_the_solid_artwork(self):
        """Selection paints its own highlight behind the icon, so the outline
        variant would vanish into it."""
        self.mode._darkUI = True
        icon = self.mode.colorAdjustedIcon(":images/new_48x48.png", False)
        selected = icon.availableSizes(QtGui.QIcon.Selected, QtGui.QIcon.Off)
        self.assertEqual(selected[0].width(), self.SOLID)

    def test_both_variants_are_loaded_so_the_selected_state_can_use_the_other(self):
        self.mode.colorAdjustedIcon(":images/new_48x48.png", False)
        self.assertEqual(self.loaded,
                         [":images/new_out.png", ":images/new_48x48.png"])

    def test_the_aux_icon_path_goes_through_the_same_adjustment(self):
        self.mode.auxIcon("new_48x48.png", True)
        self.assertEqual(self.loaded,
                         [":images/new_out.png", ":images/new_48x48.png"])


if __name__ == "__main__":
    unittest.main()
