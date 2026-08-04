"""Gate 5 — TransformManip's pointer handlers on the port itself.

These four handlers (move / push / drag / release) plus deactivate() are the whole
interactive surface of the manipulator, and nothing else covered them: the golden
scenarios drive the command API rather than the pointer, and the manipulator draws in
GL, so neither the behavioral nor the pixel gate can see it.

That gap hid two real defects until an independent review found them, both of which
these tests now pin:

* every handler called ``int(Qt.CursorShape.X)``, which raises TypeError under
  PySide6 6.5 because Qt.CursorShape is a plain enum.Enum — so move() aborted before
  it could ever find an edit node, and the manipulator never worked at all;
* drag() computed the corner diagonal unconditionally, and control() returns the
  centroid as the grab point for a non-corner grab, so a free-translation drag
  normalised (0,0) and raised ZeroDivisionError on every event.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6.QtCore import Qt


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class _Event:
    def __init__(self, pointer=(50.0, 25.0)):
        self._pointer = pointer
        self.rejected = False

    def pointer(self):
        return self._pointer

    def reject(self):
        self.rejected = True


class PointerTest(unittest.TestCase):
    """A single 100x50 tile whose RVTransform2D is tagged for the manipulator."""

    CORNERS = [(0.0, 0.0), (100.0, 0.0), (100.0, 50.0), (0.0, 50.0)]

    def setUp(self):
        try:
            self.mod, self.graph = _rv_stubs.importPort("transform_manip")
        except ImportError as exc:
            raise unittest.SkipTest("transform_manip needs PyOpenGL: %s" % exc)

        self.mode = self.mod.TransformManip.__new__(self.mod.TransformManip)
        self.mode._currentEditNode = None
        self.mode._control = self.mod.NoControl
        self.mode._editing = False
        self.mode._didDrag = False
        self.mode._downPoint = (0.0, 0.0)
        self.mode._gc = (0.0, 0.0)
        self.mode._corner = (0.0, 0.0)

        self.graph.addNode("layoutGroup", "RVLayoutGroup")
        self.graph.addNode("t_a", "RVTransform2D")
        self.graph.viewNode = "layoutGroup"
        self.graph.seedFloat("t_a.transform.translate", [0.0, 0.0])
        self.graph.seedFloat("t_a.transform.scale", [1.0, 1.0])
        self.mode._editNodes = [self.mod.EditNodePair("t_a", "src_a")]

        self.cursors = []
        self.mod.commands.setCursor = lambda c: self.cursors.append(c)
        self.mod.commands.imageGeometryByIndex = lambda i: self.CORNERS
        self.mod.commands.imagesAtPixel = lambda p, **k: [
            {"inside": True, "index": 0, "tags": [("tmanip", "t_a")]}
        ]
        self.mod.commands.renderedImages = lambda: [
            {"index": 0, "tags": [("tmanip_state", "hover")]}
        ]

    def translate(self):
        return self.graph.getFloatProperty("t_a.transform.translate")

    def scale(self):
        return self.graph.getFloatProperty("t_a.transform.scale")


class TestCursorShapesAreUsable(PointerTest):
    """Every cursor the handlers set must survive being passed to setCursor."""

    SHAPES = ("ArrowCursor", "SizeBDiagCursor", "SizeFDiagCursor",
              "OpenHandCursor", "ClosedHandCursor", "WhatsThisCursor")

    def test_int_on_a_cursor_shape_still_raises(self):
        """Guards the reason .value is used, so nobody reintroduces int()."""
        with self.assertRaises(TypeError):
            int(Qt.CursorShape.ArrowCursor)

    def test_every_shape_the_port_uses_has_an_int_value(self):
        for name in self.SHAPES:
            self.assertIsInstance(getattr(Qt.CursorShape, name).value, int, name)


class TestMove(PointerTest):
    def test_finds_the_tagged_edit_node(self):
        self.mode.move(_Event((50.0, 25.0)))
        self.assertIsNotNone(self.mode._currentEditNode,
                             "move() must reach the imagesAtPixel loop")
        self.assertEqual(self.mode._currentEditNode.tformNode, "t_a")

    def test_sets_the_hover_manip_state(self):
        self.mode.move(_Event((50.0, 25.0)))
        self.assertEqual(
            self.graph.getStringProperty("t_a.tag.tmanip_state"), ["hover"])

    def test_centre_grab_reports_a_free_translation_and_open_hand(self):
        self.mode.move(_Event((50.0, 25.0)))
        self.assertEqual(self.mode._control, self.mod.FreeTranslation)
        self.assertEqual(self.cursors[-1], Qt.CursorShape.OpenHandCursor.value)

    def test_corner_grab_reports_a_corner_and_a_resize_cursor(self):
        self.mode.move(_Event((2.0, 2.0)))
        self.assertEqual(self.mode._control, self.mod.BotLeftCorner)
        self.assertEqual(self.cursors[-1], Qt.CursorShape.SizeBDiagCursor.value)

    def test_no_tile_under_the_pointer_clears_the_edit_node(self):
        self.mod.commands.imagesAtPixel = lambda p, **k: []
        self.mode._currentEditNode = self.mode._editNodes[0]
        self.mode.move(_Event((5.0, 5.0)))
        self.assertIsNone(self.mode._currentEditNode)
        self.assertEqual(self.cursors[-1], Qt.CursorShape.ArrowCursor.value)

    def test_leaving_a_tile_clears_its_manip_state(self):
        self.mode.move(_Event((50.0, 25.0)))
        self.mod.commands.imagesAtPixel = lambda p, **k: []
        self.mode.move(_Event((500.0, 500.0)))
        self.assertEqual(self.graph.getStringProperty("t_a.tag.tmanip_state"), [""])

    def test_an_untagged_tile_is_not_grabbed(self):
        self.mod.commands.imagesAtPixel = lambda p, **k: [
            {"inside": True, "index": 0, "tags": [("other", "x")]}
        ]
        self.mode.move(_Event((50.0, 25.0)))
        self.assertIsNone(self.mode._currentEditNode)

    def test_the_event_is_rejected(self):
        event = _Event((50.0, 25.0))
        self.mode.move(event)
        self.assertTrue(event.rejected)


class TestPush(PointerTest):
    def setUp(self):
        super().setUp()
        self.mode.move(_Event((50.0, 25.0)))

    def test_begins_editing(self):
        self.mode.push(_Event((50.0, 25.0)))
        self.assertTrue(self.mode._editing)
        self.assertFalse(self.mode._didDrag)

    def test_records_the_down_point(self):
        self.mode.push(_Event((60.0, 30.0)))
        self.assertEqual(tuple(self.mode._downPoint), (60.0, 30.0))

    def test_sets_the_editing_manip_state_and_closed_hand(self):
        self.mode.push(_Event((50.0, 25.0)))
        self.assertEqual(
            self.graph.getStringProperty("t_a.tag.tmanip_state"), ["editing"])
        self.assertIn(Qt.CursorShape.ClosedHandCursor.value, self.cursors)

    def test_nothing_grabbed_is_a_noop(self):
        self.mode._currentEditNode = None
        self.mode.push(_Event())
        self.assertFalse(self.mode._editing)

    def test_no_active_image_does_not_begin_editing(self):
        self.mod.commands.renderedImages = lambda: []
        self.mode.push(_Event())
        self.assertFalse(self.mode._editing)


class TestDragFreeTranslation(PointerTest):
    """The path that used to raise ZeroDivisionError on every event."""

    def setUp(self):
        super().setUp()
        self.mode.move(_Event((50.0, 25.0)))       # centre -> FreeTranslation
        self.mode.push(_Event((50.0, 25.0)))

    def test_corner_equals_centroid_on_a_free_grab(self):
        """The precondition that made the diagonal degenerate."""
        self.assertEqual(self.mode._control, self.mod.FreeTranslation)
        self.assertEqual(tuple(self.mode._corner), tuple(self.mode._gc))

    def test_drag_translates_instead_of_raising(self):
        self.mode.drag(_Event((60.0, 25.0)))
        self.assertNotEqual(self.translate(), [0.0, 0.0],
                            "a free drag must move the tile")

    def test_horizontal_drag_moves_only_x(self):
        self.mode.drag(_Event((60.0, 25.0)))
        x, y = self.translate()
        self.assertGreater(x, 0.0)
        self.assertAlmostEqual(y, 0.0)

    def test_vertical_drag_moves_only_y(self):
        self.mode.drag(_Event((50.0, 35.0)))
        x, y = self.translate()
        self.assertAlmostEqual(x, 0.0)
        self.assertGreater(y, 0.0)

    def test_scale_is_untouched_by_a_free_drag(self):
        self.mode.drag(_Event((60.0, 30.0)))
        self.assertEqual(self.scale(), [1.0, 1.0])

    def test_the_down_point_advances_so_drags_accumulate(self):
        self.mode.drag(_Event((60.0, 25.0)))
        self.assertEqual(tuple(self.mode._downPoint), (60.0, 25.0))
        first = list(self.translate())
        self.mode.drag(_Event((70.0, 25.0)))
        self.assertGreater(self.translate()[0], first[0])

    def test_did_drag_is_recorded(self):
        self.mode.drag(_Event((60.0, 25.0)))
        self.assertTrue(self.mode._didDrag)


class TestDragCornerScale(PointerTest):
    def setUp(self):
        super().setUp()
        self.mode.move(_Event((2.0, 2.0)))        # bottom-left corner
        self.mode.push(_Event((2.0, 2.0)))

    def test_corner_grab_uses_the_diagonal(self):
        self.assertNotEqual(tuple(self.mode._corner), tuple(self.mode._gc))

    def test_dragging_a_corner_changes_the_scale(self):
        self.mode.drag(_Event((10.0, 6.0)))
        self.assertNotEqual(self.scale(), [1.0, 1.0])

    def test_scale_never_goes_below_the_floor(self):
        """Mu clamps with max(scale * scl, 0.01)."""
        self.mode.drag(_Event((49.0, 24.0)))
        self.assertGreaterEqual(self.scale()[0], 0.01)

    def test_dragging_a_corner_also_translates(self):
        self.mode.drag(_Event((10.0, 6.0)))
        self.assertNotEqual(self.translate(), [0.0, 0.0])


class TestRelease(PointerTest):
    def test_release_after_an_edit_returns_to_hover(self):
        self.mode.move(_Event((50.0, 25.0)))
        self.mode.push(_Event((50.0, 25.0)))
        self.mode.release(_Event())
        self.assertEqual(
            self.graph.getStringProperty("t_a.tag.tmanip_state"), ["hover"])
        self.assertEqual(self.cursors[-1], Qt.CursorShape.OpenHandCursor.value)

    def test_release_without_an_edit_restores_the_arrow(self):
        self.mode.release(_Event())
        self.assertEqual(self.cursors[-1], Qt.CursorShape.ArrowCursor.value)

    def test_release_clears_the_editing_flags(self):
        self.mode.move(_Event((50.0, 25.0)))
        self.mode.push(_Event((50.0, 25.0)))
        self.mode.drag(_Event((60.0, 25.0)))
        self.mode.release(_Event())
        self.assertFalse(self.mode._editing)
        self.assertFalse(self.mode._didDrag)


class TestDeactivate(PointerTest):
    def test_deactivate_removes_the_tags(self):
        """These tags get written into the saved session if they survive."""
        self.graph.seedString("t_a.tag.tmanip", ["t_a"])
        self.graph.seedString("t_a.tag.tmanip_state", ["hover"])
        self.mode._active = True

        self.mode.deactivate()

        self.assertFalse(self.graph.propertyExists("t_a.tag.tmanip"))
        self.assertFalse(self.graph.propertyExists("t_a.tag.tmanip_state"))

    def test_deactivate_restores_the_arrow_cursor(self):
        self.mode._active = True
        self.mode.deactivate()
        self.assertEqual(self.cursors[-1], Qt.CursorShape.ArrowCursor.value)


if __name__ == "__main__":
    unittest.main()
