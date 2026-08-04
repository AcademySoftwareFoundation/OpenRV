"""Gate 5 — the request.imageComponent helpers on the port itself.

These drive sub-component selection (which view/layer/channel the source resolves
to), which is primary outcome-adjacent: the property they write is what the
behavioral gate reads back out of session.rv, and a wrong reload() decision shows up
as a stale viewport.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class RequestTest(unittest.TestCase):
    PROP = "src_source.request.imageComponent"

    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        # "#RVSource.request.<name>" resolves against the current view node.
        self.graph.addNode("src", "RVSourceGroup")
        self.graph.addNode("src_source", "RVSource", group="src")
        self.graph.viewNode = "src"
        self.graph.seedString(self.PROP, [])

    def value(self):
        return self.graph.getStringProperty(self.PROP)


class TestIsImageRequestPropEqual(RequestTest):
    def test_equal_and_unequal(self):
        self.graph.seedString(self.PROP, ["view", "left"])
        self.assertTrue(
            self.sm.isImageRequestPropEqual("imageComponent", ["view", "left"])
        )
        self.assertFalse(
            self.sm.isImageRequestPropEqual("imageComponent", ["view", "right"])
        )

    def test_order_matters(self):
        self.graph.seedString(self.PROP, ["view", "left"])
        self.assertFalse(
            self.sm.isImageRequestPropEqual("imageComponent", ["left", "view"])
        )

    def test_empty_matches_empty(self):
        self.assertTrue(self.sm.isImageRequestPropEqual("imageComponent", []))


class TestSetImageRequestProp(RequestTest):
    def test_write_and_reload_when_changed(self):
        before = self.graph.reloaded
        self.sm.setImageRequestProp("imageComponent", ["view", "left"])
        self.assertEqual(self.value(), ["view", "left"])
        self.assertEqual(self.graph.reloaded, before + 1)

    def test_no_reload_when_unchanged(self):
        self.sm.setImageRequestProp("imageComponent", ["view", "left"])
        before = self.graph.reloaded
        self.sm.setImageRequestProp("imageComponent", ["view", "left"])
        self.assertEqual(self.graph.reloaded, before,
                         "an unchanged request must not force a reload")


class TestSetImageRequestToggle(RequestTest):
    def test_first_click_selects(self):
        self.sm.setImageRequest(["view", "left"])
        self.assertEqual(self.value(), ["view", "left"])

    def test_second_click_on_the_same_value_deselects(self):
        self.sm.setImageRequest(["view", "left"])
        self.sm.setImageRequest(["view", "left"])
        self.assertEqual(self.value(), [],
                         "re-picking the current sub-component clears the request")

    def test_clicking_a_different_value_replaces(self):
        self.sm.setImageRequest(["view", "left"])
        self.sm.setImageRequest(["view", "right"])
        self.assertEqual(self.value(), ["view", "right"])

    def test_toggle_off_disables_the_deselect(self):
        self.sm.setImageRequest(["view", "left"], toggle=False)
        self.sm.setImageRequest(["view", "left"], toggle=False)
        self.assertEqual(self.value(), ["view", "left"])


class TestSetNodeRequest(RequestTest):
    def test_writes_the_named_nodes_property(self):
        self.sm.setNodeRequest("src_source", ["layer", "left", "diffuse"])
        self.assertEqual(self.value(), ["layer", "left", "diffuse"])

    def test_a_missing_property_raises_rather_than_being_created(self):
        """setNodeRequest writes without cprop(), exactly as Mu does.

        session_manager.mu.in's setNodeRequest calls setStringProperty directly, so
        neither implementation creates the property; a source that somehow lacks
        request.imageComponent makes both raise badProperty. An earlier version of
        this test asserted the property was created, which the lenient FakeGraph
        allowed and real RV would not.
        """
        self.graph.deleteProperty(self.PROP)
        with self.assertRaises(Exception):
            self.sm.setNodeRequest("src_source", ["view", "left"])


if __name__ == "__main__":
    unittest.main()
