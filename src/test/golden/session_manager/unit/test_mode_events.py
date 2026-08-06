"""Gate 5 — the session manager's event handlers and mode lifecycle.

Almost every method here is bound to an RV event, and almost every one of them ends
in `event.reject()`. That call is not decoration: RV stops dispatching an event to the
remaining handlers as soon as one accepts it, so a handler that forgets to reject
silently disables every other mode's handler for the same event. Nothing in a golden
screenshot shows that, so each handler is checked for it individually.

The other half is the lazy-update discipline. The mode never rebuilds its tree
synchronously from an event; it starts a timer, and several handlers deliberately do
*not* start one (an inputs change on a node that is not the view, a tree update during
progressive loading). Getting that wrong gives either a stale panel or a rebuild storm
during load, and both are timing-dependent enough that a golden would not catch them
reliably.

`SessionManagerMode(name)` segfaults under the offscreen platform (see
`test_mode_panel.py`), so the instance is built with `object.__new__` and given only
the attributes the handler under test reads.
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
    """Stands in for a QTimer so a test can see that a lazy update was queued."""

    def __init__(self):
        self.starts = []
        self.stops = 0

    def start(self, ms):
        self.starts.append(ms)

    def stop(self):
        self.stops += 1


class EventTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.mode = self.sm.SessionManagerMode.__new__(self.sm.SessionManagerMode)
        self.mode._disableUpdates = False
        self.mode._inputOrderLock = False
        self.mode._previewsEnabled = False
        self.mode._progressiveLoadingInProgress = False
        self.mode._quitting = False
        self.mode._active = True
        self.mode._editors = []
        self.mode._srcNodeKeys = []
        self.mode._grpNodeValues = []

        self.mode._lazyUpdateTimer = _Timer()
        self.mode._lazySetInputsTimer = _Timer()
        self.mode._mainWinVisTimer = _Timer()

        self.model = QtGui.QStandardItemModel()
        self.view = QtWidgets.QTreeView()
        self.view.setModel(self.model)
        self.view._dropAction = Qt.IgnoreAction
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


class TestEventFilter(EventTest):
    """The dock installs this so RV keyboard shortcuts still reach the main view."""

    def test_it_forwards_to_the_session_gl_view(self):
        glView = self.sm.qtutils.sessionGLView()
        before = len(glView.forwarded)
        obj = QtCore.QObject()
        event = QtCore.QEvent(QtCore.QEvent.KeyPress)

        self.sm.EventFilter(None).eventFilter(obj, event)

        self.assertEqual(len(glView.forwarded), before + 1)
        self.assertIs(glView.forwarded[-1][0], obj)

    def test_it_returns_what_the_view_returned(self):
        """Returning True here would swallow the event instead of forwarding it."""
        glView = self.sm.qtutils.sessionGLView()
        glView.eventFilter = lambda obj, event: False
        self.assertFalse(
            self.sm.EventFilter(None).eventFilter(
                QtCore.QObject(), QtCore.QEvent(QtCore.QEvent.KeyPress)))

        glView.eventFilter = lambda obj, event: True
        self.assertTrue(
            self.sm.EventFilter(None).eventFilter(
                QtCore.QObject(), QtCore.QEvent(QtCore.QEvent.KeyPress)))

    def test_the_dock_installs_it_rather_than_the_mode_filtering_itself(self):
        """Mu puts eventFilter on a separate QObject, not on the mode: the mode is
        not a QObject and cannot be installed as a filter."""
        self.assertFalse(hasattr(self.mode, "eventFilter"))
        self.assertTrue(issubclass(self.sm.EventFilter, QtCore.QObject))


class TestProgressiveLoading(EventTest):
    """RV loads large sessions incrementally and brackets it with these two events."""

    def test_before_sets_the_flag(self):
        self.mode.beforeProgressiveLoading(_Event())
        self.assertTrue(self.mode._progressiveLoadingInProgress)

    def test_after_clears_the_flag(self):
        self.mode._progressiveLoadingInProgress = True
        self.mode.updateTree = lambda: None
        self.mode.updateInputs = lambda node: None
        self.mode.afterProgressiveLoading(_Event())
        self.assertFalse(self.mode._progressiveLoadingInProgress)

    def test_after_rebuilds_the_tree_and_the_inputs_once(self):
        calls = []
        self.mode.updateTree = lambda: calls.append("tree")
        self.mode.updateInputs = lambda node: calls.append(("inputs", node))
        self.graph.addNode("seq", "RVSequenceGroup")
        self.graph.viewNode = "seq"

        self.mode.afterProgressiveLoading(_Event())

        self.assertEqual(calls, ["tree", ("inputs", "seq")])

    def test_a_tree_update_during_loading_is_dropped(self):
        """Rebuilding per source turns a 500-source load into 500 full rebuilds."""
        calls = []
        self.mode.updateTree = lambda: calls.append(1)
        self.mode._progressiveLoadingInProgress = True
        self.mode.updateTreeEvent(_Event())
        self.assertEqual(calls, [])

    def test_a_tree_update_outside_loading_runs(self):
        calls = []
        self.mode.updateTree = lambda: calls.append(1)
        self.mode.updateTreeEvent(_Event())
        self.assertEqual(calls, [1])

    def test_all_three_reject(self):
        self.mode.updateTree = lambda: None
        self.mode.updateInputs = lambda node: None
        for method in ("beforeProgressiveLoading", "afterProgressiveLoading",
                       "updateTreeEvent"):
            with self.subTest(method=method):
                event = _Event()
                getattr(self.mode, method)(event)
                self.assertTrue(event.rejected)


class TestGraphViewChange(EventTest):
    def setUp(self):
        super().setUp()
        self.calls = []
        self.mode.selectViewableNode = lambda: self.calls.append("select")
        self.mode.setNodeStatus = lambda n, s: self.calls.append(("status", n, s))
        self.mode.updateNavUI = lambda: self.calls.append("nav")
        self.mode.restoreTabState = lambda: self.calls.append("restore")
        self.mode.saveTabState = lambda: self.calls.append("save")

        self.graph.addNode("seq", "RVSequenceGroup")
        self.graph.viewNode = "seq"

    def test_after_marks_the_new_view_node_with_a_tick(self):
        self.mode.afterGraphViewChange(_Event())
        self.assertIn(("status", "seq", "✔"), self.calls)

    def test_after_reselects_and_refreshes_the_nav_bar(self):
        self.mode.afterGraphViewChange(_Event())
        self.assertIn("select", self.calls)
        self.assertIn("nav", self.calls)
        self.assertIn("restore", self.calls)

    def test_after_asks_the_sibling_modes_for_their_editor(self):
        self.mode.afterGraphViewChange(_Event())
        self.assertIn(("session-manager-load-ui", "seq"), self.graph.events)

    def test_after_enables_the_inputs_panel_for_a_sequence(self):
        self.mode.afterGraphViewChange(_Event())
        self.assertTrue(self.mode._inputsView.isEnabled())

    def test_after_disables_the_inputs_panel_for_every_source_type(self):
        """A source has no inputs to reorder; leaving the panel live invites a
        setNodeInputs() against a node that cannot take one."""
        for nodeType in ("RVSource", "RVFileSource", "RVImageSource",
                         "RVSourceGroup"):
            with self.subTest(nodeType=nodeType):
                self.mode._inputsView.setEnabled(True)
                self.graph.addNode("src", nodeType)
                self.graph.viewNode = "src"
                self.mode.afterGraphViewChange(_Event())
                self.assertFalse(self.mode._inputsView.isEnabled())

    def test_after_does_nothing_without_a_view_node(self):
        self.graph.viewNode = None
        self.mode.afterGraphViewChange(_Event())
        self.assertEqual(self.calls, [])

    def test_after_still_rejects_without_a_view_node(self):
        """The reject comes first, so the other modes run even on an empty session."""
        self.graph.viewNode = None
        event = _Event()
        self.mode.afterGraphViewChange(event)
        self.assertTrue(event.rejected)

    def test_before_saves_the_tab_and_clears_the_old_tick(self):
        self.mode.beforeGraphViewChange(_Event())
        self.assertIn("save", self.calls)
        self.assertIn(("status", "seq", ""), self.calls)

    def test_before_hides_every_editor(self):
        """Without this the outgoing view's editor stays behind the incoming one."""
        tree = QtWidgets.QTreeWidget()
        self.addCleanup(tree.setParent, None)
        editors = [QtWidgets.QTreeWidgetItem(["Stack"]),
                   QtWidgets.QTreeWidgetItem(["Sequence"])]
        for e in editors:
            tree.addTopLevelItem(e)
            e.setHidden(False)
        self.mode._editors = editors

        self.mode.beforeGraphViewChange(_Event())

        self.assertTrue(all(e.isHidden() for e in editors))

    def test_both_reject(self):
        for method in ("afterGraphViewChange", "beforeGraphViewChange"):
            with self.subTest(method=method):
                event = _Event()
                getattr(self.mode, method)(event)
                self.assertTrue(event.rejected)

    def test_view_edit_mode_activated_reloads_the_editor(self):
        event = _Event()
        self.mode.viewEditModeActivated(event)
        self.assertTrue(event.rejected)
        self.assertIn(("session-manager-load-ui", "seq"), self.graph.events)


