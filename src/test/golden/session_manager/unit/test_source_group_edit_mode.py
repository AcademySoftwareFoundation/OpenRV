"""Gate 5 — SourceGroup_edit_mode on the port itself.

Cut in/out is stored as int properties whose "unset" value is Mu's int.max sentinel,
so the tests check both the sentinel round-trip and the prompt text that reads it —
the prompt is the only place a user sees whether a cut is set.
"""
from __future__ import annotations

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


class SourceGroupTest(unittest.TestCase):
    IN = "source.cut.in"
    OUT = "source.cut.out"
    SYNC = "source.cut.syncGui"

    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("SourceGroup_edit_mode")
        self.mode = self.mod.SourceGroupEditMode.__new__(self.mod.SourceGroupEditMode)
        self.mode._ui = None
        self.mode._locked = False

        self.graph.addNode("sourceGroup", "RVSourceGroup")
        self.graph.addNode("source", "RVFileSource", group="sourceGroup")
        self.graph.viewNode = "sourceGroup"
        self.graph.seedInt(self.IN, [-self.mod.MU_INT_MAX])
        self.graph.seedInt(self.OUT, [self.mod.MU_INT_MAX])

    def ints(self, name):
        return self.graph.getIntProperty(name)


class TestMuIntMax(SourceGroupTest):
    def test_sentinel_matches_mu(self):
        self.assertEqual(self.mod.MU_INT_MAX, 2 ** 31 - 1)


class TestSetCutValue(SourceGroupTest):
    def test_sets_in_point(self):
        self.mode.setCutValue("in", "12")
        self.assertEqual(self.ints(self.IN), [12])

    def test_sets_out_point(self):
        self.mode.setCutValue("out", "34")
        self.assertEqual(self.ints(self.OUT), [34])

    def test_negative_value(self):
        self.mode.setCutValue("in", "-5")
        self.assertEqual(self.ints(self.IN), [-5])

    def test_redraws(self):
        before = self.graph.redraws
        self.mode.setCutValue("in", "1")
        self.assertEqual(self.graph.redraws, before + 1)


class TestReset(SourceGroupTest):
    def test_restores_both_sentinels(self):
        self.graph.seedInt(self.IN, [10])
        self.graph.seedInt(self.OUT, [20])

        self.mode.reset()

        self.assertEqual(self.ints(self.IN), [-self.mod.MU_INT_MAX])
        self.assertEqual(self.ints(self.OUT), [self.mod.MU_INT_MAX])

    def test_clears_the_lock_it_takes(self):
        self.mode.reset()
        self.assertFalse(self.mode._locked)

    def test_redraws(self):
        before = self.graph.redraws
        self.mode.reset()
        self.assertEqual(self.graph.redraws, before + 1)


class TestPrompts(SourceGroupTest):
    def test_in_prompt_without_a_cut(self):
        self.assertEqual(self.mode.cutInPrompt(), "Set Source In Point:")

    def test_in_prompt_with_a_cut(self):
        self.graph.seedInt(self.IN, [7])
        self.assertEqual(self.mode.cutInPrompt(), "Set Source In Point (current=7):")

    def test_out_prompt_without_a_cut(self):
        self.assertEqual(self.mode.cutOutPrompt(), "Set Source Out Point:")

    def test_out_prompt_with_a_cut(self):
        self.graph.seedInt(self.OUT, [99])
        self.assertEqual(self.mode.cutOutPrompt(), "Set Source Out Point (current=99):")


class TestSyncSlot(SourceGroupTest):
    def test_enabling_writes_one(self):
        self.graph.seedInt(self.SYNC, [0])
        self.mode.syncSlot(True)
        self.assertEqual(self.ints(self.SYNC), [1])

    def test_disabling_writes_zero(self):
        self.graph.seedInt(self.SYNC, [1])
        self.mode.syncSlot(False)
        self.assertEqual(self.ints(self.SYNC), [0])

    def test_locked_mode_ignores_the_toggle(self):
        self.graph.seedInt(self.SYNC, [0])
        self.mode._locked = True
        self.mode.syncSlot(True)
        self.assertEqual(self.ints(self.SYNC), [0])


class TestNewInOutPoint(SourceGroupTest):
    class _Event:
        def __init__(self):
            self.rejected = False

        def reject(self):
            self.rejected = True

    def test_in_point_follows_the_session_when_syncing(self):
        self.graph.seedInt(self.SYNC, [1])
        event = self._Event()
        self.mode.newInPoint(event)
        self.assertEqual(self.ints(self.IN), [1])       # FakeGraph inPoint() == 1
        self.assertTrue(event.rejected)

    def test_out_point_follows_the_session_when_syncing(self):
        self.graph.seedInt(self.SYNC, [1])
        self.mode.newOutPoint(self._Event())
        self.assertEqual(self.ints(self.OUT), [100])    # FakeGraph outPoint() == 100

    def test_no_write_when_sync_is_off(self):
        self.graph.seedInt(self.SYNC, [0])
        self.mode.newInPoint(self._Event())
        self.assertEqual(self.ints(self.IN), [-self.mod.MU_INT_MAX])

    def test_no_write_while_locked(self):
        self.graph.seedInt(self.SYNC, [1])
        self.mode._locked = True
        self.mode.newInPoint(self._Event())
        self.assertEqual(self.ints(self.IN), [-self.mod.MU_INT_MAX])

    def test_event_is_always_rejected(self):
        """Rejecting lets the rest of RV keep handling the point change."""
        self.graph.seedInt(self.SYNC, [0])
        event = self._Event()
        self.mode.newOutPoint(event)
        self.assertTrue(event.rejected)


class TestUpdateUIWithoutPanel(SourceGroupTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
