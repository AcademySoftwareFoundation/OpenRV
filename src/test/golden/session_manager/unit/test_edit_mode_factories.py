"""Gate 5 — `createMode()` and `auxFilePath()` for all eleven sibling modes.

Every sibling module ends in a `createMode()` that RV's package loader calls by name,
and the constructor it runs is the mode's whole registration: the name RV files it
under, the event bindings, the menu, and the sort key that decides where in the event
chain the mode sits. None of that is observable from a golden — a mode that registers
under the wrong name or silently drops an event binding still starts RV cleanly and
still renders an identical first frame. It only shows up later, as an editor tab that
never refreshes or a manipulator that stops receiving pointer events.

So the expectations below are transcribed from the `.mu` originals rather than from
the ports, and `_rv_stubs`' MinorMode retains what `init()` was handed so they can be
read back.

Two deliberate deviations are pinned as such:

* `RetimeGroup_edit_mode` binds one event the Mu version does not. Mu's prompts are
  blocking modal dialogs; the port drives the same prompts through RV's non-blocking
  text entry, which needs a commit event to apply the value.
* `LayoutGroup_edit_mode` and `SourceGroup_edit_mode` call `self.auxFilePath()` where
  Mu calls `manager.auxFilePath()`. `supportPath()` resolves off the calling module's
  own file and every sibling is staged into the same directory, so both spell the same
  path — asserted here rather than assumed.
"""
from __future__ import annotations

import os
import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtWidgets

_app = None


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)
    global _app
    _app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])


#
#  module, class, global bindings, override bindings, menu?, sort key, .ui file.
#  Bindings and sort keys transcribed from the init() call in the matching .mu.
#
MODES = [
    (
        "Composite_edit_mode", "CompositeEditMode",
        [],
        ["session-manager-load-ui", "graph-state-change"],
        True, "b", "composite.ui",
    ),
    (
        "FolderGroup_edit_mode", "FolderGroupEditMode",
        [],
        ["session-manager-load-ui", "graph-state-change"],
        False, None, None,
    ),
    (
        "LayoutGroup_edit_mode", "LayoutGroupEditMode",
        ["session-manager-load-ui", "graph-state-change"],
        [],
        True, "a", "layout.ui",
    ),
    (
        "RetimeGroup_edit_mode", "RetimeGroupEditMode",
        [],
        ["session-manager-load-ui", "graph-state-change"],
        True, None, "retime.ui",
    ),
    (
        "SequenceGroup_edit_mode", "SequenceGroupEditMode",
        [],
        ["session-manager-load-ui", "range-changed", "image-structure-change",
         "before-session-read", "after-session-read", "graph-state-change"],
        True, None, "sequence.ui",
    ),
    (
        "SourceGroup_edit_mode", "SourceGroupEditMode",
        [],
        ["new-in-point", "new-out-point", "session-manager-load-ui",
         "graph-state-change"],
        True, None, "source.ui",
    ),
    (
        "StackGroup_edit_mode", "StackGroupEditMode",
        [],
        ["graph-state-change"],
        True, None, None,
    ),
    (
        "Stack_edit_mode", "StackEditMode",
        [],
        ["session-manager-load-ui", "range-changed", "image-structure-change",
         "graph-state-change"],
        False, "z", "stack.ui",
    ),
    (
        "SwitchGroup_edit_mode", "SwitchGroupEditMode",
        [],
        [],
        False, None, None,
    ),
    (
        "Switch_edit_mode", "SwitchEditMode",
        [],
        ["session-manager-load-ui", "range-changed", "image-structure-change",
         "graph-state-change"],
        True, "z0", "switch.ui",
    ),
    (
        "transform_manip", "TransformManip",
        [],
        ["pointer--move", "pointer-1--push", "pointer-1--drag", "pointer-1--release",
         "graph-node-inputs-changed", "after-graph-view-change",
         "before-graph-view-change", "stylus-pen--move", "stylus-pen--push",
         "stylus-pen--drag", "stylus-pen--release"],
        True, "zza", None,
    ),
]

#
#  Mu's RetimeGroup has no equivalent; see the module docstring.
#
EXTRA_BINDINGS = {"RetimeGroup_edit_mode": ["retime-group-text-entry-commit"]}


def _events(bindings):
    return [b[0] for b in (bindings or [])]


class FactoryTest(unittest.TestCase):
    def build(self, moduleName, className):
        mod, graph = _rv_stubs.importPort(moduleName)
        mode = mod.createMode()
        self.assertIsInstance(mode, getattr(mod, className))
        return mod, graph, mode