class TestNodeInputsChanged(EventTest):
    def setUp(self):
        super().setUp()
        self.updated = []
        self.mode.updateInputs = lambda node: self.updated.append(node)
        self.graph.addNode("seq", "RVSequenceGroup")
        self.graph.viewNode = "seq"

    def test_a_change_on_the_view_node_refreshes_the_inputs_panel(self):
        self.mode.nodeInputsChanged(_Event("seq"))
        self.assertEqual(self.updated, ["seq"])

    def test_a_change_elsewhere_leaves_the_panel_alone(self):
        self.graph.addNode("other", "RVSequenceGroup")
        self.mode.nodeInputsChanged(_Event("other"))
        self.assertEqual(self.updated, [])

    def test_a_folder_change_queues_a_tree_rebuild(self):
        """Folder membership is tree structure, not just an inputs list."""
        self.graph.addNode("folder", "RVFolderGroup")
        self.mode.nodeInputsChanged(_Event("folder"))
        self.assertEqual(self.mode._lazyUpdateTimer.starts, [0])

    def test_a_folder_change_mid_drop_does_not_queue_one(self):
        """The drop handler rebuilds once it settles; rebuilding underneath it
        destroys the items Qt is still using."""
        self.graph.addNode("folder", "RVFolderGroup")
        self.view._dropAction = Qt.MoveAction
        self.mode.nodeInputsChanged(_Event("folder"))
        self.assertEqual(self.mode._lazyUpdateTimer.starts, [])

    def test_nothing_happens_without_a_view_node(self):
        self.graph.viewNode = None
        event = _Event("seq")
        self.mode.nodeInputsChanged(event)
        self.assertEqual(self.updated, [])
        self.assertFalse(event.rejected,
                         "Mu returns before the reject in this branch too")

    def test_it_rejects(self):
        event = _Event("seq")
        self.mode.nodeInputsChanged(event)
        self.assertTrue(event.rejected)


