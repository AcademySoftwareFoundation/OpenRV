"""Gate 5 — StackGroup_edit_mode and SwitchGroup_edit_mode on the port itself.

These two modes own no widgets. Their whole job is to switch sibling modes on and
off as the view changes, and — for StackGroup — to keep the wipes minor mode in step
with the view's `ui.wipes` flag. Both reach RV through `rv.runtime.eval`, because the
mode manager and the session State they need have no Python binding.

That makes the Mu snippet each one builds the actual unit under test: if the string
is malformed or the wrong mode name is interpolated, nothing fails loudly in a
golden scenario, the sibling editor just silently never appears. So the tests capture
the snippets and assert on what was asked for.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class GroupModeTest(unittest.TestCase):
    MODULE = None
    CLASS = None

    def setUp(self):
        if self.MODULE is None:
            self.skipTest("base class")
        self.mod, self.graph = _rv_stubs.importPort(self.MODULE)
        self.evals = []
        self.mod.rv.runtime.eval = lambda code, mods=None: (
            self.evals.append((code, mods)) or ""
        )
        self.mode = getattr(self.mod, self.CLASS).__new__(getattr(self.mod, self.CLASS))

    def codes(self):
        return [c for c, _ in self.evals]

    def allCode(self):
        return "\n".join(self.codes())


class TestSwitchGroupEditMode(GroupModeTest):
    MODULE = "SwitchGroup_edit_mode"
    CLASS = "SwitchGroupEditMode"

    def test_activate_ui_on_targets_switch_edit_mode(self):
        self.mode.activateUI(True)
        self.assertEqual(len(self.evals), 1)
        self.assertIn('findModeEntry("Switch_edit_mode")', self.codes()[0])

    def test_activate_ui_on_asks_for_true(self):
        self.mode.activateUI(True)
        self.assertIn("true", self.codes()[0].split("findModeEntry")[1])

    def test_activate_ui_off_asks_for_false(self):
        self.mode.activateUI(False)
        self.assertIn("false", self.codes()[0].split("findModeEntry")[1])

    def test_activate_turns_the_sibling_on(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)
        self.assertIn('findModeEntry("Switch_edit_mode")', self.allCode())
        self.assertIn("true", self.allCode())

    def test_deactivate_turns_the_sibling_off(self):
        self.mode._active = True
        self.mode.deactivate()
        self.assertFalse(self.mode._active)
        self.assertIn("false", self.allCode())

    def test_the_mode_manager_module_is_required(self):
        """The snippet names mode_manager types, so it must be in the module list."""
        self.mode.activateUI(True)
        self.assertIn("mode_manager", self.evals[0][1])


class TestStackGroupEditMode(GroupModeTest):
    MODULE = "StackGroup_edit_mode"
    CLASS = "StackGroupEditMode"

    def test_activate_ui_drives_both_siblings(self):
        self.mode.activateUI(True)
        code = self.allCode()
        self.assertIn('findModeEntry("Composite_edit_mode")', code)
        self.assertIn('findModeEntry("Stack_edit_mode")', code)

    def test_activate_ui_also_syncs_the_wipe_mode(self):
        self.mode.activateUI(True)
        self.assertEqual(len(self.evals), 3,
                         "two sibling activations plus one wipe reconciliation")
        self.assertIn("ui.wipes", self.allCode())

    def test_wipe_sync_reads_the_view_nodes_flag(self):
        self.mode.activateUI(True)
        wipeCode = [c for c in self.codes() if "wipes" in c][0]
        self.assertIn('viewNode() + ".ui.wipes"', wipeCode)
        self.assertIn("getIntProperty(p).front() == 1", wipeCode)

    def test_wipe_sync_off_never_turns_wipes_on(self):
        """Deactivating passes false, so the "wipe on" branch cannot be taken."""
        self.mode.activateUI(False)
        wipeCode = [c for c in self.codes() if "wipes" in c][0]
        self.assertIn("let wipeon = false", wipeCode)

    def test_wipe_sync_on_can_turn_wipes_on(self):
        self.mode.activateUI(True)
        wipeCode = [c for c in self.codes() if "wipes" in c][0]
        self.assertIn("let wipeon = true", wipeCode)

    def test_wipe_off_uses_toggle_not_toggleWipe(self):
        """The two are not interchangeable.

        toggleWipe() resets the wipes and clears ui.wipes; wipe.toggle() only makes
        the mode inactive, so returning to this view restores the same wipes. The
        port's comment says as much — this pins it.
        """
        self.mode.activateUI(True)
        wipeCode = [c for c in self.codes() if "wipes" in c][0]
        self.assertIn("toggleWipe()", wipeCode)
        self.assertIn("wipe.toggle()", wipeCode)

    def test_activate_and_deactivate_track_active_state(self):
        self.mode._active = False
        self.mode.activate()
        self.assertTrue(self.mode._active)
        self.mode.deactivate()
        self.assertFalse(self.mode._active)

    def test_property_change_on_wipes_reactivates_and_redraws(self):
        before = self.graph.redraws
        self.mode.propertyChanged(_Event("#RVStackGroup.ui.wipes"))
        self.assertIn("ui.wipes", self.allCode())
        self.assertEqual(self.graph.redraws, before + 1)

    def test_property_change_on_retime_to_output_reactivates(self):
        before = self.graph.redraws
        self.mode.propertyChanged(_Event("#RVStackGroup.timing.retimeToOutput"))
        self.assertEqual(self.graph.redraws, before + 1)

    def test_unrelated_property_change_is_ignored(self):
        before = self.graph.redraws
        self.mode.propertyChanged(_Event("#RVStackGroup.output.fps"))
        self.assertEqual(self.evals, [])
        self.assertEqual(self.graph.redraws, before)

    def test_property_change_always_rejects_the_event(self):
        """Rejecting lets the other graph-state-change handlers still run."""
        event = _Event("#RVStackGroup.output.fps")
        self.mode.propertyChanged(event)
        self.assertTrue(event.rejected)


class _Event:
    def __init__(self, contents):
        self._contents = contents
        self.rejected = False

    def contents(self):
        return self._contents

    def reject(self):
        self.rejected = True


if __name__ == "__main__":
    unittest.main()
