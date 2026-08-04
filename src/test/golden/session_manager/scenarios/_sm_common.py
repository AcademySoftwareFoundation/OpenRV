#
# Shared helpers for session_manager golden scenarios.
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
from __future__ import annotations

import hashlib
import os
import time

import rv.commands as rvc
import rv.qtutils as qtutils
from qt_scenario_utils import QtCore, QtWidgets, QTest, pump, click_button, open_tool_button_menu, click_menu_action

SM_MODE = "session_manager"


# ---------------------------------------------------------------------------
# RV Python API shims (functions missing from rv.commands in installed RV)
# ---------------------------------------------------------------------------

def set_ui_name(node: str, name: str) -> None:
    """Set a node's display name via the ui.name property."""
    prop = node + ".ui.name"
    if not rvc.propertyExists(prop):
        rvc.newProperty(prop, rvc.StringType, 1)
    rvc.setStringProperty(prop, [name], True)


def get_ui_name(node: str) -> str:
    """Get a node's display name from the ui.name property, or the node name."""
    prop = node + ".ui.name"
    if rvc.propertyExists(prop):
        vals = rvc.getStringProperty(prop)
        if vals and vals[0]:
            return vals[0]
    return node

# Media fixture — override with SM_MERIDIAN_DIR env var.
_DEFAULT_MERIDIAN = (
    "/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001"
)
SM_MERIDIAN_DIR = os.environ.get("SM_MERIDIAN_DIR", _DEFAULT_MERIDIAN)
SM_CLIP_1 = os.path.join(SM_MERIDIAN_DIR, "Meridian_-_Clip_0001.mp4")
SM_CLIP_2 = os.path.join(SM_MERIDIAN_DIR, "Meridian_-_Clip_0002.mp4")
SM_CLIP_3 = os.path.join(SM_MERIDIAN_DIR, "Meridian_-_Clip_0003.mp4")


# How many clips of the fixture folder the gated scenario loads. The folder holds
# 83 clips; generation is two rvio jobs per clip at MAX_WORKERS=2, and the gated
# scenario runs once per gate, so the full folder is exercised separately by
# run_folder_thumbnails_all.sh after the gates pass instead.
SM_FOLDER_CLIP_COUNT = int(os.environ.get("SM_FOLDER_CLIP_COUNT", "12"))

# Source-row preview widget size (mirrors SOURCE_PREVIEW_WIDTH/HEIGHT in the mode).
SOURCE_PREVIEW_WIDTH = 80
SOURCE_PREVIEW_HEIGHT = 45

# Panel grab size — fixed logical size for deterministic pixel golden.
PANEL_GRAB_W = 400
PANEL_GRAB_H = 600
NAV_GRAB_W = 400
NAV_GRAB_H = 40

# Source preview box (session_manager.mu.in SOURCE_PREVIEW_WIDTH/HEIGHT).
SOURCE_PREVIEW_WIDTH = 80
SOURCE_PREVIEW_HEIGHT = 45

# Movieproc templates — %s filled with param string.
MOVIEPROC_FMT = "%s.movieproc"


# ---------------------------------------------------------------------------
# Mode helpers
# ---------------------------------------------------------------------------

def activate_session_manager(log=None) -> None:
    """Activate session_manager and confirm the dock widget is accessible.

    Key constraint: the installed RV's session_manager calls deleteLater() on its
    dock within ~1ms of activation (its lazy update timer fires, detects no viewable
    nodes, and hides/destroys the dock). Pumping the event loop after activateMode
    triggers this deletion. Solution: find the dock BEFORE pumping, then store a
    direct reference the caller can use via find_dock_widget() (re-calls findChild
    each time for a fresh C++ wrapper).

    Callers should add any needed sources BEFORE calling activate_session_manager,
    so the mode sees media and does NOT auto-deactivate.
    """
    if log:
        log("activateMode", SM_MODE)
    try:
        rvc.activateMode(SM_MODE)
    except Exception as e:
        if log:
            log("activateMode raised (non-fatal):", e)
    # Minimal pump — enough for the dock to be created but NOT for the lazy timer to fire.
    pump(50)
    dock = find_dock_widget(log=log)
    assert dock is not None, (
        "session_manager dock widget not found after activateMode. "
        "Ensure ModeManagerPreload=session_manager is in the RV flags "
        "(run_scenario.py handles this automatically)."
    )
    if log:
        log("session_manager dock found and accessible")


def deactivate_session_manager(log=None) -> None:
    if rvc.isModeActive(SM_MODE):
        if log:
            log("deactivateMode", SM_MODE)
        rvc.deactivateMode(SM_MODE)
    pump(300)


# ---------------------------------------------------------------------------
# Widget finders — all search within the dock or main window
# ---------------------------------------------------------------------------

