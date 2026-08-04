"""Gate 5 — SessionManagerMode methods that can be driven without the whole dock.

Each test builds the one or two widgets the method under test actually touches and
attaches them to an instance made with object.__new__. Constructing the real mode
needs a live RV session window, which VERIFICATION.md rules out for unit tests, and
most of these methods only reach a model, a tab widget or the settings anyway.

Methods that genuinely need the assembled panel (updateTree, newNodeRow,
makeSourceRowWidget, the drag/drop slots) are not here; they remain listed as
untested in COVERAGE.md rather than covered by a test that asserts nothing.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtGui, QtWidgets
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
        self.mode._previewsEnabled = True

    def row(self, text, node, columns=3):
        """A tree row shaped like newNodeRow builds it: name, radio, status."""
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        rest = [QtGui.QStandardItem("") for _ in range(columns - 1)]
        return [item] + rest


class TestSplitterMoved(ModeTest):
    PROP = "rv.session.sm_window.splitter"

    def test_writes_the_position_as_a_fraction_of_height(self):
        splitter = QtWidgets.QSplitter()
        splitter.resize(100, 400)
        self.mode._splitter = splitter

        self.mode.splitterMoved(100, 0)

        self.assertAlmostEqual(self.graph.getFloatProperty(self.PROP)[0], 0.25)

    def test_creates_the_property_when_absent(self):
        splitter = QtWidgets.QSplitter()
        splitter.resize(100, 200)
        self.mode._splitter = splitter
        self.assertFalse(self.graph.propertyExists(self.PROP))

        self.mode.splitterMoved(50, 0)

        self.assertTrue(self.graph.propertyExists(self.PROP))

    def test_overwrites_a_previous_fraction(self):
        splitter = QtWidgets.QSplitter()
        splitter.resize(100, 400)
        self.mode._splitter = splitter
        self.mode.splitterMoved(100, 0)
        self.mode.splitterMoved(300, 0)
        self.assertAlmostEqual(self.graph.getFloatProperty(self.PROP)[0], 0.75)


class TestSetNodeStatus(ModeTest):
    def setUp(self):
        super().setUp()
        self.model = QtGui.QStandardItemModel()
        self.mode._viewModel = self.model
        category = QtGui.QStandardItem("SOURCES")
        self.rowA = self.row("A", "nodeA")
        self.rowB = self.row("B", "nodeB")
        category.appendRow(self.rowA)
        category.appendRow(self.rowB)
        self.model.appendRow([category])

    def status(self, row):
        return row[0].parent().child(row[0].row(), 2).text()

    def test_sets_the_status_column_for_the_named_node(self):
        self.mode.setNodeStatus("nodeA", "✓")
        self.assertEqual(self.status(self.rowA), "✓")

    def test_leaves_other_nodes_alone(self):
        self.mode.setNodeStatus("nodeA", "✓")
        self.assertEqual(self.status(self.rowB), "")

    def test_clearing_the_status(self):
        self.mode.setNodeStatus("nodeA", "✓")
        self.mode.setNodeStatus("nodeA", "")
        self.assertEqual(self.status(self.rowA), "")

    def test_unknown_node_is_a_noop(self):
        self.mode.setNodeStatus("nope", "✓")   # must not raise

    def test_creates_the_status_cell_when_the_row_is_short(self):
        category = self.model.item(0)
        shortRow = self.row("C", "nodeC", columns=1)
        category.appendRow(shortRow)

        self.mode.setNodeStatus("nodeC", "✓")

        self.assertEqual(self.status(shortRow), "✓")


class TestTabState(ModeTest):
    def setUp(self):
        super().setUp()
        self.tabs = QtWidgets.QTabWidget()
        for name in ("Info", "Source", "Color"):
            self.tabs.addTab(QtWidgets.QWidget(), name)
        self.mode._tabWidget = self.tabs
        self.graph.addNode("seqGroup", "RVSequenceGroup")
        self.graph.viewNode = "seqGroup"

    def test_save_records_the_current_tab(self):
        self.tabs.setCurrentIndex(2)
        self.mode.saveTabState()
        self.assertEqual(self.graph.getIntProperty("seqGroup.sm_state.tab"), [2])

    def test_restore_applies_a_saved_tab(self):
        self.graph.seedInt("seqGroup.sm_state.tab", [2])
        self.mode.restoreTabState()
        self.assertEqual(self.tabs.currentIndex(), 2)

    def test_round_trip(self):
        self.tabs.setCurrentIndex(1)
        self.mode.saveTabState()
        self.tabs.setCurrentIndex(0)
        self.mode.restoreTabState()
        self.assertEqual(self.tabs.currentIndex(), 1)

    def test_a_source_group_defaults_to_tab_one(self):
        """J3: selecting a source jumps to the Source tab when nothing is saved."""
        self.graph.addNode("srcGroup", "RVSourceGroup")
        self.graph.viewNode = "srcGroup"
        self.tabs.setCurrentIndex(0)

        self.mode.restoreTabState()

        self.assertEqual(self.tabs.currentIndex(), 1)

    def test_a_saved_tab_beats_the_source_group_default(self):
        self.graph.addNode("srcGroup", "RVSourceGroup")
        self.graph.viewNode = "srcGroup"
        self.graph.seedInt("srcGroup.sm_state.tab", [2])

        self.mode.restoreTabState()

        self.assertEqual(self.tabs.currentIndex(), 2)

    def test_other_node_types_keep_the_current_tab(self):
        self.tabs.setCurrentIndex(2)
        self.mode.restoreTabState()
        self.assertEqual(self.tabs.currentIndex(), 2)

    def test_no_view_node_is_a_noop(self):
        self.graph.viewNode = None
        self.tabs.setCurrentIndex(2)
        self.mode.restoreTabState()
        self.assertEqual(self.tabs.currentIndex(), 2)

    def test_tab_change_slot_saves(self):
        self.tabs.setCurrentIndex(2)
        self.mode.tabChangeSlot(2)
        self.assertEqual(self.graph.getIntProperty("seqGroup.sm_state.tab"), [2])


class TestConfigSlot(ModeTest):
    def test_writes_both_settings(self):
        self.mode.configSlot(True, "always", True)
        self.assertEqual(
            self.graph.settings[("SessionManager", "showOnStartup")], "always")
        self.assertEqual(self.graph.settings[("Tools", "show_session_manager")], True)

    def test_each_startup_choice(self):
        for choice in ("always", "no", "last"):
            self.mode.configSlot(True, choice, True)
            self.assertEqual(
                self.graph.settings[("SessionManager", "showOnStartup")], choice)


class TestTogglePreviews(ModeTest):
    def setUp(self):
        super().setUp()
        self.treeUpdates = []
        self.mode.updateTree = lambda: self.treeUpdates.append(1)

    def test_enabling_persists_and_announces(self):
        self.mode.togglePreviews(True)
        self.assertTrue(self.mode._previewsEnabled)
        self.assertEqual(
            self.graph.settings[("SessionManager", "previewsEnabled")], True)
        self.assertIn(("session-manager-previews-enabled", ""), self.graph.events)

    def test_disabling_persists_and_announces(self):
        self.mode.togglePreviews(False)
        self.assertFalse(self.mode._previewsEnabled)
        self.assertEqual(
            self.graph.settings[("SessionManager", "previewsEnabled")], False)
        self.assertIn(("session-manager-previews-disabled", ""), self.graph.events)

    def test_the_tree_is_rebuilt_either_way(self):
        self.mode.togglePreviews(False)
        self.mode.togglePreviews(True)
        self.assertEqual(len(self.treeUpdates), 2)


class TestNavButtonClicked(ModeTest):
    def setUp(self):
        super().setUp()
        for n in ("a", "b", "c"):
            self.graph.addNode(n, "RVSourceGroup")
        self.graph.viewNode = "b"
        self.inputsUpdates = []
        self.mode.updateInputs = lambda node: self.inputsUpdates.append(node)

    def test_next_moves_to_the_next_view_node(self):
        self.sm.commands.nextViewNode = lambda: "c"
        self.mode.navButtonClicked("next", False)
        self.assertEqual(self.graph.viewNode, "c")

    def test_prev_moves_to_the_previous_view_node(self):
        self.sm.commands.previousViewNode = lambda: "a"
        self.mode.navButtonClicked("prev", False)
        self.assertEqual(self.graph.viewNode, "a")

    def test_no_next_node_leaves_the_view_alone(self):
        self.sm.commands.nextViewNode = lambda: None
        self.mode.navButtonClicked("next", False)
        self.assertEqual(self.graph.viewNode, "b")

    def test_updates_the_inputs_panel_for_the_new_view(self):
        self.sm.commands.nextViewNode = lambda: "c"
        self.mode.navButtonClicked("next", False)
        self.assertEqual(self.inputsUpdates, ["c"])

    def test_updates_are_re_enabled_afterwards(self):
        """The flag suppresses tree churn during the move; leaving it set is a bug."""
        self.sm.commands.nextViewNode = lambda: "c"
        self.mode.navButtonClicked("next", False)
        self.assertFalse(self.mode._disableUpdates)

    def test_updates_are_re_enabled_even_when_the_move_throws(self):
        def boom():
            raise RuntimeError("no such view")

        self.sm.commands.nextViewNode = boom
        self.mode.navButtonClicked("next", False)
        self.assertFalse(self.mode._disableUpdates)


class TestAuxFilePath(ModeTest):
    def test_joins_onto_the_support_path(self):
        import os

        path = self.mode.auxFilePath("session_manager.ui")
        self.assertEqual(os.path.basename(path), "session_manager.ui")
        self.assertEqual(os.path.dirname(path), _rv_stubs.PKG_DIR)

    def test_the_named_asset_exists_in_the_package(self):
        """auxFilePath is how every .ui and icon is found; a wrong root is fatal."""
        import os

        expected = os.path.join(_rv_stubs.PKG_DIR, "session_manager.ui")
        self.assertEqual(self.mode.auxFilePath("session_manager.ui"), expected)
        self.assertTrue(os.path.isfile(expected),
                        "the package must actually ship session_manager.ui")


class TestIconForNode(ModeTest):
    def setUp(self):
        super().setUp()
        # _typeIcons is an ordered list of (typeName, icon) pairs, mirroring Mu's
        # (string, QIcon)[] — not a mapping. iconForNode scans it linearly.
        self.mode._typeIcons = [("RVSourceGroup", "videofile"),
                                ("RVStackGroup", "album")]
        self.mode._unknownTypeIcon = "unknown"

    def test_known_type(self):
        self.graph.addNode("src", "RVSourceGroup")
        self.assertEqual(self.mode.iconForNode("src"), "videofile")

    def test_unknown_type_falls_back(self):
        self.graph.addNode("weird", "RVSomethingElse")
        self.assertEqual(self.mode.iconForNode("weird"), "unknown")

    def test_first_matching_pair_wins(self):
        self.mode._typeIcons = [("RVSourceGroup", "first"),
                                ("RVSourceGroup", "second")]
        self.graph.addNode("src", "RVSourceGroup")
        self.assertEqual(self.mode.iconForNode("src"), "first")

    def test_sub_component_icons_take_precedence_over_the_type(self):
        self.mode._viewIcon = "viewIcon"
        self.mode._layerIcon = "layerIcon"
        self.mode._channelIcon = "channelIcon"
        self.graph.addNode("src", "RVSourceGroup")

        for subType, expected in ((self.sm.ViewSubComponent, "viewIcon"),
                                  (self.sm.LayerSubComponent, "layerIcon"),
                                  (self.sm.ChannelSubComponent, "channelIcon")):
            self.graph.seedInt("src.sm_state.componentSubType", [subType])
            self.assertEqual(self.mode.iconForNode("src"), expected)

    def test_an_empty_sub_component_property_falls_through_to_the_type(self):
        self.mode._viewIcon = "viewIcon"
        self.graph.addNode("src", "RVSourceGroup")
        self.graph.seedInt("src.sm_state.componentSubType", [])
        self.assertEqual(self.mode.iconForNode("src"), "videofile")


if __name__ == "__main__":
    unittest.main()
