#
# Shared helpers for doc_browser golden scenarios.
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
from __future__ import annotations

import os

import rv.commands as rvc
import rv.qtutils as qtutils
from qt_scenario_utils import QtCore, QtWidgets, click_menu_action, pump

MODE_RUNTIME_NAME = "doc_browser"
HELP_MODE_NAME = "help"
WINDOW_OBJECT_NAME = "docBrowser"
SEARCH_EDIT_NAME = "searchEdit"
HELP_MENU_BROWSER_ITEM = "   Mu Command API Browser..."
# Stable symbols present in every RV Mu runtime (verified via harness diag).
MODULE_SYMBOL = "commands"
FUNCTION_SYMBOL = "commands.addSources"
TYPE_MODULE = "rvtypes"
TYPE_SYMBOL = "MinorMode"
METHOD_SYMBOL = "init"
CONSTANT_MODULE = "math"
CONSTANT_SYMBOL = "pi"
ASCIIDOC_MODULE = "asciidoc_to_html"
DOC_BROWSER_MODULE = "doc_browser"
WEBENGINE_SETTLE_MS = 1500
# Pinned grab size — matches committed golden-mac/ PNG dimensions; avoids monitor/DPI drift.
BROWSER_GRAB_W = 860
BROWSER_GRAB_H = 1343


def activate_doc_browser(log=None) -> None:
    if log:
        log("activateMode", MODE_RUNTIME_NAME)
    rvc.activateMode(MODE_RUNTIME_NAME)
    assert rvc.isModeActive(MODE_RUNTIME_NAME), f"{MODE_RUNTIME_NAME} not active"
    pump(WEBENGINE_SETTLE_MS)


def deactivate_doc_browser(log=None) -> None:
    if log:
        log("deactivateMode", MODE_RUNTIME_NAME)
    rvc.deactivateMode(MODE_RUNTIME_NAME)
    pump(400)
    assert not rvc.isModeActive(MODE_RUNTIME_NAME), f"{MODE_RUNTIME_NAME} still active"


def activate_via_help_menu(log=None) -> None:
    """Help → Mu Command API Browser (COVERAGE §A2, ``openrv_help_menu`` caller path)."""
    if rvc.isModeActive(MODE_RUNTIME_NAME):
        deactivate_doc_browser(log=log)

    win = qtutils.sessionWindow()
    assert win is not None, "sessionWindow() is None"
    menubar = win.menuBar()
    help_menu = None
    for action in menubar.actions():
        if action.text().replace("&", "") == "Help":
            help_menu = action.menu()
            break

    if help_menu is not None and menubar.isVisible():
        click_menu_action(help_menu, HELP_MENU_BROWSER_ITEM.strip(), settle_ms=400)
        pump(WEBENGINE_SETTLE_MS)
        if log:
            log("triggered Help menu item")
    else:
        # Headless / -nomb: no QMenuBar — activate doc_browser the same way the
        # Help menu handler does (lazy load after ``help`` mode only is preloaded).
        if log:
            log("Help menu unavailable — activateMode (openrv_help_menu equivalent)")
        rvc.activateMode(MODE_RUNTIME_NAME)
        pump(WEBENGINE_SETTLE_MS)

    assert rvc.isModeActive(MODE_RUNTIME_NAME), "doc_browser not active after Help path"


def find_browser_window(log=None):
    app = QtWidgets.QApplication.instance()
    for widget in app.topLevelWidgets():
        if widget.objectName() == WINDOW_OBJECT_NAME:
            if log:
                log("found browser window", widget.objectName(), widget.isVisible())
            return widget
    if log:
        log(
            "browser window not found; topLevelWidgets:",
            [w.objectName() for w in app.topLevelWidgets()],
        )
    return None


def find_doc_browser_widget(window, log=None):
    if window is None:
        return None
    central = window.centralWidget()
    if log:
        log("central widget", type(central).__name__ if central else None)
    return central


def find_column_view(browser_widget, log=None):
    if browser_widget is None:
        return None
    view = browser_widget.findChild(QtWidgets.QColumnView)
    if log:
        log("column view", view is not None)
    return view


def find_search_edit(window, log=None):
    if window is None:
        return None
    search_widget = window.findChild(QtWidgets.QWidget, "searchWidget")
    edit = None
    if search_widget is not None:
        edit = search_widget.findChild(QtWidgets.QLineEdit)
    if edit is None:
        edit = window.findChild(QtWidgets.QLineEdit, SEARCH_EDIT_NAME)
    if log:
        log("search edit", edit is not None)
    return edit