def find_dock_widget(log=None) -> QtWidgets.QDockWidget | None:
    """Find the session manager QDockWidget.

    Searches by objectName first, then falls back to window title
    ("Session Manager") to handle installed-RV builds whose dock
    objectName may be empty or differ from the source-tree name.
    """
    _TITLE_VARIANTS = ("Session Manager", "session_manager", SM_MODE)

    def _is_sm_dock(dock):
        if dock.objectName() == SM_MODE:
            return True
        return dock.windowTitle() in _TITLE_VARIANTS

    # Scan allWidgets first and validate each candidate by touching it. The mode
    # tears down and recreates its dock as the session changes, and findChild()
    # will hand back a wrapper for one that is already destroyed in C++ -- every
    # later call on it (or on a widget looked up through it) then raises
    # "Internal C++ object already deleted".
    app = QtWidgets.QApplication.instance()
    live: list[tuple[int, int, QtWidgets.QDockWidget]] = []
    for w in app.allWidgets():
        try:
            if not isinstance(w, QtWidgets.QDockWidget):
                continue
            if not _is_sm_dock(w):
                continue
            live.append((int(w.isVisible()), int(w.widget() is not None), w))
        except RuntimeError:
            continue
    if live:
        live.sort(key=lambda c: (-c[0], -c[1]))
        best = live[0]
        if log:
            log("dock found via allWidgets, visible:", bool(best[0]),
                f"({len(live)} candidate(s))")
        return best[2]

    win = qtutils.sessionWindow()
    if win is not None:
        # Prefer by objectName.
        dock = win.findChild(QtWidgets.QDockWidget, SM_MODE)
        if dock is not None:
            if log:
                log("dock found by objectName via sessionWindow", dock.isVisible())
            return dock
        # Fall back: any dock whose title matches.
        for dock in win.findChildren(QtWidgets.QDockWidget):
            if _is_sm_dock(dock):
                if log:
                    log("dock found by title", repr(dock.windowTitle()), dock.isVisible())
                return dock
        # Last resort: the only dock widget in the window (session_manager is
        # typically the only dock in a -noPrefs headless launch).
        all_docks = win.findChildren(QtWidgets.QDockWidget)
        if len(all_docks) == 1:
            dock = all_docks[0]
            if log:
                log("dock found (sole dock, objectName={!r})".format(dock.objectName()), dock.isVisible())
            return dock

    # Also scan all top-level widgets.
    app = QtWidgets.QApplication.instance()
    for w in app.topLevelWidgets():
        if w is win:
            continue
        dock = w.findChild(QtWidgets.QDockWidget, SM_MODE)
        if dock is not None:
            if log:
                log("dock found in top-level widget", w.objectName(), dock.isVisible())
            return dock
        for dock in w.findChildren(QtWidgets.QDockWidget):
            if _is_sm_dock(dock):
                if log:
                    log("dock found by title in top-level", dock.windowTitle())
                return dock

    if log:
        log("dock NOT found")
    return None


def find_base_widget(log=None) -> QtWidgets.QWidget | None:
    """Return the active session_manager content widget (objectName 'sessionManager').

    Uses QApplication.allWidgets() directly — the most reliable approach because
    any parent-chain traversal (dock.widget(), sessionWindow.findChild) is fragile
    when the session_manager rebuilds its widget hierarchy via deleteLater cycles.
    Prefers visible widgets so stale old instances are ranked lower.

    Single allWidgets() pass, no nested lookups: a second scan from inside this
    function invalidates the wrappers collected by the first, and the widget
    returned then raises "already deleted" on first use.
    """
    app = QtWidgets.QApplication.instance()
    candidates = []
    for w in app.allWidgets():
        try:
            if w.objectName() == "sessionManager":
                vis = w.isVisible()
                candidates.append((vis, w))
        except RuntimeError:
            continue
    if not candidates:
        if log:
            log("base widget NOT found")
        return None
    candidates.sort(key=lambda x: -int(x[0]))  # prefer visible
    best = candidates[0][1]
    if log:
        log("base widget found via allWidgets, visible:", candidates[0][0],
            f"({len(candidates)} candidate(s))")
    return best


def _find_titlebar(log=None) -> QtWidgets.QWidget | None:
    """Return the dock titleBarWidget (objectName 'navPanel')."""
    app = QtWidgets.QApplication.instance()
    for w in app.allWidgets():
        try:
            if w.objectName() == "navPanel":
                return w
        except RuntimeError:
            continue
    # Fall back: look inside the dock.
    dock = find_dock_widget(log=log)
    if dock is not None:
        try:
            return dock.titleBarWidget()
        except RuntimeError:
            pass
    return None


def _is_descendant_of(widget, ancestor) -> bool:
    try:
        node = widget.parentWidget()
        while node is not None:
            if node is ancestor:
                return True
            node = node.parentWidget()
    except RuntimeError:
        return False
    return False


def _find_child(name: str, cls=QtWidgets.QWidget, log=None):
    """Find a named widget inside the session_manager UI via allWidgets().

    Goes directly to QApplication.allWidgets() to avoid parent-chain traversal
    and stale-reference issues when the session_manager rebuilds its widget tree.

    Kept to a single allWidgets() pass with no nested lookups. Scanning again from
    inside this function (e.g. to resolve the live dock and prefer its
    descendants) invalidates the wrappers this pass just collected, so the widget
    returned then raises "Internal C++ object already deleted" on first use even
    though the underlying panel is alive and well.
    """
    app = QtWidgets.QApplication.instance()
    fallback = None
    for w in app.allWidgets():
        try:
            if w.objectName() != name:
                continue
            if not isinstance(w, cls):
                continue
            if w.isVisible():
                return w
            if fallback is None:
                fallback = w
        except RuntimeError:
            continue
    return fallback


def find_tree_view(log=None):
    """Return the session tree QTreeView (unnamed, uses QStandardItemModel).

    Searches allWidgets() rather than going through the dock/base widget chain,
    because intermediate widget wrappers may be stale after the session_manager
    fires its lazy-update cycle. The session tree is the only unnamed QTreeView
    (not QTreeWidget) whose model is a QStandardItemModel with invisibleRootItem.
    """
    from qt_scenario_utils import QtCore
    app = QtWidgets.QApplication.instance()
    candidates = []
    for w in app.allWidgets():
        try:
            if not isinstance(w, QtWidgets.QTreeView):
                continue
            if isinstance(w, QtWidgets.QTreeWidget):
                continue
            if w.objectName():
                continue
            model = w.model()
            if model is None:
                continue
            if not hasattr(model, "invisibleRootItem"):
                continue
            root = model.invisibleRootItem()
            if root is None:
                continue
            candidates.append((w, root.rowCount()))
        except RuntimeError:
            continue
    # Prefer the candidate with the most rows (populated tree over empty one).
    if not candidates:
        if log:
            log("tree view NOT found via allWidgets")
        return None
    candidates.sort(key=lambda x: -x[1])
    tv = candidates[0][0]
    if log:
        log("tree view found via allWidgets, rows:", candidates[0][1])
    return tv


