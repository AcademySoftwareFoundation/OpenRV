"""Gate 5 — TransformManip's stateful methods on the port itself.

The vector helpers are covered in test_transform_manip.py; this covers the mode
methods that mutate the graph: the tmanip tag lifecycle, the corner hit test, and the
two menu actions. The manipulator is invisible to the golden scenarios (it draws in
GL and is driven by pointer events), so these are the only tests that pin it.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class ManipModeTest(unittest.TestCase):
    def setUp(self):
        try:
            self.mod, self.graph = _rv_stubs.importPort("transform_manip")
        except ImportError as exc:
            raise unittest.SkipTest("transform_manip needs PyOpenGL: %s" % exc)
        self.mode = self.mod.TransformManip.__new__(self.mod.TransformManip)
        self.mode._editNodes = []
        self.graph.addNode("layoutGroup", "RVLayoutGroup")
        self.graph.viewNode = "layoutGroup"

    def pair(self, tform, inputNode):
        self.graph.addNode(tform, "RVTransform2D")
        return self.mod.EditNodePair(tform, inputNode)


class TestEditNode(ManipModeTest):
    def test_finds_the_pair_by_transform_node(self):
        a = self.pair("t_a", "src_a")
        b = self.pair("t_b", "src_b")
        self.mode._editNodes = [a, b]
        self.assertIs(self.mode.editNode("t_b"), b)

    def test_unknown_name_is_none(self):
        self.mode._editNodes = [self.pair("t_a", "src_a")]
        self.assertIsNone(self.mode.editNode("nope"))

    def test_empty_list_is_none(self):
        self.assertIsNone(self.mode.editNode("t_a"))


class TestSetManipState(ManipModeTest):
    def test_writes_the_tag_property(self):
        p = self.pair("t_a", "src_a")
        self.mode.setManipState(p, "hover")
        self.assertEqual(
            self.graph.getStringProperty("t_a.tag.tmanip_state"), ["hover"]
        )

    def test_none_pair_is_a_noop(self):
        self.mode.setManipState(None, "hover")   # must not raise

    def test_missing_node_is_a_noop(self):
        p = self.mod.EditNodePair("notANode", "src")
        self.mode.setManipState(p, "hover")
        self.assertFalse(self.graph.propertyExists("notANode.tag.tmanip_state"))


class TestActiveImageIndex(ManipModeTest):
    def test_returns_the_index_of_the_tagged_image(self):
        self.graph.renderedImages = lambda: [
            {"index": 0, "tags": [("other", "x")]},
            {"index": 7, "tags": [("tmanip_state", "hover")]},
        ]
        self.mod.commands.renderedImages = self.graph.renderedImages
        self.assertEqual(self.mode.activeImageIndex(), 7)

    def test_empty_tag_value_does_not_count(self):
        self.graph.renderedImages = lambda: [
            {"index": 3, "tags": [("tmanip_state", "")]},
        ]
        self.mod.commands.renderedImages = self.graph.renderedImages
        self.assertEqual(self.mode.activeImageIndex(), -1)

    def test_no_images_is_minus_one(self):
        self.mod.commands.renderedImages = lambda: []
        self.assertEqual(self.mode.activeImageIndex(), -1)


class TestControlHitTest(ManipModeTest):
    """Which corner the pointer grabs, given a 100x100 image centred on (50,50)."""

    CORNERS = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]

    def _control(self, pointer):
        self.mod.commands.imageGeometryByIndex = lambda i: self.CORNERS

        class _Ev:
            def pointer(self_inner):
                return pointer

        return self.mode.control(0, _Ev())

    def test_bottom_left(self):
        control, gc, corner = self._control((2.0, 2.0))
        self.assertEqual(control, self.mod.BotLeftCorner)
        self.assertEqual(tuple(corner), (0.0, 0.0))

    def test_bottom_right(self):
        control, _, corner = self._control((99.0, 2.0))
        self.assertEqual(control, self.mod.BotRightCorner)

    def test_top_right(self):
        control, _, _ = self._control((99.0, 99.0))
        self.assertEqual(control, self.mod.TopRightCorner)

    def test_top_left(self):
        control, _, _ = self._control((2.0, 99.0))
        self.assertEqual(control, self.mod.TopLeftCorner)

    def test_centre_is_a_free_translation(self):
        control, gc, corner = self._control((50.0, 50.0))
        self.assertEqual(control, self.mod.FreeTranslation)
        self.assertEqual(tuple(gc), tuple(corner),
                         "a free translation reports the centroid as the grab point")

    def test_just_outside_the_corner_radius_is_free_translation(self):
        """The corner hit box is 25 units; 30 away must not grab a corner."""
        control, _, _ = self._control((30.0, 30.0))
        self.assertEqual(control, self.mod.FreeTranslation)


class TestResetAll(ManipModeTest):
    def test_resets_every_edit_node(self):
        self.mode._editNodes = [self.pair("t_a", "s_a"), self.pair("t_b", "s_b")]
        for n in ("t_a", "t_b"):
            self.graph.seedFloat(n + ".transform.translate", [5.0, 5.0])
            self.graph.seedFloat(n + ".transform.scale", [2.0, 2.0])
            self.graph.seedFloat(n + ".transform.rotate", [45.0])

        self.mode.resetAll(None)

        for n in ("t_a", "t_b"):
            self.assertEqual(
                self.graph.getFloatProperty(n + ".transform.translate"), [0.0, 0.0])
            self.assertEqual(
                self.graph.getFloatProperty(n + ".transform.scale"), [1.0, 1.0])
            self.assertEqual(
                self.graph.getFloatProperty(n + ".transform.rotate"), [0.0])

    def test_redraws(self):
        before = self.graph.redraws
        self.mode.resetAll(None)
        self.assertEqual(self.graph.redraws, before + 1)

    def test_no_edit_nodes_is_harmless(self):
        self.mode.resetAll(None)   # must not raise


class TestFitAll(ManipModeTest):
    """fitAll's scale is always 1.0, because nodeAspect() ignores its argument.

    nodeAspect(node) measures nodeImageGeometry(viewNode(), ...) and never looks at
    `node` — transform_manip.mu:294 does the same, so this is Mu behavior the port
    reproduces rather than a porting mistake. The consequence is that
    `s = aspect / inaspect` divides the view aspect by itself, so "Fit All Images"
    only ever resets the transforms instead of fitting anything. Pinned as-is: fixing
    it would change what the command does, which is a product decision and something
    no committed golden currently covers.
    """

    def _geometry(self, mapping):
        self.mod.commands.nodeImageGeometry = lambda node, frame: mapping[node]

    def test_scale_is_unity_even_when_the_aspects_differ(self):
        self.mode._editNodes = [self.pair("t_a", "s_a")]
        self._geometry({
            "layoutGroup": {"width": 200, "height": 100, "pixelAspect": 1.0},
            "t_a": {"width": 100, "height": 100, "pixelAspect": 1.0},
        })

        self.mode.fitAll(None)

        self.assertEqual(self.graph.getFloatProperty("t_a.transform.scale"), [1.0, 1.0])

    def test_transform_is_otherwise_reset(self):
        self.mode._editNodes = [self.pair("t_a", "s_a")]
        self._geometry({
            "layoutGroup": {"width": 200, "height": 100, "pixelAspect": 1.0},
            "t_a": {"width": 100, "height": 100, "pixelAspect": 1.0},
        })
        self.graph.seedFloat("t_a.transform.translate", [9.0, 9.0])
        self.graph.seedFloat("t_a.transform.rotate", [45.0])

        self.mode.fitAll(None)

        self.assertEqual(
            self.graph.getFloatProperty("t_a.transform.translate"), [0.0, 0.0])
        self.assertEqual(self.graph.getFloatProperty("t_a.transform.rotate"), [0.0])

    def test_matching_aspects_also_give_unit_scale(self):
        self.mode._editNodes = [self.pair("t_a", "s_a")]
        self._geometry({
            "layoutGroup": {"width": 100, "height": 100, "pixelAspect": 1.0},
            "t_a": {"width": 100, "height": 100, "pixelAspect": 1.0},
        })
        self.mode.fitAll(None)
        self.assertEqual(self.graph.getFloatProperty("t_a.transform.scale"), [1.0, 1.0])


class TestNodeAspect(ManipModeTest):
    """Note the `node` argument is ignored; the view node is always measured."""

    def test_the_node_argument_is_ignored(self):
        seen = []
        self.mod.commands.nodeImageGeometry = lambda n, f: (
            seen.append(n) or {"width": 100, "height": 100, "pixelAspect": 1.0})
        self.mod.nodeAspect("someOtherNode")
        self.assertEqual(seen, ["layoutGroup"],
                         "matches transform_manip.mu:294, which measures viewNode()")

    def test_wide_pixel_aspect_widens(self):
        self.mod.commands.nodeImageGeometry = lambda n, f: {
            "width": 100, "height": 100, "pixelAspect": 2.0}
        self.assertAlmostEqual(self.mod.nodeAspect("layoutGroup"), 2.0)

    def test_narrow_pixel_aspect_narrows(self):
        self.mod.commands.nodeImageGeometry = lambda n, f: {
            "width": 100, "height": 100, "pixelAspect": 0.5}
        self.assertAlmostEqual(self.mod.nodeAspect("layoutGroup"), 0.5)

    def test_square_pixels(self):
        self.mod.commands.nodeImageGeometry = lambda n, f: {
            "width": 160, "height": 80, "pixelAspect": 1.0}
        self.assertAlmostEqual(self.mod.nodeAspect("layoutGroup"), 2.0)


class TestTagLifecycle(ManipModeTest):
    def _infos(self, nodes):
        self.mod.commands.metaEvaluateClosestByType = lambda f, t: [
            {"node": n} for n in nodes
        ]

    def test_find_editing_nodes_tags_each_transform(self):
        self.graph.addNode("t_a", "RVTransform2D")
        self.graph.connections["layoutGroup"] = ["s_a"]
        self._infos(["t_a"])

        self.mode.findEditingNodes()

        self.assertEqual(len(self.mode._editNodes), 1)
        self.assertEqual(self.graph.getStringProperty("t_a.tag.tmanip"), ["t_a"])
        self.assertEqual(self.graph.getStringProperty("t_a.tag.tmanip_state"), [""])

    def test_mismatched_counts_bail_out(self):
        """Happens during teardown; tagging half a graph would leave stale tags."""
        self.graph.connections["layoutGroup"] = ["s_a", "s_b"]
        self._infos(["t_a"])

        self.mode.findEditingNodes()

        self.assertEqual(self.mode._editNodes, [])

    def test_set_states_false_preserves_an_existing_tag(self):
        self.graph.addNode("t_a", "RVTransform2D")
        self.graph.connections["layoutGroup"] = ["s_a"]
        self._infos(["t_a"])
        self.graph.seedString("t_a.tag.tmanip_state", ["editing"])
        self.graph.seedString("t_a.tag.tmanip", ["t_a"])

        self.mode.findEditingNodes(False)

        self.assertEqual(
            self.graph.getStringProperty("t_a.tag.tmanip_state"), ["editing"],
            "an in-progress edit must survive an inputs-changed refresh")

    def test_remove_tags_deletes_both_properties(self):
        self.mode._editNodes = [self.pair("t_a", "s_a")]
        self.graph.seedString("t_a.tag.tmanip", ["t_a"])
        self.graph.seedString("t_a.tag.tmanip_state", [""])

        self.mode.removeTags()

        self.assertFalse(self.graph.propertyExists("t_a.tag.tmanip"))
        self.assertFalse(self.graph.propertyExists("t_a.tag.tmanip_state"))

    def test_remove_tags_tolerates_absent_properties(self):
        self.mode._editNodes = [self.pair("t_a", "s_a")]
        self.mode.removeTags()   # must not raise

    def test_before_view_change_removes_tags_and_rejects(self):
        self.mode._editNodes = [self.pair("t_a", "s_a")]
        self.graph.seedString("t_a.tag.tmanip", ["t_a"])
        event = _Event("")
        self.mode.beforeGraphViewChange(event)
        self.assertFalse(self.graph.propertyExists("t_a.tag.tmanip"))
        self.assertTrue(event.rejected)

    def test_inputs_changed_on_the_view_node_refreshes_without_resetting_state(self):
        self.graph.addNode("t_a", "RVTransform2D")
        self.graph.connections["layoutGroup"] = ["s_a"]
        self._infos(["t_a"])
        self.graph.seedString("t_a.tag.tmanip", ["t_a"])
        self.graph.seedString("t_a.tag.tmanip_state", ["editing"])

        self.mode.nodeInputsChanged(_Event("layoutGroup"))

        self.assertEqual(
            self.graph.getStringProperty("t_a.tag.tmanip_state"), ["editing"])

    def test_inputs_changed_on_another_node_is_ignored(self):
        self._infos(["t_a"])
        self.mode.nodeInputsChanged(_Event("someOtherNode"))
        self.assertEqual(self.mode._editNodes, [])


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