class TestPropertyChanged(EventTest):
    def setUp(self):
        super().setUp()
        self.navUpdates = []
        self.mode.updateNavUI = lambda: self.navUpdates.append(1)
        self.graph.addNode("seq", "RVSequenceGroup")
        self.graph.viewNode = "seq"

    def test_a_ui_name_change_queues_a_rebuild_and_refreshes_the_nav_bar(self):
        self.mode.propertyChanged(_Event("seq.ui.name"))
        self.assertEqual(self.mode._lazyUpdateTimer.starts, [0])
        self.assertEqual(self.navUpdates, [1])

    def test_a_sort_key_change_queues_a_rebuild_without_touching_the_nav_bar(self):
        for name in ("sortKey", "sortKeyParent"):
            with self.subTest(name=name):
                self.mode._lazyUpdateTimer = _Timer()
                self.mode.propertyChanged(_Event("seq.sm_state.%s" % name))
                self.assertEqual(self.mode._lazyUpdateTimer.starts, [0])
        self.assertEqual(self.navUpdates, [])

    def test_an_unrelated_property_queues_nothing(self):
        self.mode.propertyChanged(_Event("seq.output.fps"))
        self.assertEqual(self.mode._lazyUpdateTimer.starts, [])
        self.assertEqual(self.navUpdates, [])

    def test_it_always_rejects(self):
        event = _Event("seq.output.fps")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