def find_toolbar_button(window, object_name: str, log=None):
    if window is None:
        return None
    btn = window.findChild(QtWidgets.QToolButton, object_name)
    if log:
        log(f"toolbar {object_name}", btn is not None)
    return btn


def select_symbol_path(browser_widget, path: list[str], log=None) -> None:
    """Walk ``QColumnView`` columns selecting ``path[0]``, ``path[1]``, …"""
    from qt_scenario_utils import QtCore

    column_view = find_column_view(browser_widget, log=log)
    assert column_view is not None, "QColumnView not found"
    model = column_view.model()
    assert model is not None, "DocModel not attached"

    parent = QtCore.QModelIndex()
    for depth, name in enumerate(path):
        found = None
        for row in range(model.rowCount(parent)):
            index = model.index(row, 0, parent)
            if index.data() == name:
                found = index
                break
        assert found is not None, f"symbol {name!r} not found at tree depth {depth}"
        column_view.setCurrentIndex(found)
        pump(WEBENGINE_SETTLE_MS)
        parent = found

    current = column_view.currentIndex()
    assert current.isValid(), "column view has no current index after selection"
    assert current.data() == path[-1], f"expected {path[-1]!r}, got {current.data()!r}"
    if log:
        log("selected path", "/".join(path))


def select_symbol_by_display_name(browser_widget, display_name: str, log=None) -> None:
    select_symbol_path(browser_widget, [display_name], log=log)


def run_search(window, text: str, log=None) -> None:
    edit = find_search_edit(window, log=log)
    assert edit is not None, "search QLineEdit not found"
    edit.setFocus()
    edit.setText(text)
    edit.returnPressed.emit()
    pump(WEBENGINE_SETTLE_MS)
    assert edit.text() == text, "search text not set"
    if log:
        log("search submitted", text)


def navigate_mudoc_link(window, url: str, log=None) -> None:
    """Navigate via ``mudoc://`` — uses search box (``DocBrowserMode.search`` path)."""
    assert url.startswith("mudoc://"), f"expected mudoc URL, got {url!r}"
    run_search(window, url, log=log)


def click_toolbar_action(window, object_name: str, log=None) -> None:
    from qt_scenario_utils import click_button

    button = find_toolbar_button(window, object_name, log=log)
    assert button is not None, f"toolbar button {object_name!r} not found"
    click_button(button, settle_ms=WEBENGINE_SETTLE_MS)


def hide_browser_window(log=None) -> None:
    window = find_browser_window(log=log)
    assert window is not None, "browser window not found"
    window.close()
    pump(WEBENGINE_SETTLE_MS)
    if log:
        log("closed browser window")


def grab_browser_png(out_dir: str, log=None) -> tuple[bool, int, int]:
    window = find_browser_window(log=log)
    assert window is not None, "doc browser window not visible"
    assert window.isVisible(), "doc browser window not visible"
    browser = find_doc_browser_widget(window, log=log)
    target = browser if browser is not None else window
    # Pin logical layout; scale device-pixel grabs (Retina) to stable golden size.
    target.setFixedSize(BROWSER_GRAB_W, BROWSER_GRAB_H)
    pump(400)
    path = os.path.join(out_dir, "browser.png")
    pixmap = target.grab()
    if pixmap.width() != BROWSER_GRAB_W or pixmap.height() != BROWSER_GRAB_H:
        pixmap = pixmap.scaled(
            BROWSER_GRAB_W,
            BROWSER_GRAB_H,
            QtCore.Qt.IgnoreAspectRatio,
            QtCore.Qt.FastTransformation,
        )
    ok = pixmap.save(path, "PNG")
    w, h = pixmap.width(), pixmap.height()
    if log:
        log("browser.png", ok, w, h, path)
    assert ok and w == BROWSER_GRAB_W and h == BROWSER_GRAB_H, (
        f"browser.png grab failed or wrong size ({w}x{h}, expected {BROWSER_GRAB_W}x{BROWSER_GRAB_H})"
    )
    return ok, w, h


def save_session(out_dir: str, log=None) -> None:
    path = os.path.join(out_dir, "session.rv")
    if log:
        log("saveSession", path)
    rvc.saveSession(path, True, False, False)
