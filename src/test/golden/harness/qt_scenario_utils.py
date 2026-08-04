#
# Shared Qt helpers for in-RV golden scenarios (package-agnostic).
#
# Every scenario needs the same handful of things: a Qt binding that works on
# both Qt5 and Qt6 builds, a way to pump the event loop (``-pyeval`` runs
# before QCoreApplication::exec(), so nothing paints on its own), and now a
# way to drive *real* widgets with synthetic input instead of calling the
# command each widget is wired to. Plain clicks (QTest.mouseClick) are not
# subject to the drag/drop limitation documented in COVERAGE.md section G
# (synthesized QDropEvents have a null source()) -- only the drag *gesture*
# is blocked headlessly, so button/menu clicks are a genuine way to exercise
# the real UI trigger rather than just pinning its outcome.
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
from __future__ import annotations

import time

try:
    from PySide6 import QtWidgets, QtCore, QtTest
    import shiboken6 as shiboken  # noqa: F401  (parity with existing scenarios)

    QTest = QtTest.QTest
except ImportError:  # pragma: no cover - older Qt
    from PySide2 import QtWidgets, QtCore, QtTest
    import shiboken2 as shiboken  # noqa: F401

    QTest = QtTest.QTest


def pump(ms: int) -> None:
    """Pump the Qt event loop for ~ms without a bare sleep."""
    app = QtWidgets.QApplication.instance()
    end = time.time() + ms / 1000.0
    while time.time() < end:
        app.processEvents(QtCore.QEventLoop.AllEvents, 20)


def click_button(button, settle_ms: int = 300) -> None:
    """Synthesize a real left-click on a QAbstractButton (or subclass).

    Fails loudly on a missing/hidden/disabled target rather than silently
    no-op'ing -- a scenario that can't find the real widget must not report
    a pass, since that would mean the button was never actually exercised.
    """
    if button is None:
        raise AssertionError("click_button: target widget is None")
    if not button.isVisible():
        raise AssertionError(f"click_button: {button.objectName()!r} is not visible")
    if not button.isEnabled():
        raise AssertionError(f"click_button: {button.objectName()!r} is not enabled")
    QTest.mouseClick(button, QtCore.Qt.LeftButton)
    pump(settle_ms)


def open_tool_button_menu(button, settle_ms: int = 300):
    """Show a QToolButton's attached QMenu without relying on InstantPopup click tracking.

    Raises if the button has no menu attached -- that's a wiring change the
    scenario must catch, not silently skip.

    Uses ``QMenu.popup()`` at the button's global position instead of synthesizing
    a tool-button click.  Empirically, ``QTest.mouseClick`` on InstantPopup buttons
    can hang or never show the menu under Xvfb (especially after a prior killed RV
    process); ``popup()`` is deterministic headlessly.
    """
    if button is None:
        raise AssertionError("open_tool_button_menu: target widget is None")
    if not button.isVisible():
        raise AssertionError(f"open_tool_button_menu: {button.objectName()!r} is not visible")
    if not button.isEnabled():
        raise AssertionError(f"open_tool_button_menu: {button.objectName()!r} is not enabled")
    menu = button.menu()
    if menu is None:
        raise AssertionError(
            f"open_tool_button_menu: {button.objectName()!r} has no menu() attached"
        )
    menu.popup(button.mapToGlobal(button.rect().bottomLeft()))
    pump(settle_ms)
    return menu


def click_menu_action(menu, text: str, settle_ms: int = 250) -> None:
    """Click the QAction in ``menu`` whose (accelerator-stripped) text matches.

    Raises if not found, listing available actions -- a renamed/removed menu
    item must break the scenario, not vanish quietly.
    """
    if menu is None:
        raise AssertionError("click_menu_action: menu is None")
    pump(settle_ms)
    target = None
    for action in menu.actions():
        if action.text().replace("&", "") == text:
            target = action
            break
    if target is None:
        available = [a.text() for a in menu.actions()]
        raise AssertionError(
            f"click_menu_action: no action {text!r} found; available: {available}"
        )
    # trigger() is reliable headlessly; mouseClick on menu actionGeometry often
    # misses under Xvfb and leaves graph mutations (setViewNode, etc.) undone.
    target.trigger()
    pump(settle_ms)


def grab_widget_png(widget, path: str, settle_ms: int = 400):
    """Pump, grab ``widget`` to a PNG, and return (ok, width, height).

    Raises if the save fails outright (bad path etc.); a False ``ok`` from
    QPixmap.save is still returned to the caller to log, since a 0x0 grab is
    a real signal something's wrong with the widget, not a harness bug.
    """
    if widget is None:
        raise AssertionError("grab_widget_png: widget is None")
    if not widget.isVisible():
        widget.show()
    pump(settle_ms)
    pixmap = widget.grab()
    ok = pixmap.save(path, "PNG")
    return ok, pixmap.width(), pixmap.height()