class TestInputRowSlots(EventTest):
    """The inputs list is reorderable by drag; Qt reports it as remove + insert."""

    def setUp(self):
        super().setUp()
        self.graph.addNode("seq", "RVSequenceGroup")
        self.graph.viewNode = "seq"

    def test_an_insert_queues_a_deferred_set_inputs(self):
        self.mode.inputRowsInsertedSlot(QtCore.QModelIndex(), 0, 0)
        self.assertEqual(self.mode._lazySetInputsTimer.starts, [100])

    def test_a_remove_queues_a_deferred_set_inputs(self):
        self.mode.inputRowsRemovedSlot(QtCore.QModelIndex(), 0, 0)
        self.assertEqual(self.mode._lazySetInputsTimer.starts, [100])

    def test_the_order_lock_suppresses_both(self):
        """updateInputs() rebuilds the model itself; without the lock its own
        row inserts would be written straight back to the graph."""
        self.mode._inputOrderLock = True
        self.mode.inputRowsInsertedSlot(QtCore.QModelIndex(), 0, 0)
        self.mode.inputRowsRemovedSlot(QtCore.QModelIndex(), 0, 0)
        self.assertEqual(self.mode._lazySetInputsTimer.starts, [])

    def test_no_view_node_suppresses_both(self):
        self.graph.viewNode = None
        self.mode.inputRowsInsertedSlot(QtCore.QModelIndex(), 0, 0)
        self.mode.inputRowsRemovedSlot(QtCore.QModelIndex(), 0, 0)
        self.assertEqual(self.mode._lazySetInputsTimer.starts, [])


class TestQuittingAndCategory(EventTest):
    def test_the_quitting_flag_is_set_before_the_session_is_deleted(self):
        """deactivate() reads it: on quit the "show on startup" setting must not
        be overwritten with the closed state."""
        event = _Event()
        self.mode.enterQuittingState(event)
        self.assertTrue(self.mode._quitting)
        self.assertTrue(event.rejected)

    def test_disabling_the_category_toggles_an_active_mode_off(self):
        toggles = []
        self.mode.toggle = lambda: toggles.append(1)
        self.graph.enabledCategories = []
        self.mode.onCategoryStateChanged(_Event())
        self.assertEqual(toggles, [1])

    def test_an_enabled_category_leaves_it_alone(self):
        toggles = []
        self.mode.toggle = lambda: toggles.append(1)
        self.mode.onCategoryStateChanged(_Event())
        self.assertEqual(toggles, [])

    def test_an_inactive_mode_is_not_toggled_again(self):
        toggles = []
        self.mode._active = False
        self.mode.toggle = lambda: toggles.append(1)
        self.graph.enabledCategories = []
        self.mode.onCategoryStateChanged(_Event())
        self.assertEqual(toggles, [])

    def test_it_rejects(self):
        self.mode.toggle = lambda: None
        event = _Event()
        self.mode.onCategoryStateChanged(event)
        self.assertTrue(event.rejected)


