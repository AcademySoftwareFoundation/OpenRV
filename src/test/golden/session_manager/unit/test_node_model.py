"""Gate 5 — NodeModel's drag mime encoding on the port itself.

mimeData() is what makes a tree row droppable onto another RV window or an external
app, and it is the one piece of the drag path with an observable payload, so it is
tested by reading the QMimeData the real model produces.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtGui, QtWidgets
    from PySide6.QtCore import Qt

_app = None


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)
    global _app
    _app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])


class NodeModelTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        self.model = self.sm.NodeModel(None)

    def _row(self, text, node):
        item = QtGui.QStandardItem(text)
        item.setData(node, Qt.UserRole + 2)
        self.model.appendRow([item])
        return item


class TestMimeTypes(NodeModelTest):
    def test_adds_uri_list_and_plain_text(self):
        types = self.model.mimeTypes()
        self.assertIn("text/uri-list", types)
        self.assertIn("text/plain", types)

    def test_keeps_the_base_class_types(self):
        base = QtGui.QStandardItemModel().mimeTypes()
        types = self.model.mimeTypes()
        for t in base:
            self.assertIn(t, types)


class TestMimeData(NodeModelTest):
    def test_source_group_encodes_one_url_per_media(self):
        self.graph.addNode("sourceGroup000000", "RVSourceGroup")
        self.graph.seedString(
            "sourceGroup000000_source.media.movie", ["a.mov", "b.mov"])
        item = self._row("Src", "sourceGroup000000")

        data = self.model.mimeData([self.model.indexFromItem(item)])
        urls = [u.toString() for u in data.urls()]

        self.assertEqual(len(urls), 2)
        for url in urls:
            self.assertTrue(url.startswith("rvnode://"))
        self.assertTrue(any(u.endswith("a.mov") for u in urls))
        self.assertTrue(any(u.endswith("b.mov") for u in urls))
        self.assertIn("RVFileSource", data.text())
        self.assertIn("media.movie", data.text())

    def test_non_source_node_encodes_type_and_name_only(self):
        self.graph.addNode("sequenceGroup", "RVSequenceGroup")
        item = self._row("Seq", "sequenceGroup")

        data = self.model.mimeData([self.model.indexFromItem(item)])
        urls = [u.toString() for u in data.urls()]

        self.assertEqual(len(urls), 1)
        self.assertIn("RVSequenceGroup", urls[0])
        self.assertIn("sequenceGroup", urls[0])
        self.assertEqual(data.text(), "RVSequenceGroup sequenceGroup\n")

    def test_multiple_indices_accumulate(self):
        self.graph.addNode("seqA", "RVSequenceGroup")
        self.graph.addNode("seqB", "RVStackGroup")
        a = self._row("A", "seqA")
        b = self._row("B", "seqB")

        data = self.model.mimeData(
            [self.model.indexFromItem(a), self.model.indexFromItem(b)]
        )

        self.assertEqual(len(data.urls()), 2)
        self.assertIn("seqA", data.text())
        self.assertIn("seqB", data.text())

    def test_missing_media_property_does_not_lose_the_mime_object(self):
        """A source group with no media property must not abort the whole drag.

        mimeData() catches and reports; the caller still needs a usable QMimeData.
        """
        self.graph.addNode("sourceGroup000001", "RVSourceGroup")
        item = self._row("Src", "sourceGroup000001")

        data = self.model.mimeData([self.model.indexFromItem(item)])
        self.assertIsNotNone(data)


if __name__ == "__main__":
    unittest.main()
