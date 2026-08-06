"""Gate 5 — LayoutGroup_edit_mode on the port itself.

The layout mode string drives both the graph and whether the transform manipulator is
active, and those two must stay in step: only "manual" turns the manipulator on.
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


class LayoutTest(unittest.TestCase):
    MODE = "layoutGroup.layout.mode"

    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("LayoutGroup_edit_mode")
        self.mode = self.mod.LayoutGroupEditMode.__new__(self.mod.LayoutGroupEditMode)
        self.mode._ui = None

        self.graph.addNode("layoutGroup", "RVLayoutGroup")
        self.graph.viewNode = "layoutGroup"
        self.graph.seedString(self.MODE, ["packed"])

        # activateTransformMode reaches the mode manager, which needs a live RV.
        self.manip = []
        self.mode.activateTransformMode = lambda on: self.manip.append(on)

    def layoutMode(self):
        return self.graph.getStringProperty(self.MODE)[0]


class TestLayoutMode(LayoutTest):
    def test_reads_the_property(self):
        self.assertEqual(self.mode.layoutMode(), "packed")

    def test_missing_property_reads_empty(self):
        self.graph.deleteProperty(self.MODE)
        self.assertEqual(self.mode.layoutMode(), "")

    def test_set_layout_mode_writes(self):
        self.mode.setLayoutMode("grid")
        self.assertEqual(self.layoutMode(), "grid")


class TestLayoutSelectors(LayoutTest):
    CASES = (
        ("layoutInRow", "row", False),
        ("layoutInColumn", "column", False),
        ("layoutPacked", "packed", False),
        ("layoutInGrid", "grid", False),
        ("layoutPacked2", "packed2", False),
        ("layoutManually", "manual", True),
        ("layoutStatic", "static", False),
    )

    def test_each_selector_writes_its_mode(self):
        for method, expected, _ in self.CASES:
            getattr(self.mode, method)()
            self.assertEqual(self.layoutMode(), expected, method)

    def test_only_manual_enables_the_manipulator(self):
        for method, _, manipOn in self.CASES:
            self.manip.clear()
            getattr(self.mode, method)()
            self.assertEqual(self.manip, [manipOn], method)


class TestIsLayoutMode(LayoutTest):
    def test_checked_for_the_active_mode(self):
        self.assertEqual(self.mode.isLayoutMode("packed")(),
                         self.mod.commands.CheckedMenuState)

    def test_unchecked_otherwise(self):
        self.assertEqual(self.mode.isLayoutMode("grid")(),
                         self.mod.commands.UncheckedMenuState)

    def test_re_evaluated_per_call(self):
        state = self.mode.isLayoutMode("grid")
        self.assertEqual(state(), self.mod.commands.UncheckedMenuState)
        self.mode.setLayoutMode("grid")
        self.assertEqual(state(), self.mod.commands.CheckedMenuState)


class TestSpacingAndGrid(LayoutTest):
    """These setters write without cprop(), matching LayoutGroup_edit_mode.mu:60.

    A real RVLayoutGroup always has these properties, so neither Mu nor the port
    creates them; both would raise badProperty against a node that lacks them.
    """

    def setUp(self):
        super().setUp()
        self.graph.seedFloat("layoutGroup.layout.spacing", [0.0])
        self.graph.seedInt("layoutGroup.layout.gridRows", [0])
        self.graph.seedInt("layoutGroup.layout.gridColumns", [0])

    def test_spacing_is_written_as_a_float(self):
        self.mode.setSpacing(0.25)
        self.assertEqual(
            self.graph.getFloatProperty("layoutGroup.layout.spacing"), [0.25]
        )

    def test_grid_rows_and_columns(self):
        self.mode.setGridRowsColumns(3, 4)
        self.assertEqual(self.graph.getIntProperty("layoutGroup.layout.gridRows"), [3])
        self.assertEqual(
            self.graph.getIntProperty("layoutGroup.layout.gridColumns"), [4]
        )

    def test_setting_the_grid_also_selects_grid_mode(self):
        self.mode.setGridRowsColumns(2, 2)
        self.assertEqual(self.layoutMode(), "grid")


class TestUpdateUIWithoutPanel(LayoutTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