class TestVisibility(EventTest):
    """The dock's visibility and the mode's active flag are kept in step, but only
    after a delay: Qt reports a minimized window as hidden."""

    def setUp(self):
        super().setUp()
        self.dock = QtWidgets.QDockWidget()
        self.mode._dockWidget = self.dock
        self.toggles = []
        self.mode.toggle = lambda: self.toggles.append(1)

    def tearDown(self):
        self.dock.setParent(None)
        super().tearDown()

    def test_a_visibility_change_only_arms_the_timer(self):
        self.mode.visibilityChanged(False)
        self.assertEqual(self.mode._mainWinVisTimer.starts, [0])
        self.assertEqual(self.toggles, [])

    def test_a_hidden_dock_on_an_active_mode_toggles_it_off(self):
        self.dock.hide()
        self.mode._active = True
        self.mode.mainWinVisTimeout()
        self.assertEqual(self.toggles, [1])

    def test_a_visible_dock_on_an_inactive_mode_toggles_it_on(self):
        self.dock.show()
        self.mode._active = False
        self.mode.mainWinVisTimeout()
        self.assertEqual(self.toggles, [1])

    def test_an_agreeing_pair_is_left_alone(self):
        self.dock.hide()
        self.mode._active = False
        self.mode.mainWinVisTimeout()
        self.assertEqual(self.toggles, [])

    def test_a_minimized_main_window_is_ignored(self):
        """Minimizing hides the dock; acting on that would close the panel for good."""
        window = self.sm.qtutils.sessionWindow()
        window.showMinimized()
        try:
            self.dock.hide()
            self.mode._active = True
            self.mode.mainWinVisTimeout()
            self.assertEqual(self.toggles, [])
        finally:
            window.showNormal()


class TestActivateDeactivate(EventTest):
    def setUp(self):
        super().setUp()
        self.dock = QtWidgets.QDockWidget()
        self.mode._dockWidget = self.dock
        self.mode._eventFilter = self.sm.EventFilter(self.dock)
        self.mode.updateTree = lambda: None
        self.graph.addNode("seq", "RVSequenceGroup")
        self.graph.viewNode = "seq"

    def tearDown(self):
        self.dock.setParent(None)
        super().tearDown()

    def test_activate_shows_the_dock_and_rebuilds(self):
        calls = []
        self.mode.updateTree = lambda: calls.append(1)
        self.mode._active = False

        self.mode.activate()

        self.assertTrue(self.mode._active)
        self.assertFalse(self.dock.isHidden())
        self.assertEqual(calls, [1])

    def test_activate_asks_the_siblings_for_their_editor(self):
        self.mode.activate()
        self.assertIn(("session-manager-load-ui", "seq"), self.graph.events)

    def test_activate_remembers_the_panel_when_the_setting_says_last(self):
        self.graph.settings[("SessionManager", "showOnStartup")] = "last"
        self.mode.activate()
        self.assertTrue(self.graph.settings[("Tools", "show_session_manager")])

    def test_activate_leaves_the_setting_alone_otherwise(self):
        self.graph.settings[("SessionManager", "showOnStartup")] = "no"
        self.mode.activate()
        self.assertNotIn(("Tools", "show_session_manager"), self.graph.settings)

    def test_deactivate_hides_the_dock_and_stops_the_timers(self):
        self.mode._active = True
        self.mode.deactivate()
        self.assertFalse(self.mode._active)
        self.assertTrue(self.dock.isHidden())
        self.assertEqual(self.mode._lazySetInputsTimer.stops, 1)
        self.assertEqual(self.mode._lazyUpdateTimer.stops, 1)

    def test_deactivate_forgets_the_panel_when_the_setting_says_last(self):
        self.graph.settings[("SessionManager", "showOnStartup")] = "last"
        self.mode.deactivate()
        self.assertFalse(self.graph.settings[("Tools", "show_session_manager")])

    def test_quitting_does_not_forget_the_panel(self):
        """Closing on quit is not the user choosing to close it."""
        self.graph.settings[("SessionManager", "showOnStartup")] = "last"
        self.mode._quitting = True
        self.mode.deactivate()
        self.assertNotIn(("Tools", "show_session_manager"), self.graph.settings)

    def test_a_settings_failure_falls_back_to_not_showing(self):
        def boom(*a):
            raise RuntimeError("settings unavailable")

        self.sm.commands.readSettings = boom
        self.mode.activate()
        self.assertEqual(self.graph.settings[("SessionManager", "showOnStartup")],
                         "no")
        self.assertFalse(self.graph.settings[("Tools", "show_session_manager")])


if __name__ == "__main__":
    unittest.main()
