"""Gate 5 — the manipulator's `render()` and the corner glyphs it draws.

`render` is bound to RV's render event and issues immediate-mode OpenGL directly. It
cannot be pinned by a golden: the manipulator only appears for a manually-laid-out
layout with a tagged active image, and the harness cannot produce the pointer state
that tags one. It also cannot be run against a real GL context here.

What it can be run against is a recording context. Every `gl*`/`glu*` name the port
pulled in with `from OpenGL.GL import *` lives in the module's own namespace, so
swapping them for recorders turns `render()` into a call log — and the log is the
interesting part, because the geometry decisions (which corners, which line widths,
when a corner nub collapses) all live in the nested `drawCorners`.

The whole body of `render` sits inside `except Exception: pass`, so a test that only
called it and checked for no exception would pass against an empty method. Every test
below asserts on what was drawn.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class _Recorder:
    """Stands in for one gl* entry point and logs its arguments."""

    def __init__(self, log, name):
        self._log = log
        self._name = name

    def __call__(self, *args):
        self._log.append((self._name, args))


class _RenderEvent:
    def __init__(self, width=800, height=600, vflip=False):
        self._domain = (width, height)
        self._vflip = vflip

    def domain(self):
        return self._domain

    def domainVerticalFlip(self):
        return self._vflip


SQUARE = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]


class RenderTest(unittest.TestCase):
    def setUp(self):
        self.mod, self.graph = _rv_stubs.importPort("transform_manip")

        self.calls = []
        for name in list(vars(self.mod)):
            if name.startswith(("gl", "glu")) and callable(getattr(self.mod, name)):
                setattr(self.mod, name, _Recorder(self.calls, name))

        self.mode = self.mod.TransformManip.__new__(self.mod.TransformManip)
        self.mode._currentEditNode = "tform"
        self.mode._gc = None
        self.mode._editNodes = []

        self.graph.addNode("layout", "RVLayoutGroup")
        self.graph.viewNode = "layout"

        self.setActiveImage(0)
        self.mod.commands.imageGeometryByIndex = lambda index: SQUARE

    def setActiveImage(self, index):
        """activeImageIndex() reads the tmanip_state tag off the rendered images."""
        if index is None:
            self.mod.commands.renderedImages = lambda: []
            return
        self.mod.commands.renderedImages = lambda: [
            {"index": index, "tags": [("tmanip_state", "active")]}
        ]

    def names(self):
        return [n for n, _a in self.calls]

    def named(self, name):
        return [a for n, a in self.calls if n == name]

    def render(self, event=None):
        self.mode.render(event or _RenderEvent())


class TestRenderGuards(RenderTest):
    def test_nothing_is_drawn_without_an_edit_node(self):
        self.mode._currentEditNode = None
        self.render()
        self.assertEqual(self.calls, [])

    def test_nothing_is_drawn_without_an_active_image(self):
        """The projection is set up first, so "nothing" means nothing after it."""
        self.setActiveImage(None)
        self.render()
        self.assertNotIn("glBegin", self.names())

    def test_an_active_image_draws(self):
        self.render()
        self.assertIn("glBegin", self.names())


class TestRenderProjection(RenderTest):
    def test_the_projection_matches_the_event_domain(self):
        self.render(_RenderEvent(width=640, height=480))
        self.assertEqual(self.named("gluOrtho2D"),
                         [(0.0, 639, 0.0, 479)])

    def test_a_vertically_flipped_domain_inverts_the_y_range(self):
        """RV renders some domains upside down; the manip has to follow."""
        self.render(_RenderEvent(width=640, height=480, vflip=True))
        self.assertEqual(self.named("gluOrtho2D"),
                         [(0.0, 639, 479, 0.0)])

    def test_both_matrices_are_reset(self):
        self.render()
        self.assertGreaterEqual(self.names().count("glLoadIdentity"), 2)


class TestRenderOutline(RenderTest):
    def test_the_image_outline_traces_all_four_corners(self):
        self.render()
        loopStart = self.names().index("glBegin")
        loopEnd = self.names().index("glEnd", loopStart)
        verts = [a for n, a in self.calls[loopStart:loopEnd] if n == "glVertex2f"]
        self.assertEqual([tuple(v) for v in verts],
                         [(c[0], c[1]) for c in SQUARE])

    def test_blending_is_enabled_and_disabled_again(self):
        """A leaked GL_BLEND changes how everything drawn after this looks."""
        self.assertNotIn(("glDisable", ("GL_BLEND",)), self.calls)
        self.render()
        enables = [a[0] for a in self.named("glEnable")]
        disables = [a[0] for a in self.named("glDisable")]
        self.assertIn(self.mod.GL_BLEND, enables)
        self.assertIn(self.mod.GL_BLEND, disables)

    def test_the_centre_glyph_is_placed_at_the_geometric_centre(self):
        self.render()
        self.assertEqual(self.named("glTranslatef")[0], (50.0, 50.0, 0.0))

    def test_the_geometric_centre_is_cached_for_the_pointer_code(self):
        """push/drag read _gc to decide whether the pointer grabbed the centre."""
        self.render()
        self.assertEqual(self.mode._gc, (50.0, 50.0))

    def test_the_centre_glyph_is_drawn_twice_for_a_dark_outline(self):
        self.render()
        translates = self.named("glTranslatef")
        self.assertEqual(translates[:2], [(50.0, 50.0, 0.0), (50.0, 50.0, 0.0)])


class TestRenderCorners(RenderTest):
    def cornerPasses(self):
        """The two drawCorners passes, as lists of (start, end) line segments."""
        passes = []
        current = None
        for name, args in self.calls:
            if name == "glLineWidth" and args[0] in (8.0, 6.0):
                current = []
                passes.append(current)
            elif name == "glLineWidth" and current is not None:
                current = None
            elif name == "glVertex2f" and current is not None:
                current.append(args)
        return passes

    def test_both_passes_draw_every_corner(self):
        """Four corners, two segments each, two vertices per segment."""
        self.render()
        passes = self.cornerPasses()
        self.assertEqual(len(passes), 2)
        for p in passes:
            self.assertEqual(len(p), 16)

    def test_the_dark_pass_is_wider_than_the_light_one(self):
        """The 8pt black pass under the 6pt white one is what makes the nub read
        against a bright image."""
        self.render()
        widths = [a[0] for a in self.named("glLineWidth")]
        # widths[0] is the 2pt outline that precedes both corner passes.
        self.assertEqual(widths[1:3], [8.0, 6.0])

    def test_each_corner_draws_towards_both_of_its_neighbours(self):
        self.render()
        first = self.cornerPasses()[0]
        # corner (0,0): neighbours are (0,100) and (100,0), so the two segments
        # leave along +y and +x.
        self.assertEqual(first[0], (0.0, 25.0))
        self.assertEqual(first[2], (25.0, 0.0))

    def test_a_side_shorter_than_the_nub_collapses_it(self):
        """Without this the nubs from adjacent corners would overlap and the
        outline would read as a solid bar."""
        thin = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)]
        self.mod.commands.imageGeometryByIndex = lambda index: thin
        self.render()
        first = self.cornerPasses()[0]
        self.assertEqual(first[0], (0.0, 0.0),
                         "the nub must collapse onto the corner, not stick out")

    def test_a_degenerate_geometry_is_swallowed(self):
        """imageGeometryByIndex can return coincident corners mid-resize; the
        divide by zero that follows is caught, as it is in Mu."""
        self.mod.commands.imageGeometryByIndex = lambda index: [(0.0, 0.0)] * 4
        self.render()
        self.assertIn("glBegin", self.names())


class TestActivate(RenderTest):
    def test_activate_marks_the_mode_active(self):
        self.mode._active = False
        self.mode.findEditingNodes = lambda setStates=True: None
        self.mode.activate()
        self.assertTrue(self.mode._active)

    def test_activate_rescans_for_editable_nodes(self):
        """Without this the manip comes up bound to the previous view's nodes."""
        calls = []
        self.mode.findEditingNodes = lambda setStates=True: calls.append(setStates)
        self.mode.activate()
        self.assertEqual(len(calls), 1)


if __name__ == "__main__":
    unittest.main()
