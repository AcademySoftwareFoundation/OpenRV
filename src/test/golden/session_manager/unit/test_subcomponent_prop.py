"""Gate 5 — subComponentPropValue on the port itself.

This builds the request.imageComponent value for a clicked sub-component row, so its
shape per sub-type is what decides which view/layer/channel the source resolves to.
The channel case walks its parent recursively and its length assertions encode which
parent shapes Mu considers possible.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QStandardItem


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


class PropValueTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")

    def _sub(self, subType, value):
        item = QStandardItem(str(value))
        item.setData(subType, Qt.UserRole + 4)
        item.setData(value, Qt.UserRole + 5)
        return item


class TestSubComponentPropValue(PropValueTest):
    def test_media_has_no_request_value(self):
        media = self._sub(self.sm.MediaSubComponent, "m.exr")
        self.assertEqual(self.sm.subComponentPropValue(media), [])

    def test_non_subcomponent_has_no_request_value(self):
        self.assertEqual(self.sm.subComponentPropValue(QStandardItem("Src")), [])

    def test_view(self):
        view = self._sub(self.sm.ViewSubComponent, "left")
        self.assertEqual(self.sm.subComponentPropValue(view), ["view", "left"])

    def test_layer_under_a_view_carries_the_view(self):
        view = self._sub(self.sm.ViewSubComponent, "left")
        layer = self._sub(self.sm.LayerSubComponent, "diffuse")
        view.appendRow([layer])
        self.assertEqual(
            self.sm.subComponentPropValue(layer), ["layer", "left", "diffuse"]
        )

    def test_layer_under_media_has_an_empty_view_slot(self):
        media = self._sub(self.sm.MediaSubComponent, "m.exr")
        layer = self._sub(self.sm.LayerSubComponent, "diffuse")
        media.appendRow([layer])
        self.assertEqual(self.sm.subComponentPropValue(layer), ["layer", "", "diffuse"])

    def test_channel_under_a_layer_under_a_view(self):
        view = self._sub(self.sm.ViewSubComponent, "left")
        layer = self._sub(self.sm.LayerSubComponent, "diffuse")
        channel = self._sub(self.sm.ChannelSubComponent, "R")
        view.appendRow([layer])
        layer.appendRow([channel])
        self.assertEqual(
            self.sm.subComponentPropValue(channel),
            ["channel", "left", "diffuse", "R"],
        )

    def test_channel_under_a_view(self):
        view = self._sub(self.sm.ViewSubComponent, "left")
        channel = self._sub(self.sm.ChannelSubComponent, "R")
        view.appendRow([channel])
        self.assertEqual(
            self.sm.subComponentPropValue(channel), ["channel", "left", "", "R"]
        )

    def test_channel_under_media_has_both_slots_empty(self):
        media = self._sub(self.sm.MediaSubComponent, "m.exr")
        channel = self._sub(self.sm.ChannelSubComponent, "R")
        media.appendRow([channel])
        self.assertEqual(
            self.sm.subComponentPropValue(channel), ["channel", "", "", "R"]
        )

    def test_every_shape_is_accepted_by_the_length_assertion(self):
        """The channel branch asserts its parent value is 0, 2 or 3 long.

        Each parent shape reachable in the tree is exercised above; this checks the
        assertion never trips for a layer parent, whose value is 3 long.
        """
        view = self._sub(self.sm.ViewSubComponent, "left")
        layer = self._sub(self.sm.LayerSubComponent, "diffuse")
        view.appendRow([layer])
        self.assertEqual(len(self.sm.subComponentPropValue(layer)), 3)


if __name__ == "__main__":
    unittest.main()
