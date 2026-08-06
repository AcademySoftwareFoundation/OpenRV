"""Gate 5 — transform_manip's geometry helpers on the port itself.

These are the module-level vector helpers the manipulator's hit testing and drawing
are built on. They are pure, so they are imported and called directly.
"""
from __future__ import annotations

import math
import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class ManipTest(unittest.TestCase):
    def setUp(self):
        try:
            self.mod, self.graph = _rv_stubs.importPort("transform_manip")
        except ImportError as exc:
            raise unittest.SkipTest("transform_manip needs PyOpenGL: %s" % exc)


class TestVectorOps(ManipTest):
    def test_add(self):
        self.assertEqual(tuple(self.mod._add((1.0, 2.0), (3.0, 4.0))), (4.0, 6.0))

    def test_sub(self):
        self.assertEqual(tuple(self.mod._sub((5.0, 7.0), (2.0, 3.0))), (3.0, 4.0))

    def test_scale(self):
        self.assertEqual(tuple(self.mod._scale((2.0, 3.0), 2.0)), (4.0, 6.0))

    def test_scale_by_zero(self):
        self.assertEqual(tuple(self.mod._scale((2.0, 3.0), 0.0)), (0.0, 0.0))

    def test_add_and_sub_are_inverse(self):
        a, b = (1.5, -2.5), (0.25, 4.0)
        self.assertEqual(tuple(self.mod._sub(self.mod._add(a, b), b)), a)


class TestDot(ManipTest):
    def test_perpendicular_is_zero(self):
        self.assertAlmostEqual(self.mod.dot((1.0, 0.0), (0.0, 1.0)), 0.0)

    def test_parallel(self):
        self.assertAlmostEqual(self.mod.dot((2.0, 0.0), (3.0, 0.0)), 6.0)

    def test_anti_parallel_is_negative(self):
        self.assertLess(self.mod.dot((1.0, 0.0), (-1.0, 0.0)), 0.0)

    def test_commutative(self):
        a, b = (1.0, 2.0), (3.0, 4.0)
        self.assertAlmostEqual(self.mod.dot(a, b), self.mod.dot(b, a))


class TestMag(ManipTest):
    def test_pythagorean(self):
        self.assertAlmostEqual(self.mod.mag((3.0, 4.0)), 5.0)

    def test_zero(self):
        self.assertAlmostEqual(self.mod.mag((0.0, 0.0)), 0.0)

    def test_unit(self):
        self.assertAlmostEqual(self.mod.mag((1.0, 0.0)), 1.0)


class TestNormalize(ManipTest):
    def test_result_is_unit_length(self):
        self.assertAlmostEqual(self.mod.mag(self.mod.normalize((3.0, 4.0))), 1.0)

    def test_direction_is_preserved(self):
        n = self.mod.normalize((3.0, 4.0))
        self.assertAlmostEqual(n[0], 0.6)
        self.assertAlmostEqual(n[1], 0.8)

    def test_diagonal(self):
        n = self.mod.normalize((1.0, 1.0))
        self.assertAlmostEqual(n[0], math.sqrt(2) / 2)

    def test_zero_vector_still_divides_by_zero(self):
        """normalize() has no zero guard, and deliberately still has none.

        An earlier version of this test claimed the raise was harmless Mu parity
        because no call site passes a zero vector. That was wrong: control() returns
        (FreeTranslation, gc, gc) for a non-corner grab, so drag() used to normalize
        exactly (0,0) on every free translation. The real fix was to stop computing
        the diagonal on that path (see TestDragMath), not to guard here — guarding
        would only move the ZeroDivisionError to the `/ downDist` a few lines later,
        because a zero direction makes both projected distances 0 too.
        """
        with self.assertRaises(ZeroDivisionError):
            self.mod.normalize((0.0, 0.0))


class TestComputeGC(ManipTest):
    def test_centroid_of_a_unit_square(self):
        corners = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]
        gc = self.mod.computeGC(corners)
        self.assertAlmostEqual(gc[0], 0.5)
        self.assertAlmostEqual(gc[1], 0.5)

    def test_translated_square_moves_its_centroid(self):
        corners = [(10.0, 10.0), (11.0, 10.0), (11.0, 11.0), (10.0, 11.0)]
        gc = self.mod.computeGC(corners)
        self.assertAlmostEqual(gc[0], 10.5)
        self.assertAlmostEqual(gc[1], 10.5)


class TestClosestPointOnLine(ManipTest):
    def test_midpoint_of_a_horizontal_segment(self):
        p = self.mod.closestPointOnLine((0.5, 3.0), (0.0, 0.0), (1.0, 0.0))
        self.assertAlmostEqual(p[0], 0.5)
        self.assertAlmostEqual(p[1], 0.0)

    def test_point_already_on_the_line(self):
        p = self.mod.closestPointOnLine((0.25, 0.0), (0.0, 0.0), (1.0, 0.0))
        self.assertAlmostEqual(p[0], 0.25)


class TestTagValue(ManipTest):
    def test_finds_a_named_tag(self):
        """Tags arrive as a list of (name, value) pairs, not a mapping."""
        self.assertEqual(self.mod.tagValue([("a", "1"), ("b", "2")], "b"), "2")

    def test_missing_tag_is_none(self):
        self.assertIsNone(self.mod.tagValue([("a", "1")], "zzz"))

    def test_empty_tag_list(self):
        self.assertIsNone(self.mod.tagValue([], "a"))

    def test_first_match_wins(self):
        self.assertEqual(self.mod.tagValue([("a", "1"), ("a", "2")], "a"), "1")


if __name__ == "__main__":
    unittest.main()
