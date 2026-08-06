"""Gate 5 — session_manager.py free-function helpers, exercised on the port itself.

Every test here calls into ``session_manager`` as imported from the package source;
none of them restate the logic locally. The RV bindings are faked by ``_rv_stubs``
(see its docstring for why), and PySide6 is the real one, since the item/model
helpers are most of what these functions touch.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QStandardItem, QStandardItemModel


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)


def _item(text="", node=None, subType=None, value=None, parentNode=None,
          hash_=None, media=None):
    """A QStandardItem carrying the same UserRole payload newNodeRow() writes."""
    item = QStandardItem(text)
    if node is not None:
        item.setData(node, Qt.UserRole + 2)
    if parentNode is not None:
        item.setData(parentNode, Qt.UserRole + 1)
    if subType is not None:
        item.setData(subType, Qt.UserRole + 4)
    if value is not None:
        item.setData(value, Qt.UserRole + 5)
    if hash_ is not None:
        item.setData(hash_, Qt.UserRole + 6)
    if media is not None:
        item.setData(media, Qt.UserRole + 7)
    return item


class HelperTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")


class TestItemNode(HelperTest):
    def test_returns_node_from_user_role(self):
        self.assertEqual(self.sm.itemNode(_item("row", node="sourceGroup000000")),
                         "sourceGroup000000")

    def test_structural_row_has_no_node(self):
        self.assertEqual(self.sm.itemNode(_item("SOURCES")), "")

    def test_none_item(self):
        self.assertEqual(self.sm.itemNode(None), "")


class TestSubComponentTypeForName(HelperTest):
    def test_known_names(self):
        self.assertEqual(self.sm.itemSubComponentTypeForName("view"),
                         self.sm.ViewSubComponent)
        self.assertEqual(self.sm.itemSubComponentTypeForName("layer"),
                         self.sm.LayerSubComponent)
        self.assertEqual(self.sm.itemSubComponentTypeForName("channel"),
                         self.sm.ChannelSubComponent)

    def test_unknown_and_empty(self):
        self.assertEqual(self.sm.itemSubComponentTypeForName("media"),
                         self.sm.NotASubComponent)
        self.assertEqual(self.sm.itemSubComponentTypeForName(""),
                         self.sm.NotASubComponent)


class TestComponentMatch(HelperTest):
    def test_match_and_mismatch(self):
        self.assertTrue(self.sm.componentMatch("view", self.sm.ViewSubComponent))
        self.assertFalse(self.sm.componentMatch("view", self.sm.LayerSubComponent))

    def test_unknown_name_matches_not_a_subcomponent(self):
        self.assertTrue(self.sm.componentMatch("zzz", self.sm.NotASubComponent))


class TestSubComponentAccessors(HelperTest):
    def test_each_accessor_reads_its_own_role(self):
        item = _item(value="left", parentNode="parentNode", hash_="H", media="m.exr")
        self.assertEqual(self.sm.itemSubComponentValue(item), "left")
        self.assertEqual(self.sm.itemParentNode(item), "parentNode")
        self.assertEqual(self.sm.itemSubComponentHash(item), "H")
        self.assertEqual(self.sm.itemSubComponentMedia(item), "m.exr")

    def test_absent_roles_are_empty_string(self):
        item = _item()
        self.assertEqual(self.sm.itemSubComponentValue(item), "")
        self.assertEqual(self.sm.itemSubComponentMedia(item), "")

    def test_none_item_is_empty_string(self):
        self.assertEqual(self.sm.itemSubComponentStringData(None, 5), "")


class TestSubComponentType(HelperTest):
    def test_reads_int_role(self):
        self.assertEqual(
            self.sm.itemSubComponentType(_item(subType=self.sm.LayerSubComponent)),
            self.sm.LayerSubComponent,
        )

    def test_missing_role_is_not_a_subcomponent(self):
        self.assertEqual(self.sm.itemSubComponentType(_item()),
                         self.sm.NotASubComponent)

    def test_is_subcomponent_predicate(self):
        self.assertTrue(
            self.sm.itemIsSubComponent(_item(subType=self.sm.ViewSubComponent))
        )
        self.assertFalse(self.sm.itemIsSubComponent(_item()))
        self.assertFalse(
            self.sm.itemIsSubComponent(_item(subType=self.sm.NotASubComponent))
        )


class TestIncludes(HelperTest):
    def test_matches_on_row(self):
        model = QStandardItemModel()
        for text in ("a", "b", "c"):
            model.appendRow(QStandardItem(text))
        i0 = model.index(0, 0)
        i1 = model.index(1, 0)
        self.assertTrue(self.sm.includes([i0, i1], model.index(1, 0)))
        self.assertFalse(self.sm.includes([i0], model.index(2, 0)))
        self.assertFalse(self.sm.includes([], i0))


class TestSourceNodeOfGroup(HelperTest):
    def test_finds_file_source(self):
        self.graph.addNode("g", "RVSourceGroup")
        self.graph.addNode("g_other", "RVLinearize", group="g")
        self.graph.addNode("g_source", "RVFileSource", group="g")
        self.assertEqual(self.sm.sourceNodeOfGroup("g"), "g_source")

    def test_finds_image_source(self):
        self.graph.addNode("g", "RVSourceGroup")
        self.graph.addNode("g_img", "RVImageSource", group="g")
        self.assertEqual(self.sm.sourceNodeOfGroup("g"), "g_img")

    def test_none_when_group_has_no_source(self):
        self.graph.addNode("g", "RVSourceGroup")
        self.graph.addNode("g_c", "RVColor", group="g")
        self.assertIsNone(self.sm.sourceNodeOfGroup("g"))


class TestPropertySetters(HelperTest):
    """setIntProp / setFloatProp / setStringProp stand in for Mu's set() overloads."""

    def test_creates_then_writes_int(self):
        self.sm.setIntProp("n.comp.p", 7)
        self.assertEqual(self.graph.getIntProperty("n.comp.p"), [7])

    def test_scalar_and_array_forms_agree(self):
        self.sm.setFloatProp("n.comp.f", 1.5)
        self.assertEqual(self.graph.getFloatProperty("n.comp.f"), [1.5])
        self.sm.setFloatProp("n.comp.g", [1.0, 2.0])
        self.assertEqual(self.graph.getFloatProperty("n.comp.g"), [1.0, 2.0])

    def test_string_form(self):
        self.sm.setStringProp("n.comp.s", "hello")
        self.assertEqual(self.graph.getStringProperty("n.comp.s"), ["hello"])

    def test_existing_property_of_other_type_raises(self):
        """Mu's cprop() only creates a missing property; it never retypes one.

        Writing an int to a float property therefore reaches setIntProperty and
        throws badPropertyType, which is the behavior RetimeGroup's reverse() relies
        on and which the port must not paper over.
        """
        self.sm.setFloatProp("n.comp.mixed", 1.0)
        with self.assertRaises(Exception):
            self.sm.setIntProp("n.comp.mixed", 1)


