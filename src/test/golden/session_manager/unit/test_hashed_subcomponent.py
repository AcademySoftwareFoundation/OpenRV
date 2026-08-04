"""Gate 5 — hashedSubComponent, both overloads, on the port itself.

The hash is the key sub-component expansion state is stored under, so a change in its
encoding silently loses a user's expanded rows rather than failing loudly. Mu
distinguishes an absent field (nil) from a present-but-empty one ("", encoded "@."),
and the port models absent as None; both overloads are checked against that.
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


class HashTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")

    def _sub(self, text, subType, value):
        item = QStandardItem(text)
        item.setData(subType, Qt.UserRole + 4)
        item.setData(value, Qt.UserRole + 5)
        return item


class TestHashedSubComponentOf(HashTest):
    def test_media_only(self):
        self.assertEqual(self.sm.hashedSubComponentOf("m.exr", None, None), "m.exr!~!~")

    def test_media_and_view(self):
        self.assertEqual(
            self.sm.hashedSubComponentOf("m.exr", "left", None), "m.exr!~!~left"
        )

    def test_media_and_layer(self):
        self.assertEqual(
            self.sm.hashedSubComponentOf("m.exr", None, "diffuse"), "m.exr!~diffuse!~"
        )

    def test_all_three_put_layer_before_view(self):
        """Mu's three-argument form emits media, layer, view — in that order."""
        self.assertEqual(
            self.sm.hashedSubComponentOf("m.exr", "left", "diffuse"),
            "m.exr!~diffuse!~left",
        )

    def test_empty_view_is_encoded_not_dropped(self):
        self.assertEqual(
            self.sm.hashedSubComponentOf("m.exr", "", None), "m.exr!~!~@."
        )

    def test_empty_layer_is_encoded_not_dropped(self):
        self.assertEqual(
            self.sm.hashedSubComponentOf("m.exr", None, ""), "m.exr!~@.!~"
        )

    def test_empty_is_distinct_from_absent(self):
        self.assertNotEqual(
            self.sm.hashedSubComponentOf("m.exr", "", None),
            self.sm.hashedSubComponentOf("m.exr", None, None),
        )


class TestHashedSubComponentOfItem(HashTest):
    def test_media_item(self):
        media = self._sub("m.exr", self.sm.MediaSubComponent, "m.exr")
        self.assertEqual(self.sm.hashedSubComponent(media), "m.exr!~!~")

    def test_view_item_uses_its_parent_media(self):
        media = self._sub("m.exr", self.sm.MediaSubComponent, "m.exr")
        view = self._sub("left", self.sm.ViewSubComponent, "left")
        media.appendRow([view])
        self.assertEqual(self.sm.hashedSubComponent(view), "m.exr!~!~left")

    def test_layer_under_a_view_picks_up_media_and_view(self):
        media = self._sub("m.exr", self.sm.MediaSubComponent, "m.exr")
        view = self._sub("left", self.sm.ViewSubComponent, "left")
        layer = self._sub("diffuse", self.sm.LayerSubComponent, "diffuse")
        media.appendRow([view])
        view.appendRow([layer])
        self.assertEqual(self.sm.hashedSubComponent(layer), "m.exr!~diffuse!~left")

    def test_layer_directly_under_media_has_no_view(self):
        media = self._sub("m.exr", self.sm.MediaSubComponent, "m.exr")
        layer = self._sub("diffuse", self.sm.LayerSubComponent, "diffuse")
        media.appendRow([layer])
        self.assertEqual(self.sm.hashedSubComponent(layer), "m.exr!~diffuse!~")

    def test_channel_item_has_no_hash(self):
        """Channels are never recorded as expanded, so they hash to ""."""
        media = self._sub("m.exr", self.sm.MediaSubComponent, "m.exr")
        channel = self._sub("R", self.sm.ChannelSubComponent, "R")
        media.appendRow([channel])
        self.assertEqual(self.sm.hashedSubComponent(channel), "")

    def test_plain_node_row_has_no_hash(self):
        item = QStandardItem("Src")
        self.assertEqual(self.sm.hashedSubComponent(item), "")

    def test_item_and_string_overloads_agree(self):
        media = self._sub("m.exr", self.sm.MediaSubComponent, "m.exr")
        view = self._sub("left", self.sm.ViewSubComponent, "left")
        layer = self._sub("diffuse", self.sm.LayerSubComponent, "diffuse")
        media.appendRow([view])
        view.appendRow([layer])
        self.assertEqual(
            self.sm.hashedSubComponent(layer),
            self.sm.hashedSubComponentOf("m.exr", "left", "diffuse"),
        )


if __name__ == "__main__":
    unittest.main()