def find_inputs_view(log=None):
    """Return the InputsView (QListView, objectName 'inputsViewList') via allWidgets."""
    app = QtWidgets.QApplication.instance()
    for w in app.allWidgets():
        try:
            if w.objectName() == "inputsViewList" and isinstance(w, QtWidgets.QListView):
                if log:
                    log("inputs view found via allWidgets")
                return w
        except RuntimeError:
            continue
    if log:
        log("inputs view NOT found")
    return None


def find_button(name: str, log=None) -> QtWidgets.QToolButton | None:
    btn = _find_child(name, QtWidgets.QToolButton, log=log)
    if btn is not None:
        try:
            _ = btn.isEnabled()  # validity check
        except RuntimeError:
            btn = None
    return btn


def find_add_button(log=None):    return find_button("addButton", log=log)
def find_folder_button(log=None): return find_button("folderButton", log=log)
def find_delete_button(log=None): return find_button("deleteButton", log=log)
def find_config_button(log=None): return find_button("configButton", log=log)
def find_rename_button(log=None): return find_button("renameButton", log=log)
def find_home_button(log=None):   return find_button("selectCurrentButton", log=log)
def find_prev_button(log=None):   return find_button("prevViewButton", log=log)
def find_next_button(log=None):   return find_button("nextViewButton", log=log)
def find_order_up_button(log=None):     return find_button("orderUpButton", log=log)
def find_order_down_button(log=None):   return find_button("orderDownButton", log=log)
def find_sort_asc_button(log=None):     return find_button("sortAscButton", log=log)
def find_sort_desc_button(log=None):    return find_button("sortDescButton", log=log)
def find_inputs_delete_button(log=None): return find_button("inputsDeleteButton", log=log)


def select_inputs_tab(log=None) -> None:
    """Ensure the session manager's inputs tab (index 0) is active.

    The tabWidget can be on the viewUITab (index 1) if a source group was
    previously viewed. Calling this before clicking inputs-panel buttons
    prevents intermittent 'button not visible' failures when running the full
    suite sequentially.
    """
    app = QtWidgets.QApplication.instance()
    for w in app.allWidgets():
        try:
            if isinstance(w, QtWidgets.QTabWidget) and w.objectName() == "tabWidget":
                if w.currentIndex() != 0:
                    w.setCurrentIndex(0)
                    pump(100)
                    if log:
                        log("select_inputs_tab: switched to inputsTab (was index 1)")
                else:
                    if log:
                        log("select_inputs_tab: already on inputsTab")
                return
        except RuntimeError:
            continue
    if log:
        log("select_inputs_tab: tabWidget not found")


def find_view_label(log=None) -> QtWidgets.QLabel | None:
    """Find the viewLabel QLabel in the nav bar via allWidgets (stale-ref-safe)."""
    app = QtWidgets.QApplication.instance()
    for w in app.allWidgets():
        try:
            if w.objectName() == "viewLabel" and isinstance(w, QtWidgets.QLabel):
                return w
        except RuntimeError:
            continue
    return _find_child("viewLabel", QtWidgets.QLabel, log=log)


def find_tab_widget(log=None) -> QtWidgets.QTabWidget | None:
    return _find_child("tabWidget", QtWidgets.QTabWidget, log=log)


# ---------------------------------------------------------------------------
# Movieproc source creation helpers (via command API — bypasses dialog)
# ---------------------------------------------------------------------------

def add_movieproc_source(fmtspec: str, params: str, name: str, log=None) -> str:
    """Add a movieproc source and return the source node name.

    fmtspec: e.g. "black" → full URL "black,width=1280,...movieproc"
    """
    url = f"{fmtspec},{params}.movieproc"
    if log:
        log("addSourceVerbose", url)
    snode = rvc.addSourceVerbose([url])
    pump(400)
    group = rvc.nodeGroup(snode)
    set_ui_name(group, name)
    pump(200)
    if log:
        log("created", group, "media", rvc.getStringProperty(snode + ".media.movie"))
    return group


def add_black_source(log=None) -> str:
    return add_movieproc_source(
        "black",
        "width=1280,height=720,fps=24,start=1,end=24,red=0,green=0,blue=0",
        "Black",
        log=log,
    )


def add_white_source(log=None) -> str:
    return add_movieproc_source(
        "solid",
        "width=1280,height=720,fps=24,start=1,end=24,red=1,green=1,blue=1",
        "White",
        log=log,
    )


def add_bars_source(log=None) -> str:
    return add_movieproc_source(
        "smptebars",
        "width=1280,height=720,fps=24,start=1,end=24",
        "SMPTEBars",
        log=log,
    )


def add_base_source(log=None) -> str:
    """A neutral white source, so the panel has a viewable node to render before
    the case under test is added.

    session_manager hides and destroys its dock when viewNodes() is empty, so a
    "before" capture is only possible once some source already exists.
    """
    return add_movieproc_source(
        "solid",
        "width=1280,height=720,fps=24,start=1,end=24,red=1,green=1,blue=1",
        "Base",
        log=log,
    )