class TestArrayHelpers(HelperTest):
    def test_contents_equal(self):
        self.assertTrue(self.sm.contents_equal(["a", "b"], ["a", "b"]))
        self.assertFalse(self.sm.contents_equal(["a"], ["a", "b"]))

    def test_compare_orders_strings(self):
        self.assertLess(self.sm._compare("a", "b"), 0)
        self.assertGreater(self.sm._compare("b", "a"), 0)
        self.assertEqual(self.sm._compare("a", "a"), 0)


class TestNodeInputs(HelperTest):
    def test_returns_first_element_of_connections(self):
        self.graph.addNode("seq", "RVSequenceGroup", inputs=["a", "b"])
        self.assertEqual(self.sm.nodeInputs("seq"), ["a", "b"])


class TestAddRow(HelperTest):
    def test_sets_children_as_one_row(self):
        parent = QStandardItem("parent")
        kids = [QStandardItem("c0"), QStandardItem("c1"), QStandardItem("c2")]
        self.sm.addRow(parent, kids)
        self.assertEqual(parent.rowCount(), 1)
        self.assertEqual(parent.child(0, 0).text(), "c0")
        self.assertEqual(parent.child(0, 2).text(), "c2")


class TestMapItems(HelperTest):
    """mapItems() must reproduce Mu's cons-list order, not merely its membership.

    Mu's map() prepends each matching item after visiting its children, so a
    matching parent comes out ahead of its matching descendants. itemOfNode() and
    selectViewableNode() both take the head, so the order is load-bearing: with the
    children first, selectViewableNode() scrolls to a sub-component row and expands
    the node row as a side effect, writing an sm_state.expandState that Mu never
    writes.
    """

    def _tree(self):
        model = QStandardItemModel()
        category = _item("SOURCES")
        node = _item("Src", node="src")
        subA = _item("media", node="src", subType=self.sm.MediaSubComponent)
        subB = _item("view", node="src", subType=self.sm.ViewSubComponent)
        node.appendRow([subA])
        node.appendRow([subB])
        category.appendRow([node])
        model.appendRow([category])
        return model, node, subA, subB

    def test_parent_precedes_its_children(self):
        model, node, subA, subB = self._tree()
        got = self.sm.mapItems(model, lambda i: self.sm.itemNode(i) == "src")
        self.assertEqual([i.text() for i in got][0], "Src")

    def test_children_come_out_in_reverse_order(self):
        model, node, subA, subB = self._tree()
        got = self.sm.mapItems(model, lambda i: self.sm.itemNode(i) == "src")
        self.assertEqual([i.text() for i in got], ["Src", "view", "media"])

    def test_structural_rows_are_never_returned(self):
        model, _, _, _ = self._tree()
        got = self.sm.mapItems(model, lambda i: True)
        self.assertNotIn("SOURCES", [i.text() for i in got])

    def test_root_argument_limits_the_walk(self):
        model, node, _, _ = self._tree()
        got = self.sm.mapItems(model, lambda i: True, root=node)
        self.assertEqual([i.text() for i in got], ["Src", "view", "media"])

    def test_item_of_node_skips_subcomponents(self):
        model, node, _, _ = self._tree()
        self.assertIs(self.sm.itemOfNode(model, "src"), node)

    def test_item_of_node_none_when_absent(self):
        model, _, _, _ = self._tree()
        self.assertIsNone(self.sm.itemOfNode(model, "nope"))