class TestCreateMode(FactoryTest):
    def test_each_factory_returns_its_own_mode(self):
        for moduleName, className, _g, _o, _m, _s, _ui in MODES:
            with self.subTest(module=moduleName):
                self.build(moduleName, className)

    def test_each_mode_registers_under_its_module_name(self):
        """RV looks the mode up by this string; a typo makes it unreachable."""
        for moduleName, className, _g, _o, _m, _s, _ui in MODES:
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                self.assertEqual(mode._modeName, moduleName)

    def test_global_bindings_match_the_mu_original(self):
        for moduleName, className, glob, _o, _m, _s, _ui in MODES:
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                self.assertEqual(_events(mode._globalBindings), glob)

    def test_override_bindings_match_the_mu_original(self):
        for moduleName, className, _g, override, _m, _s, _ui in MODES:
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                expected = override + EXTRA_BINDINGS.get(moduleName, [])
                self.assertEqual(_events(mode._overrideBindings), expected)

    def test_every_binding_is_a_callable_with_a_description(self):
        for moduleName, className, _g, _o, _m, _s, _ui in MODES:
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                bindings = list(mode._globalBindings or []) + list(
                    mode._overrideBindings or [])
                for event, func, doc in bindings:
                    self.assertTrue(callable(func), "%s: %s" % (moduleName, event))
                    self.assertIsInstance(doc, str)

    def test_sort_keys_match_the_mu_original(self):
        """Ordering is what keeps the manipulator's screen-covering events last."""
        for moduleName, className, _g, _o, _m, sortKey, _ui in MODES:
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                self.assertEqual(mode._sortKey, sortKey)

    def test_modes_that_ship_a_menu_ship_a_non_empty_one(self):
        for moduleName, className, _g, _o, hasMenu, _s, _ui in MODES:
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                if hasMenu:
                    self.assertTrue(mode._menu)
                else:
                    self.assertFalse(mode._menu)

    def test_stack_registers_no_menu_at_construction(self):
        """Mu passes `nil, //menu()` — the entry is built later by updateMenu(),
        because the top-level label depends on whether the view is a stack or a
        layout. Registering it here would freeze the wrong label."""
        _mod, _graph, mode = self.build("Stack_edit_mode", "StackEditMode")
        self.assertIsNone(mode._menu)

    def test_a_second_mode_is_a_distinct_object(self):
        mod, _graph = _rv_stubs.importPort("Switch_edit_mode")
        self.assertIsNot(mod.createMode(), mod.createMode())


class TestAuxFilePath(FactoryTest):
    def test_resolves_to_a_file_that_exists_in_the_package(self):
        for moduleName, className, _g, _o, _m, _s, uiFile in MODES:
            if uiFile is None:
                continue
            with self.subTest(module=moduleName):
                _mod, _graph, mode = self.build(moduleName, className)
                path = mode.auxFilePath(uiFile)
                self.assertTrue(os.path.isfile(path), path)

    def test_the_modes_without_their_own_go_through_the_session_manager(self):
        """FolderGroup defines no auxFilePath of its own — Mu does not either. It
        reaches folder.ui through the manager, so that path is what has to resolve."""
        mod, _graph = _rv_stubs.importPort("FolderGroup_edit_mode")
        self.assertFalse(hasattr(mod.createMode(), "auxFilePath"))

        import session_manager
        manager = session_manager.SessionManagerMode.__new__(
            session_manager.SessionManagerMode)
        self.assertTrue(os.path.isfile(manager.auxFilePath("folder.ui")))

    def test_every_mode_resolves_to_the_same_support_directory(self):
        """Six siblings ask the session manager for the path and two ask themselves;
        both spellings have to land in one directory or half the editors load no UI."""
        dirs = set()
        for moduleName, className, _g, _o, _m, _s, _ui in MODES:
            _mod, _graph, mode = self.build(moduleName, className)
            if hasattr(mode, "auxFilePath"):
                dirs.add(os.path.dirname(mode.auxFilePath("x.ui")))
        self.assertEqual(len(dirs), 1, dirs)

    def test_the_name_is_appended_not_substituted(self):
        _mod, _graph, mode = self.build("Stack_edit_mode", "StackEditMode")
        self.assertTrue(mode.auxFilePath("stack.ui").endswith(os.sep + "stack.ui"))

    def test_a_name_that_does_not_exist_still_returns_a_path(self):
        """auxFilePath is a join, not a lookup; loadUIFile is what fails on a typo."""
        _mod, _graph, mode = self.build("Stack_edit_mode", "StackEditMode")
        path = mode.auxFilePath("no_such_file.ui")
        self.assertFalse(os.path.exists(path))
        self.assertTrue(path.endswith("no_such_file.ui"))


if __name__ == "__main__":
    unittest.main()