def add_colorchart_source(log=None) -> str:
    return add_movieproc_source(
        "srgbcolorchart",
        "width=1280,height=720,fps=24,start=1,end=24",
        "SRGBColorChart",
        log=log,
    )


# ---------------------------------------------------------------------------
# Tree inspection helpers
# ---------------------------------------------------------------------------

def _safe_tree_view(tree_view=None, log=None):
    """Return a valid QTreeView, re-finding it if the given ref is stale."""
    if tree_view is not None:
        try:
            _ = tree_view.model()  # validity check
            return tree_view
        except RuntimeError:
            pass
    return find_tree_view(log=log)


def _safe_inputs_view(inputs_view=None, log=None):
    """Return a valid QListView for inputs, re-finding if the given ref is stale."""
    if inputs_view is not None:
        try:
            _ = inputs_view.model()
            return inputs_view
        except RuntimeError:
            pass
    return find_inputs_view(log=log)


def tree_category_items(tree_view=None, log=None) -> dict[str, list[str]]:
    """Return {category_name: [child_node_uinames]} from the tree model.

    Accepts a tree_view widget or None (auto-finds it). Handles stale refs.
    """
    tv = _safe_tree_view(tree_view, log=log)
    if tv is None:
        return {}
    try:
        model = tv.model()
    except RuntimeError:
        return {}
    if model is None:
        return {}
    root = model.invisibleRootItem()
    result: dict[str, list[str]] = {}
    for row in range(root.rowCount()):
        cat_item = root.child(row, 0)
        if cat_item is None:
            continue
        cat_name = cat_item.text()
        children = []
        for crow in range(cat_item.rowCount()):
            child = cat_item.child(crow, 0)
            if child is not None:
                children.append(child.text())
        result[cat_name] = children
    if log:
        log("tree categories", result)
    return result


def select_tree_item_for_node(tree_view, node: str, log=None) -> bool:
    """Select the first tree item whose node data matches `node`.

    Accepts a tree_view widget or None (auto-finds it). Handles stale refs.
    """
    tv = _safe_tree_view(tree_view, log=log)
    if tv is None:
        return False
    try:
        model = tv.model()
    except RuntimeError:
        return False
    if model is None:
        return False
    root = model.invisibleRootItem()

    def search(parent_item):
        for row in range(parent_item.rowCount()):
            item = parent_item.child(row, 0)
            if item is None:
                continue
            node_data = item.data(QtCore.Qt.UserRole + 2)
            if node_data == node:
                try:
                    idx = model.indexFromItem(item)
                    tv.scrollTo(idx)
                    # Programmatic selection first.
                    tv.setCurrentIndex(idx)
                    tv.selectionModel().select(
                        idx,
                        QtCore.QItemSelectionModel.SelectCurrent
                        | QtCore.QItemSelectionModel.Rows,
                    )
                    # Also simulate a mouse click so any clicked() or pressed()
                    # signal handlers in the installed session_manager fire.
                    rect = tv.visualRect(idx)
                    if rect.isValid() and rect.width() > 0:
                        from qt_scenario_utils import QTest
                        QTest.mouseClick(
                            tv.viewport(),
                            QtCore.Qt.LeftButton,
                            QtCore.Qt.NoModifier,
                            rect.center(),
                        )
                except RuntimeError:
                    pass
                pump(400)
                if log:
                    log("selected item for node", node, "row", row)
                return True
            if search(item):
                return True
        return False

    return search(root)


def get_inputs_node_list(inputs_view=None, log=None) -> list[str]:
    """Return list of node names from the inputs view model (UserRole+2 data).

    Accepts inputs_view widget or None (auto-finds it). Handles stale refs.
    """
    iv = _safe_inputs_view(inputs_view, log=log)
    if iv is None:
        return []
    try:
        model = iv.model()
    except RuntimeError:
        return []
    if model is None:
        return []
    nodes = []
    for row in range(model.rowCount(QtCore.QModelIndex())):
        idx = model.index(row, 0, QtCore.QModelIndex())
        node_data = model.data(idx, QtCore.Qt.UserRole + 2)
        if node_data:
            nodes.append(node_data)
    if log:
        log("inputs nodes", nodes)
    return nodes


def select_inputs_item(inputs_view, row_or_node, log=None) -> bool:
    """Select a single row in the inputs view by row index or node name string.

    Accepts inputs_view widget or None (auto-finds it). Handles stale refs.
    """
    iv = _safe_inputs_view(inputs_view, log=log)
    if iv is None:
        return False
    inputs_view = iv
    try:
        model = inputs_view.model()
    except RuntimeError:
        return False
    if model is None:
        return False
    if isinstance(row_or_node, str):
        # Find the row whose UserRole+2 data matches the node name.
        for r in range(model.rowCount(QtCore.QModelIndex())):
            idx = model.index(r, 0, QtCore.QModelIndex())
            if model.data(idx, QtCore.Qt.UserRole + 2) == row_or_node:
                inputs_view.selectionModel().clearSelection()
                inputs_view.selectionModel().select(idx, QtCore.QItemSelectionModel.Select)
                pump(100)
                if log:
                    log("selected inputs row", r, "for node", row_or_node)
                return True
        if log:
            log("node not found in inputs view:", row_or_node)
        return False
    else:
        idx = model.index(row_or_node, 0, QtCore.QModelIndex())
        inputs_view.selectionModel().clearSelection()
        inputs_view.selectionModel().select(idx, QtCore.QItemSelectionModel.Select)
        pump(100)
        if log:
            log("selected inputs row", row_or_node)
        return True


# ---------------------------------------------------------------------------
# Wait helpers
# ---------------------------------------------------------------------------