class TestSubComponentItemsOfNode(HelperTest):
    def test_excludes_media_and_non_subcomponents(self):
        model = QStandardItemModel()
        node = _item("Src", node="src")
        media = _item("media", node="src", subType=self.sm.MediaSubComponent)
        view = _item("view", node="src", subType=self.sm.ViewSubComponent)
        layer = _item("layer", node="src", subType=self.sm.LayerSubComponent)
        for child in (media, view, layer):
            node.appendRow([child])
        model.appendRow([node])

        got = [i.text() for i in self.sm.subComponentItemsOfNode(model, "src")]
        self.assertIn("view", got)
        self.assertIn("layer", got)
        self.assertNotIn("media", got)
        self.assertNotIn("Src", got)


class TestNodeFromIndex(HelperTest):
    def test_resolves_index_to_node(self):
        model = QStandardItemModel()
        model.appendRow([_item("Src", node="src")])
        self.assertEqual(self.sm.nodeFromIndex(model.index(0, 0), model), "src")


class TestResizeColumns(HelperTest):
    def test_resizes_every_column(self):
        import PySide6.QtWidgets as QtWidgets

        model = QStandardItemModel()
        model.setColumnCount(3)
        model.appendRow([QStandardItem("a"), QStandardItem("b"), QStandardItem("c")])
        view = QtWidgets.QTreeView()
        view.setModel(model)

        called = []
        view.resizeColumnToContents = lambda c: called.append(c)
        self.sm.resizeColumns(view, model)
        self.assertEqual(called, [0, 1, 2])


if __name__ == "__main__":
    unittest.main()
