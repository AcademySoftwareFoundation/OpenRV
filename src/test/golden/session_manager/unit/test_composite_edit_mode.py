"""Gate 5 — Composite_edit_mode on the port itself.

setOp() is the blend-mode control; its index-to-name table and the dissolve clamping
are the two places a port can silently disagree with Mu, so both are driven through
the real methods and read back off the graph.
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


class CompositeTest(unittest.TestCase):
    TYPE = "stack.composite.type"
    AMOUNT = "stack.composite.dissolveAmount"

    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("Composite_edit_mode")
        self.mode = self.mod.CompositeEditMode.__new__(self.mod.CompositeEditMode)
        self.mode._ui = None

        self.graph.addNode("stackGroup", "RVStackGroup")
        self.graph.addNode("stack", "RVStack", group="stackGroup")
        self.graph.viewNode = "stackGroup"
        self.graph.seedString(self.TYPE, ["over"])
        self.graph.seedFloat(self.AMOUNT, [0.5])

    def opType(self):
        return self.graph.getStringProperty(self.TYPE)[0]

    def amount(self):
        return self.graph.getFloatProperty(self.AMOUNT)[0]


class TestSetOp(CompositeTest):
    def test_every_index_maps_to_its_name(self):
        expected = ["over", "add", "dissolve", "difference", "-difference",
                    "replace", "topmost"]
        for index, name in enumerate(expected):
            self.mode.setOp(index)
            self.assertEqual(self.opType(), name, "index %d" % index)

    def test_index_past_the_end_falls_back_to_over(self):
        self.mode.setOp(3)
        self.mode.setOp(99)
        self.assertEqual(self.opType(), "over")

    def test_negative_index_falls_back_to_over(self):
        self.mode.setOp(3)
        self.mode.setOp(-1)
        self.assertEqual(self.opType(), "over")

    def test_redraws(self):
        before = self.graph.redraws
        self.mode.setOp(1)
        self.assertEqual(self.graph.redraws, before + 1)

    def test_event_wrapper_delegates(self):
        self.mode.setOpEvent(None, 2)
        self.assertEqual(self.opType(), "dissolve")


class TestDissolveAmount(CompositeTest):
    def setUp(self):
        super().setUp()
        self.mode._dissolveLineEdit = QtWidgets.QLineEdit()
        self.mode._dissolveSlider = QtWidgets.QSlider()
        self.mode._dissolveSlider.setRange(0, 100)

    def test_in_range_value_is_written(self):
        self.mode._dissolveLineEdit.setText("0.25")
        self.mode.setDissolveAmount()
        self.assertAlmostEqual(self.amount(), 0.25)

    def test_value_above_one_clamps(self):
        self.mode._dissolveLineEdit.setText("5")
        self.mode.setDissolveAmount()
        self.assertAlmostEqual(self.amount(), 1.0)

    def test_value_below_zero_clamps(self):
        self.mode._dissolveLineEdit.setText("-3")
        self.mode.setDissolveAmount()
        self.assertAlmostEqual(self.amount(), 0.0)

    def test_slider_follows_the_text(self):
        self.mode._dissolveLineEdit.setText("0.3")
        self.mode.setDissolveAmount()
        self.assertEqual(self.mode._dissolveSlider.value(), 30)

    def test_unparseable_text_resets_to_a_half(self):
        self.mode._dissolveLineEdit.setText("not a number")
        self.mode.setDissolveAmount()
        self.assertAlmostEqual(self.amount(), 0.5)
        self.assertEqual(self.mode._dissolveLineEdit.text(), "0.5")
        self.assertEqual(self.mode._dissolveSlider.value(), 50)

    def test_slider_drives_the_text_and_property(self):
        self.mode.setDissolveAmountFromSlider(75)
        self.assertAlmostEqual(self.amount(), 0.75)
        self.assertEqual(self.mode._dissolveLineEdit.text(), "0.75")

    def test_slider_zero_and_full(self):
        self.mode.setDissolveAmountFromSlider(0)
        self.assertAlmostEqual(self.amount(), 0.0)
        self.mode.setDissolveAmountFromSlider(100)
        self.assertAlmostEqual(self.amount(), 1.0)


class TestUpdateUIWithoutPanel(CompositeTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