def wait_for_preview(source_node: str = "", timeout_s: float = 10.0, log=None) -> bool:
    """Wait for local_thumbnail_gen to produce a preview (best-effort poll).

    rvc.bindToEvent / unbindEvent are not available in the installed RV Python API.
    Falls back to a simple timeout poll.
    """
    start = time.time()
    while time.time() - start < timeout_s:
        pump(200)
        # If loadTotal drops to 0, progressive loading is complete.
        if rvc.loadTotal() == 0:
            pump(400)
            if log:
                log("preview wait done (loadTotal=0)", round(time.time() - start, 1), "s")
            return True
    if log:
        log("preview wait timeout after", timeout_s, "s")
    return False


# Legacy alias used by older scenarios.
wait_for_preview_available = wait_for_preview


# ---------------------------------------------------------------------------
# Source preview (thumbnail / filmstrip) helpers
# ---------------------------------------------------------------------------

def meridian_clips(limit: int | None = None) -> list[str]:
    """Every .mp4 in the fixture folder, sorted by name (stable ordering).

    ``limit`` (or SM_FOLDER_CLIPS) caps how many are used so the folder-load
    scenario has a fixed, reviewable size regardless of what else lands in the
    fixture directory.
    """
    names = sorted(n for n in os.listdir(SM_MERIDIAN_DIR) if n.lower().endswith(".mp4"))
    paths = [os.path.join(SM_MERIDIAN_DIR, n) for n in names]
    if limit is None:
        env = os.environ.get("SM_FOLDER_CLIPS", "")
        limit = int(env) if env.strip().isdigit() else None
    return paths[:limit] if limit else paths


def add_folder_sources(clips: list[str], log=None) -> list[str]:
    """Load a list of media files one at a time and return their source groups.

    Deliberately not ``addSources()``: that queues the files through the
    progressive loader, which never advances in a ``-pyeval`` run (verified
    2026-08-03 — loadTotal stays at the file count and no RVSourceGroup is ever
    created, even while the Qt event loop is pumped for 180s, and even though
    progressiveSourceLoading() is already False). ``addSourceVerbose`` loads
    synchronously and returns the source node, which is both deterministic and
    fast (~0.2s per clip).
    """
    groups = []
    for clip in clips:
        snode = add_source_verbose_group(clip)
        groups.append(snode)
        pump(50)
    if log:
        log("loaded", len(groups), "sources via addSourceVerbose")
    return groups


def add_source_verbose_group(path: str) -> str:
    """addSourceVerbose one media file, returning its enclosing source group."""
    snode = rvc.addSourceVerbose([path])
    return rvc.nodeGroup(snode)


def thumbnail_cache_dir() -> str:
    """local_thumbnail_gen's cache dir for this RV process.

    It keys the directory on the RV pid (see local_thumbnail_gen.py), and the
    scenario runs inside RV, so os.getpid() resolves to the same directory.
    Counting files here is how a scenario knows generation actually finished:
    rvio runs in worker threads, so there is no synchronous "all done" call.
    """
    import tempfile

    return os.path.join(tempfile.gettempdir(), f"rv_thumbnails_{os.getpid()}")


def _cache_counts(cache_dir: str) -> tuple[int, int]:
    """(thumbnails, filmstrips) fully written in the cache.

    Zero-length files are not counted: rvio is suspended mid-write while playback
    defers generation, which leaves a partial file behind that would otherwise read
    as a finished preview.
    """
    try:
        names = os.listdir(cache_dir)
    except OSError:
        return (0, 0)

    def done(name: str) -> bool:
        try:
            return os.path.getsize(os.path.join(cache_dir, name)) > 0
        except OSError:
            return False

    thumbs = sum(1 for n in names if n.endswith("_thumbnail.jpg") and done(n))
    strips = sum(1 for n in names if n.endswith("_filmstrip.jpg") and done(n))
    return (thumbs, strips)


def wait_for_all_previews(expected: int, timeout_s: float = 900.0, log=None) -> tuple[int, int]:
    """Block until every source has a generated thumbnail *and* filmstrip.

    Waiting for the filmstrips too, not just the thumbnails, is what makes the
    capture deterministic: each completed job fires
    ``session-manager-preview-available``, which rebuilds that row's widget. A
    grab taken while jobs are still landing can catch a half-rebuilt panel.
    """
    cache_dir = thumbnail_cache_dir()
    start = time.time()
    thumbs = strips = 0
    while time.time() - start < timeout_s:
        thumbs, strips = _cache_counts(cache_dir)
        if thumbs >= expected and strips >= expected:
            break
        pump(500)
    else:
        # Returning here would grab a panel still showing fallback icons and
        # commit that as the golden, which pins the opposite of what is intended.
        raise AssertionError(
            f"preview generation did not finish within {timeout_s}s: "
            f"{thumbs}/{expected} thumbnails, {strips}/{expected} filmstrips in {cache_dir}"
        )
    # Let the queued preview-available events rebuild every row before returning.
    pump(3000)
    if log:
        log("preview generation finished:", thumbs, "thumbnails,", strips, "filmstrips in",
            round(time.time() - start, 1), "s (expected", expected, "each)")
    return (thumbs, strips)


def find_config_menu(log=None):
    """Return the config QToolButton's menu (Always/Never/Restore + previews)."""
    btn = find_config_button(log=log)
    assert btn is not None, "configButton not found"
    return open_tool_button_menu(btn)


