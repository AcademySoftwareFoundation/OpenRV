"""Gate 5 — RetimeGroup_edit_mode on the port itself.

The interesting case is reverse(): Mu writes the audio offset with the int overload
against what is normally a float property, and RV's setIntProperty throws
badPropertyType in that situation. That throw is Mu behavior, so the port has to
reproduce it rather than quietly normalise the types — the tests below pin which
writes land before it, and that an int-typed property makes the same call complete.

Methods are called on an instance built with object.__new__: reset(), reverse() and
setFactorValue() touch only properties and commands, never the .ui tree, so building
the editor panel would add a dependency on a live RV without testing anything more.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class RetimeTest(unittest.TestCase):
    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("RetimeGroup_edit_mode")
        self.mode = self.mod.RetimeGroupEditMode.__new__(self.mod.RetimeGroupEditMode)
        self.mode._ui = None

        self.graph.addNode("retimeGroup", "RVRetimeGroup")
        self.graph.addNode("retime", "RVRetime", group="retimeGroup")
        self.graph.viewNode = "retimeGroup"

    def floats(self, name):
        return self.graph.getFloatProperty(name)

    def ints(self, name):
        return self.graph.getIntProperty(name)

    def seedFloats(self, visualScale=1.0):
        for name, value in (
            ("retime.visual.scale", visualScale),
            ("retime.visual.offset", 0.0),
            ("retime.audio.scale", 1.0),
            ("retime.audio.offset", 0.0),
        ):
            self.graph.seedFloat(name, [value])


class TestReset(RetimeTest):
    def test_restores_identity_timing(self):
        self.seedFloats(visualScale=-1.0)
        self.graph.seedFloat("retime.visual.offset", [-42.0])

        self.mode.reset()

        self.assertEqual(self.floats("retime.visual.scale"), [1.0])
        self.assertEqual(self.floats("retime.visual.offset"), [0.0])
        self.assertEqual(self.floats("retime.audio.scale"), [1.0])
        self.assertEqual(self.floats("retime.audio.offset"), [0.0])

    def test_writes_all_four_as_floats(self):
        self.mode.reset()
        for name in ("visual.scale", "visual.offset", "audio.scale", "audio.offset"):
            self.assertEqual(self.graph.props["retime." + name][0], self.graph.FLOAT)

    def test_redraws(self):
        before = self.graph.redraws
        self.mode.reset()
        self.assertEqual(self.graph.redraws, before + 1)


class TestReverse(RetimeTest):
    def test_forward_to_reverse_sets_negative_scale_then_throws(self):
        """The int write against a float audio.offset aborts the rest of reverse()."""
        self.seedFloats(visualScale=1.0)

        with self.assertRaises(Exception):
            self.mode.reverse()

        self.assertEqual(self.floats("retime.visual.scale"), [-1.0])
        self.assertEqual(self.floats("retime.visual.offset"), [float(-(100 - 1))])
        self.assertEqual(self.floats("retime.audio.scale"), [1.0])

    def test_reverse_to_forward_throws_on_the_first_int_write(self):
        self.seedFloats(visualScale=-1.0)

        with self.assertRaises(Exception):
            self.mode.reverse()

        # visual.scale is restored before the int write aborts the rest.
        self.assertEqual(self.floats("retime.visual.scale"), [1.0])

    def test_offset_uses_the_frame_range_length(self):
        self.seedFloats(visualScale=1.0)
        self.graph.props["retime.audio.offset"] = (self.graph.INT, [0])

        self.mode.reverse()

        self.assertEqual(self.floats("retime.visual.offset"), [float(-(100 - 1))])

    def test_int_typed_offset_makes_reverse_complete(self):
        """Same code path, no exception — the throw comes from the property's type.

        This distinguishes "the port picked the wrong overload" from "Mu's overload
        choice collides with an existing float property", which is the real story.
        """
        self.seedFloats(visualScale=1.0)
        self.graph.props["retime.audio.offset"] = (self.graph.INT, [0])

        self.mode.reverse()

        self.assertEqual(self.floats("retime.visual.scale"), [-1.0])
        self.assertEqual(self.ints("retime.audio.offset"), [0])

    def test_visual_offset_is_a_float_write_on_the_forward_branch(self):
        self.seedFloats(visualScale=1.0)
        self.graph.props["retime.audio.offset"] = (self.graph.INT, [0])

        self.mode.reverse()

        self.assertEqual(self.graph.props["retime.visual.offset"][0], self.graph.FLOAT)

    def test_visual_offset_is_an_int_write_on_the_reverse_branch(self):
        """Mu's set(..., 0) on this branch is the int overload, unlike the other."""
        self.seedFloats(visualScale=-1.0)
        self.graph.props["retime.visual.offset"] = (self.graph.INT, [0])
        self.graph.props["retime.audio.offset"] = (self.graph.INT, [0])

        self.mode.reverse()

        self.assertEqual(self.ints("retime.visual.offset"), [0])


class TestSetFactorValue(RetimeTest):
    def test_plain_factor(self):
        self.seedFloats()
        self.mode.setFactorValue("2", False)
        self.assertEqual(self.floats("retime.visual.scale"), [2.0])

    def test_inverted_factor(self):
        self.seedFloats()
        self.mode.setFactorValue("4", True)
        self.assertEqual(self.floats("retime.visual.scale"), [0.25])

    def test_fractional_input(self):
        self.seedFloats()
        self.mode.setFactorValue("0.5", False)
        self.assertEqual(self.floats("retime.visual.scale"), [0.5])


class TestSetConvertFPS(RetimeTest):
    def test_writes_output_fps(self):
        self.graph.seedFloat("retime.output.fps", [24.0])
        self.mode.setConvertFPS("48")
        self.assertEqual(self.floats("retime.output.fps"), [48.0])

    def test_accepts_non_integer_rates(self):
        self.graph.seedFloat("retime.output.fps", [24.0])
        self.mode.setConvertFPS("23.98")
        self.assertEqual(self.floats("retime.output.fps"), [23.98])


class TestUpdateUIWithoutPanel(RetimeTest):
    def test_is_a_noop_when_the_editor_is_not_loaded(self):
        self.mode.updateUI()   # must not raise


if __name__ == "__main__":
    unittest.main()