def toggle_previews(log=None) -> bool:
    """Flip Config > Show Source Previews and return the new checked state.

    Drains pending events first: a session-manager-preview-available event landing
    while the menu is open rebuilds the panel and destroys the config button's
    menu under us ("Internal C++ object already deleted").
    """
    pump(600)
    menu = find_config_menu(log=log)
    target = None
    for action in menu.actions():
        if action.text().replace("&", "") == "Show Source Previews":
            target = action
            break
    assert target is not None, (
        "Show Source Previews action not found; available: "
        f"{[a.text() for a in menu.actions()]}"
    )
    assert target.isEnabled(), (
        "Show Source Previews is disabled — RV_SESSION_MANAGER_USE_THUMBNAILS=0 "
        "forces previews off, so this scenario cannot toggle them"
    )
    was = target.isChecked()
    target.trigger()
    pump(800)
    menu.close()
    pump(300)
    now = target.isChecked()
    assert now != was, f"toggling previews did not change the action state (still {now})"
    if log:
        log("previews toggled", was, "->", now)
    return now


def source_row_widgets(log=None) -> list[QtWidgets.QWidget]:
    """Every per-source row widget currently in the tree ('sourceRowWidget')."""
    app = QtWidgets.QApplication.instance()
    rows = []
    for w in app.allWidgets():
        try:
            if w.objectName() == "sourceRowWidget":
                rows.append(w)
        except RuntimeError:
            continue
    if log:
        log("source row widgets:", len(rows))
    return rows


def wait_for_rows_with_thumbnails(
    expected: int, fallback_hash: str, timeout_s: float = 300.0, log=None
) -> int:
    """Wait until `expected` preview labels have replaced the fallback image.

    The generated files landing in the cache is not the same event as the rows
    repainting: the mode rebuilds each row from a queued
    session-manager-preview-available event, several event loop turns later.
    """
    start = time.time()
    shown = 0
    while time.time() - start < timeout_s:
        shown = tree_rows_with_thumbnails(fallback_hash, log=None)
        if shown >= expected:
            break
        pump(400)
    else:
        raise AssertionError(
            f"only {shown}/{expected} rows replaced the fallback image within {timeout_s}s"
        )
    pump(1500)
    if log:
        log("rows showing generated frames:", shown, "after", round(time.time() - start, 1), "s")
    return shown


def _preview_labels_of_row(row_widget) -> list[QtWidgets.QLabel]:
    """The visible thumbnail label of one source row.

    A sourceRowWidget holds four labels: sourceNameLabel, sourceMetaLabel, and the
    ThumbnailWidget plus FilmstripWidget inside the unnamed SourcePreviewWidget.
    Only the thumbnail is wanted -- the name/meta labels carry text, and the
    filmstrip is kept hidden until hover, so including any of them makes a
    "one image per source" check meaningless.
    """
    found = []
    try:
        children = row_widget.findChildren(QtWidgets.QLabel)
    except RuntimeError:
        return found
    for label in children:
        try:
            if label.objectName():
                continue
            if (label.width(), label.height()) != (SOURCE_PREVIEW_WIDTH, SOURCE_PREVIEW_HEIGHT):
                continue
            if not label.isVisible():
                continue
        except RuntimeError:
            continue
        found.append(label)
    return found


def tree_source_row_previews(tree_view=None, log=None) -> list[QtWidgets.QLabel]:
    """Preview labels installed as index widgets in the tree, right now.

    Scoped to the tree rather than QApplication.allWidgets() on purpose: toggling
    previews rebuilds the tree, and the discarded row widgets stay reachable
    through allWidgets() until their deleteLater runs. Counting those would make
    "previews are off" look like "previews are on" depending on when the event
    loop got around to the deletions.
    """
    tv = _safe_tree_view(tree_view, log=log)
    if tv is None:
        return []
    try:
        model = tv.model()
    except RuntimeError:
        return []
    if model is None:
        return []
    labels: list[QtWidgets.QLabel] = []

    def walk(parent_item):
        for row in range(parent_item.rowCount()):
            item = parent_item.child(row, 0)
            if item is None:
                continue
            try:
                widget = tv.indexWidget(model.indexFromItem(item))
            except RuntimeError:
                widget = None
            if widget is not None and widget.objectName() == "sourceRowWidget":
                labels.extend(_preview_labels_of_row(widget))
            walk(item)

    walk(model.invisibleRootItem())
    if log:
        log("preview labels in tree row widgets:", len(labels))
    return labels


def tree_row_preview_hashes(tree_view=None, log=None) -> list[str]:
    """Content hash of every preview pixmap currently installed in the tree.

    Hashing the pixels is the only reliable way to tell the fallback icon from a
    generated frame. Pixmap size does not work: the fallback is requested at the
    preview box size but comes back at the display's device pixel ratio (160x45*2
    on a Retina panel), so "bigger than the box" is true for the fallback too and
    every row looks generated before anything has been generated.
    """
    hashes = []
    for label in tree_source_row_previews(tree_view, log=None):
        try:
            pixmap = label.pixmap()
            if pixmap is None or pixmap.isNull():
                continue
            image = pixmap.toImage()
            data = bytes(image.constBits())
        except (RuntimeError, TypeError):
            continue
        hashes.append(hashlib.sha1(data).hexdigest())
    if log:
        log("preview pixmap hashes:", len(hashes), "distinct:", len(set(hashes)))
    return hashes


def fallback_preview_hash(tree_view=None, log=None) -> str:
    """The hash shared by every row while no thumbnail has been generated yet.

    Calibrated from the live panel instead of hardcoded or read off disk, so it
    stays correct across icon changes and display scaling.
    """
    hashes = tree_row_preview_hashes(tree_view, log=None)
    assert hashes, "no preview pixmaps found to calibrate the fallback hash"
    distinct = set(hashes)
    assert len(distinct) == 1, (
        f"expected every row to show the same fallback image, found {len(distinct)} "
        "distinct images — generation already produced a thumbnail, so this state "
        "is not a usable baseline"
    )
    fallback = hashes[0]
    if log:
        log("fallback preview hash:", fallback[:12], "across", len(hashes), "labels")
    return fallback


def tree_rows_with_thumbnails(fallback_hash: str, tree_view=None, log=None) -> int:
    """Preview labels showing something other than the fallback image."""
    count = sum(1 for h in tree_row_preview_hashes(tree_view, log=None) if h != fallback_hash)
    if log:
        log("preview labels showing a generated frame:", count)
    return count


def loaded_thumbnail_count(log=None) -> int:
    """Count source rows whose preview label is showing a real (non-fallback) image.

    The fallback and a generated thumbnail are both QPixmaps on the same QLabel
    (ThumbnailWidget.load keeps the decoded image at its native size and relies on
    scaledContents for display), so they are told apart by size: the fallback is
    built at exactly the preview box size, a decoded movie frame is far larger.
    """
    count = 0
    for row in source_row_widgets(log=None):
        try:
            labels = row.findChildren(QtWidgets.QLabel)
        except RuntimeError:
            continue
        for label in labels:
            try:
                pixmap = label.pixmap()
            except RuntimeError:
                continue
            if pixmap is None or pixmap.isNull():
                continue
            if pixmap.width() > SOURCE_PREVIEW_WIDTH:
                count += 1
                break
    if log:
        log("rows showing a generated thumbnail:", count)
    return count


def wait_for_progressive_loading(
    timeout_s: float = 600.0, log=None, use_native: bool = False
) -> None:
    """Wait until progressive loading completes, pumping the event loop.

    Deliberately does NOT call rvc.waitForProgressiveLoading() by default. That
    native call blocks the calling thread, so on a real display (no Xvfb) it
    deadlocks: the loader needs the main event loop to keep running to finish, and
    the blocking wait is what stops it running. Polling loadTotal() while pumping
    Qt events reaches the same state without the deadlock -- this is what lets the
    mp4 scenarios run under the GUI sanity gate instead of being skipped.

    Requires two consecutive idle polls, since loadTotal() reads 0 in the gap
    between one source finishing and the next being queued.
    """
    start = time.time()
    if use_native and hasattr(rvc, "waitForProgressiveLoading"):
        rvc.waitForProgressiveLoading()
        pump(600)
        if log:
            log("progressive loading done via native wait", round(time.time() - start, 1), "s")
        return
    idle_polls = 0
    while time.time() - start < timeout_s:
        if rvc.loadTotal() == 0:
            idle_polls += 1
            if idle_polls >= 2:
                break
        else:
            idle_polls = 0
        pump(300)
    else:
        raise AssertionError(
            f"progressive loading did not finish within {timeout_s}s "
            f"(loadTotal={rvc.loadTotal()})"
        )
    pump(600)
    if log:
        log("progressive loading done in", round(time.time() - start, 1), "s")


# ---------------------------------------------------------------------------
# PNG grab helpers
# ---------------------------------------------------------------------------

def grab_widget_png(widget, out_dir: str, name: str, w: int, h: int, log=None) -> str:
    """Grab widget at fixed logical size w×h and save to out_dir/name.png."""
    assert widget is not None, f"grab_widget_png: widget for {name} is None"
    widget.setFixedSize(w, h)
    pump(300)
    pixmap = widget.grab()
    if pixmap.width() != w or pixmap.height() != h:
        pixmap = pixmap.scaled(
            w, h,
            QtCore.Qt.IgnoreAspectRatio,
            QtCore.Qt.FastTransformation,
        )
    path = os.path.join(out_dir, name)
    ok = pixmap.save(path, "PNG")
    assert ok, f"grab_widget_png: failed to save {path}"
    if log:
        log("saved", path, pixmap.width(), pixmap.height())
    return path


def grab_panel_png(out_dir: str, name: str = "panel.png", log=None) -> str:
    """Grab the main session manager panel (base widget)."""
    base = find_base_widget(log=log)
    assert base is not None, "panel widget not found"
    return grab_widget_png(base, out_dir, name, PANEL_GRAB_W, PANEL_GRAB_H, log=log)


def grab_nav_png(out_dir: str, name: str = "nav.png", log=None) -> str:
    """Grab the nav bar (prevButton + label + nextButton)."""
    nav = _find_child("navPanel", QtWidgets.QWidget, log=log)
    if nav is None:
        dock = find_dock_widget(log=log)
        if dock:
            nav = dock.titleBarWidget()
    assert nav is not None, "navPanel not found"
    return grab_widget_png(nav, out_dir, name, NAV_GRAB_W, NAV_GRAB_H, log=log)


def assert_images_differ(path_a: str, path_b: str, what: str = "", log=None) -> None:
    """Fail the scenario unless two captured PNGs actually differ.

    VERIFICATION.md Primary outcomes rule 2: a user-visible outcome must be pinned
    by two viewport states that must differ. Without this check a scenario can
    happily commit a before/after pair that shows the same thing, which pins
    nothing and silently passes the pixel gate forever after.
    """
    for path in (path_a, path_b):
        assert os.path.isfile(path), f"assert_images_differ: missing {path}"
    with open(path_a, "rb") as fa, open(path_b, "rb") as fb:
        same_bytes = fa.read() == fb.read()
    if not same_bytes:
        if log:
            log("images differ OK:", os.path.basename(path_a), "vs", os.path.basename(path_b), what)
        return
    raise AssertionError(
        f"before/after captures are identical ({os.path.basename(path_a)} == "
        f"{os.path.basename(path_b)}) — the outcome under test is not visible: {what}"
    )


# ---------------------------------------------------------------------------
# Session save
# ---------------------------------------------------------------------------

def save_session(out_dir: str, log=None) -> None:
    path = os.path.join(out_dir, "session.rv")
    if log:
        log("saveSession", path)
    rvc.saveSession(path, True, False, False)


# ---------------------------------------------------------------------------
# Folder-of-clips thumbnail flow (shared by the gated and full-folder scenarios)
# ---------------------------------------------------------------------------

def folder_thumbnail_flow(out_dir: str, clip_limit=None, log=None) -> None:
    """Load a folder of mp4s and pin the fully-generated thumbnail panel.

    Shared by sm_folder_thumbnails (a 12-clip subset, cheap enough to run once per
    gate) and sm_folder_thumbnails_all (the whole folder, run once after the gates
    pass), so the two cannot drift apart.

    Both halves of the pixel pair are quiescent, which is what allows a -dmax 0
    gate over an asynchronous pipeline:

      panel_fallback.png    every row on the fallback icon, with generation held
                            off by the session-manager-previews-disabled internal
                            event before any source exists. Nothing has been
                            generated at all at that point (asserted against the
                            cache), so the grab cannot race a finishing job.
      panel_thumbnails.png  generation re-enabled and every thumbnail *and*
                            filmstrip written, so no further preview-available
                            event can rebuild a row after the grab.

    Two routes to the fallback state were tried and rejected. The Config > Show
    Source Previews menu action cannot be driven here: under the memory churn of a
    folder-sized preview run the Mu mode's config QMenu gets collected (unlike
    _folderMenu, no member holds it -- only Qt parenting), so every use raises
    "Internal C++ object already deleted". sm_previews_toggle covers the menu path
    on a single source, where the mode survives it. Running playback to defer
    generation is not deterministic either: 6 of 12 clips still finished during
    playback, and stopping mid-clip writes a varying current frame into session.rv.
    """
    clips = meridian_clips()
    if log:
        log("fixture dir:", SM_MERIDIAN_DIR, "clips in folder:", len(clips))
    if clip_limit is not None:
        clips = clips[:clip_limit]
    assert len(clips) >= 2, f"expected a folder of mp4s in {SM_MERIDIAN_DIR}, found {len(clips)}"
    for clip in clips:
        assert os.path.exists(clip), f"media fixture not found: {clip}"
    if log:
        log("loading", len(clips), "clips")

    # Hold generation off before any source exists, so not one job is ever
    # submitted and the fallback state below is exact rather than "whatever had not
    # finished yet".
    rvc.sendInternalEvent("session-manager-previews-disabled", "")
    pump(300)

    add_folder_sources(clips, log=log)
    pump(1000)

    source_groups = [n for n in rvc.nodes() if rvc.nodeType(n) == "RVSourceGroup"]
    assert len(source_groups) == len(clips), (
        f"expected {len(clips)} sources, got {len(source_groups)}"
    )

    rvc.setViewNode(source_groups[0])
    pump(300)

    activate_session_manager(log=log)
    pump(1500)

    cats = tree_category_items(None, log=log)
    assert "SOURCES" in cats, f"SOURCES not in tree: {list(cats.keys())}"
    assert len(cats["SOURCES"]) == len(clips), (
        f"tree should list every clip: {len(cats['SOURCES'])} rows for {len(clips)} clips"
    )

    # --- before half: every row still on the fallback icon --------------------
    labels = tree_source_row_previews(log=log)
    assert len(labels) == len(clips), (
        f"expected one preview per clip, found {len(labels)} for {len(clips)} clips"
    )
    pre_thumbs, pre_strips = _cache_counts(thumbnail_cache_dir())
    assert (pre_thumbs, pre_strips) == (0, 0), (
        f"generation was supposed to be held off, but the cache already holds "
        f"{pre_thumbs} thumbnails and {pre_strips} filmstrips"
    )
    fallback_hash = fallback_preview_hash(log=log)
    panel_fallback = grab_panel_png(out_dir, "panel_fallback.png", log=log)

    # --- after half: generation re-enabled and finished for every clip --------
    rvc.sendInternalEvent("session-manager-previews-enabled", "")
    pump(500)

    thumbs, strips = wait_for_all_previews(len(clips), log=log)
    assert thumbs >= len(clips), f"only {thumbs}/{len(clips)} thumbnails generated"
    assert strips >= len(clips), f"only {strips}/{len(clips)} filmstrips generated"
    shown = wait_for_rows_with_thumbnails(len(clips), fallback_hash, log=log)
    assert shown == len(clips), (
        f"only {shown}/{len(clips)} rows show a generated frame (rest still fallback)"
    )
    # Each clip is a different scene, so identical thumbnails would mean rows are
    # sharing one image rather than each showing its own media.
    row_hashes = tree_row_preview_hashes(log=log)
    assert len(set(row_hashes)) == len(clips), (
        f"{len(clips)} clips but only {len(set(row_hashes))} distinct thumbnails"
    )

    media_exts = set()
    for group in source_groups:
        for node in rvc.nodesInGroup(group):
            if rvc.nodeType(node) in ("RVFileSource", "RVImageSource"):
                movie = rvc.getStringProperty(node + ".media.movie")[0]
                media_exts.add(os.path.basename(movie).rsplit(".", 1)[-1])
    assert media_exts == {"mp4"}, f"expected only mp4 sources, got {sorted(media_exts)}"

    panel_thumbs = grab_panel_png(out_dir, "panel_thumbnails.png", log=log)
    assert_images_differ(
        panel_fallback, panel_thumbs,
        "generated thumbnails replace the fallback icons", log=log,
    )
    save_session(out_dir, log=log)
