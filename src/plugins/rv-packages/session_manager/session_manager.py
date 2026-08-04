#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""Session Manager mode — Python port of session_manager.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side during the migration, and so each Mu method maps to one
Python method for coverage tracking.
"""
import os
import re
import sys

import rv.commands as commands
import rv.extra_commands as extra_commands
import rv.qtutils as qtutils
import rv.rvtypes

from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtCore import Qt
from PySide6.QtUiTools import QUiLoader

NotASubComponent = 0
MediaSubComponent = 1
ViewSubComponent = 2
LayerSubComponent = 3
ChannelSubComponent = 4

FILMSTRIP_FRAME_WIDTH = 240
SOURCE_PREVIEW_WIDTH = 80
SOURCE_PREVIEW_HEIGHT = 45
SOURCE_ROW_HEIGHT = 55
SOURCE_ROW_MARGIN = 8
SOURCE_ROW_SPACING = 5
SOURCE_TEXT_SPACING = 3
TREE_VIEW_INDENTATION = 10

# Mu's int.max, used as the "no sort key recorded" marker.
MU_INT_MAX = 2**31 - 1
UNDEFINED_SORT_KEY = MU_INT_MAX - 100


def itemNode(item):
    """The node name an item stands for, or "" for structural rows."""
    if item is None:
        return ""
    d = item.data(Qt.UserRole + 2)
    return "" if d is None else str(d)


def itemSubComponentTypeForName(n):
    return {
        "view": ViewSubComponent,
        "layer": LayerSubComponent,
        "channel": ChannelSubComponent,
    }.get(n, NotASubComponent)


def componentMatch(n, c):
    return itemSubComponentTypeForName(n) == c


def itemSubComponentStringData(item, n):
    if item is None:
        return ""
    d = item.data(Qt.UserRole + n)
    return "" if d is None else str(d)


def itemSubComponentMedia(item):
    return itemSubComponentStringData(item, 7)


def itemSubComponentHash(item):
    return itemSubComponentStringData(item, 6)


def itemSubComponentValue(item):
    return itemSubComponentStringData(item, 5)


def itemParentNode(item):
    return itemSubComponentStringData(item, 1)


def itemSubComponentType(item):
    if item is None:
        return NotASubComponent
    d = item.data(Qt.UserRole + 4)
    if d is None:
        return NotASubComponent
    try:
        return int(d)
    except (TypeError, ValueError):
        return NotASubComponent


def itemIsSubComponent(item):
    return itemSubComponentType(item) != NotASubComponent


def includes(array, item):
    return any(a.row() == item.row() for a in array)


def contents_equal(a, b):
    """Mu's contentsEqual, which has no Python counterpart."""
    return list(a) == list(b)


def _compare(a, b):
    """Mu's compare() for strings: negative, zero or positive."""
    return (a > b) - (a < b)


def _cprop(name, propType):
    """Mu's extra_commands.cprop."""
    if not commands.propertyExists(name):
        commands.newProperty(name, propType, 1)


def _as_list(value):
    return list(value) if isinstance(value, (list, tuple)) else [value]


#
#  Mu has six set() overloads (float/int/string, scalar and array) and picks one
#  by argument type. Only a single overload is reachable through the Python
#  binding of extra_commands.set, so the property type is named at the call site
#  instead. Which of these three a call uses matches the overload Mu resolved.
#


def setFloatProp(name, value):
    _cprop(name, commands.FloatType)
    commands.setFloatProperty(name, _as_list(value), True)


def setIntProp(name, value):
    _cprop(name, commands.IntType)
    commands.setIntProperty(name, _as_list(value), True)


def setStringProp(name, value):
    _cprop(name, commands.StringType)
    commands.setStringProperty(name, _as_list(value), True)


def checkStateIsChecked(state):
    """Whether a QCheckBox.stateChanged payload means Checked.

    Mu compares the signal argument against Qt.Checked directly, which cannot be
    done here: stateChanged carries a plain int, and PySide6 6.5's Qt.CheckState is
    an enum.Enum rather than an IntEnum, so `2 == Qt.Checked` is False and
    int(Qt.Checked) raises TypeError. Comparing through .value keeps the Mu answer,
    and reading .value off the argument first also accepts a real CheckState in case
    a caller (or a later Qt) hands one over.
    """
    return getattr(state, "value", state) == Qt.Checked.value


def menuItem(label, eventPattern, category, func, stateFunc):
    """Mu's app_utils.menuItem, which the Python menu API has no counterpart for.

    A Python mode's menu entry is a plain (label, func, key, stateFunc) tuple with no
    notion of an event category, so the gate app_utils.menuItem wraps around both
    callables is reproduced here: while the category is disabled (live review filters
    categories off), the item draws disabled and activating it reports
    category-event-blocked instead of running func.

    Every session_manager menuItem call passes an empty eventPattern, so the bind and
    the derived key accelerator Mu's version would add are both no-ops; the key slot is
    None for that reason rather than as a simplification.
    """
    assert eventPattern == "", "menuItem() shim does not implement event binding"

    def compositeFunc(event):
        if not commands.isEventCategoryEnabled(category):
            commands.sendInternalEvent("category-event-blocked", category)
        else:
            func(event)

    def compositeStateFunc():
        if not commands.isEventCategoryEnabled(category):
            return commands.DisabledMenuState
        return stateFunc()

    return (label, compositeFunc, None, compositeStateFunc)


def sourceNodeOfGroup(group):
    for node in commands.nodesInGroup(group):
        if commands.nodeType(node) in ("RVFileSource", "RVImageSource"):
            return node
    return None


def hashedSubComponentOf(media, view, layer):
    """Mu's hashedSubComponent(string, string, string).

    Mu distinguishes nil (absent) from "" (present but empty) here, and encodes the
    empty-but-present case as "@."; Python uses None for nil.
    """
    v = "@." if view is not None and view == "" else view
    l = "@." if layer is not None and layer == "" else layer

    if v is None and l is None:
        return "%s!~!~" % media
    if v is None:
        return "%s!~%s!~" % (media, l)
    if l is None:
        return "%s!~!~%s" % (media, v)
    return "%s!~%s!~%s" % (media, l, v)


def hashedSubComponent(item):
    """Mu's hashedSubComponent(QStandardItem) overload."""
    value = itemSubComponentValue(item)
    subType = itemSubComponentType(item)
    parent = item.parent()
    pvalue = itemSubComponentValue(parent)

    if subType == MediaSubComponent:
        return hashedSubComponentOf(value, None, None)

    if subType == LayerSubComponent:
        grandParent = parent.parent() if parent is not None else None
        psubType = itemSubComponentType(parent)
        if psubType == ViewSubComponent:
            arg0 = itemSubComponentValue(grandParent)
            arg1 = pvalue
        else:
            arg0 = pvalue
            arg1 = None
        return hashedSubComponentOf(arg0, arg1, value)

    if subType == ViewSubComponent:
        return hashedSubComponentOf(pvalue, value, None)

    return ""


def isSubComponentExpanded(node, item):
    propName = "%s.sm_state.expandedSubState" % node
    key = hashedSubComponent(item)
    if commands.propertyExists(propName):
        return key in commands.getStringProperty(propName)
    return False


def setSubComponentExpanded(node, item, expanded):
    propName = "%s.sm_state.expandedSubState" % node
    key = hashedSubComponent(item)

    if commands.propertyExists(propName):
        p = list(commands.getStringProperty(propName))
        hasit = key in p
        if hasit and not expanded:
            setStringProp(propName, [x for x in p if x != key])
        elif not hasit and expanded:
            p.append(key)
            setStringProp(propName, p)
    else:
        setStringProp(propName, [key])


def isExpandedInParent(node, parent):
    propName = "%s.sm_state.expandState" % node
    if commands.propertyExists(propName):
        return parent in commands.getStringProperty(propName)
    return False


def setExpandedInParent(node, parent, expanded):
    propName = "%s.sm_state.expandState" % node

    if commands.propertyExists(propName):
        p = list(commands.getStringProperty(propName))
        hasNode = parent in p
        if hasNode and not expanded:
            setStringProp(propName, [x for x in p if x != parent])
        elif not hasNode and expanded:
            p.append(parent)
            setStringProp(propName, p)
    else:
        setStringProp(propName, parent)


def setToolTipProp(node, toolTip):
    setStringProp("%s.sm_state.toolTip" % node, toolTip)


def toolTipFromProp(node):
    propName = "%s.sm_state.toolTip" % node
    if commands.propertyExists(propName):
        try:
            return commands.getStringProperty(propName)[0]
        except Exception:
            pass
    return None


def sortKeyInParent(node, parent):
    propNameParent = "%s.sm_state.sortKeyParent" % node
    propNameKey = "%s.sm_state.sortKey" % node

    if commands.propertyExists(propNameParent) and commands.propertyExists(propNameKey):
        try:
            p = list(commands.getStringProperty(propNameParent))
            keys = list(commands.getIntProperty(propNameKey))
            i = p.index(parent) if parent in p else -1
            if i == -1 or len(keys) != len(p):
                return UNDEFINED_SORT_KEY
            return keys[i]
        except Exception:
            pass

    return UNDEFINED_SORT_KEY


def setSortKeyInParent(node, parent, value):
    propNameParent = "%s.sm_state.sortKeyParent" % node
    propNameKey = "%s.sm_state.sortKey" % node

    if commands.propertyExists(propNameParent) and commands.propertyExists(propNameKey):
        try:
            p = list(commands.getStringProperty(propNameParent))
            keys = list(commands.getIntProperty(propNameKey))
            i = p.index(parent) if parent in p else -1

            if len(p) == len(keys):
                if i == -1:
                    p.append(parent)
                    keys.append(value)
                    setStringProp(propNameParent, p)
                    setIntProp(propNameKey, keys)
                else:
                    keys[i] = value
                    setIntProp(propNameKey, keys)
                return
        except Exception:
            pass

    setStringProp(propNameParent, parent)
    setIntProp(propNameKey, value)


def nodeFromIndex(index, model):
    return itemNode(model.itemFromIndex(index))


def nodeInputs(node):
    return commands.nodeConnections(node, False)[0]


def addRow(item, children):
    row = item.rowCount()
    for count, child in enumerate(children):
        item.setChild(row, count, child)


def setInputs(node, inputs):
    """Set a node's inputs, reporting rejected ones the way the Mu mode does."""
    msg = commands.testNodeInputs(node, inputs)

    if msg is not None:
        commands.alertPanel(
            False,
            commands.ErrorAlert,
            "Some inputs are not allowed here",
            msg,
            "Ok",
            None,
            None,
        )
    else:
        commands.setNodeInputs(node, inputs)

    return msg is None


def removeInput(node, inputNode):
    if node != "":
        ins = commands.nodeConnections(node)[0]
        return setInputs(node, [n for n in ins if n != inputNode])
    return True


def hasInput(node, inputNode):
    if node is None or node == "":
        return True
    return inputNode in commands.nodeConnections(node)[0]


def addInput(node, inputNode):
    if commands.nodeExists(node):
        newInputs = list(commands.nodeConnections(node)[0])
        newInputs.append(inputNode)
        return setInputs(node, newInputs)
    return True


def mapItems(model, F, root=None):
    """Every item under `root` (or the whole model) for which F(item) is true.

    Mu's map() accumulates into a cons list and visits an item's children before
    prepending the item itself, so a matching parent ends up ahead of its matching
    children and siblings come out in reverse order. Callers depend on that: both
    itemOfNode() and selectViewableNode() take the head, which is the node's own row
    rather than one of its sub-component rows. Appending instead puts a sub-component
    first, and scrolling to it expands the node row, which writes an
    sm_state.expandState the Mu implementation never writes.
    """

    def mapOverItem(item, acc):
        for i in range(item.rowCount()):
            acc = mapOverItem(item.child(i, 0), acc)
        if itemNode(item) != "" and F(item):
            return [item] + acc
        return acc

    result = []
    if root is None:
        for i in range(model.rowCount(QtCore.QModelIndex())):
            result = mapOverItem(model.item(i, 0), result)
    else:
        result = mapOverItem(root, result)
    return result


def itemOfNode(model, node):
    items = mapItems(model, lambda i: itemNode(i) == node and not itemIsSubComponent(i))
    return items[0] if items else None


def subComponentItemsOfNode(model, node):
    def match(i):
        subType = itemSubComponentType(i)
        return (
            itemNode(i) == node
            and subType != NotASubComponent
            and subType != MediaSubComponent
            and i.index().column() == 0
        )

    return mapItems(model, match)


def assignSortOrder(root):
    if root is None:
        return
    try:
        rootNode = itemNode(root)
        index = 0
        for i in range(root.rowCount()):
            item = root.child(i, 0)
            if item is not None:
                setSortKeyInParent(itemNode(item), rootNode, index)
                index += 1
    except Exception as exc:
        print("CAUGHT %s\n" % exc)


def resizeColumns(treeView, model):
    for i in range(model.columnCount(QtCore.QModelIndex())):
        treeView.resizeColumnToContents(i)


def isImageRequestPropEqual(name, array):
    return contents_equal(commands.getStringProperty("#RVSource.request." + name), array)


def setImageRequestProp(name, array):
    pname = "#RVSource.request." + name
    if not contents_equal(commands.getStringProperty(pname), array):
        setStringProp(pname, array)
        commands.reload()


def setImageRequest(value, toggle=True):
    pname = "imageComponent"

    if toggle and isImageRequestPropEqual(pname, value):
        # Clicking the same selection a second time deselects everything, which is
        # expressed by clearing the request properties.
        setImageRequestProp(pname, [])
    else:
        setImageRequestProp(pname, value)


def subComponentPropValue(item):
    t = itemSubComponentType(item)

    if t == ViewSubComponent:
        return ["view", itemSubComponentValue(item)]

    if t == LayerSubComponent:
        parent = item.parent()
        view = (
            itemSubComponentValue(parent)
            if itemSubComponentType(parent) == ViewSubComponent
            else ""
        )
        return ["layer", view, itemSubComponentValue(item)]

    if t == ChannelSubComponent:
        parent = item.parent()
        pvalue = subComponentPropValue(parent)
        s = len(pvalue)
        value = itemSubComponentValue(item)

        assert s in (0, 2, 3)
        if s == 0:
            return ["channel", "", "", value]
        if s == 2:
            return ["channel", pvalue[1], "", value]
        return ["channel", pvalue[1], pvalue[2], value]

    return []


def setNodeRequest(node, value):
    commands.setStringProperty(node + ".request.imageComponent", value, True)


def loadUIFile(path, parent):
    """Mu's loadUIFile(), which has no Python counterpart."""
    uifile = QtCore.QFile(path)
    uifile.open(QtCore.QFile.ReadOnly)
    try:
        return QUiLoader().load(uifile, parent)
    finally:
        uifile.close()


def _model_settle_ms():
    """How long the Mu mode waits for QStandardItemModel to become consistent
    again after a drop. Windows needs longer."""
    return 200 if sys.platform.startswith("win") else 100


class ThumbnailWidget(QtWidgets.QLabel):
    """Displays a static thumbnail image, falling back to a placeholder pixmap."""

    def __init__(self, parent):
        QtWidgets.QLabel.__init__(self, parent)
        self.setScaledContents(True)

    def setFallback(self, pixmap):
        self.setPixmap(pixmap)

    def load(self, path):
        pixmap = QtGui.QPixmap.fromImage(QtGui.QImage(path, ""), Qt.AutoColor)
        if not pixmap.isNull():
            self.setPixmap(pixmap)


class FilmstripWidget(QtWidgets.QLabel):
    """A scrubbable filmstrip: shows the frame under the mouse position."""

    def __init__(self, parent):
        QtWidgets.QLabel.__init__(self, parent)
        self._strip = QtGui.QImage()
        self._frameWidth = FILMSTRIP_FRAME_WIDTH
        self._loaded = False
        self.setScaledContents(True)
        self.setMouseTracking(True)

    def showFrameAtX(self, mouseX):
        if not self._loaded or self.width() <= 0:
            return
        nativeWidth = self._strip.width()
        proportionX = float(mouseX) / float(self.width())
        frameX = (
            int(proportionX * float(nativeWidth) / float(self._frameWidth) + 0.5)
            * self._frameWidth
        )
        if frameX > nativeWidth - self._frameWidth:
            clampedX = nativeWidth - self._frameWidth
        elif frameX < 0:
            clampedX = 0
        else:
            clampedX = frameX
        frame = self._strip.copy(
            QtCore.QRect(clampedX, 0, self._frameWidth, self._strip.height())
        )
        self.setPixmap(QtGui.QPixmap.fromImage(frame, Qt.AutoColor))

    def isLoaded(self):
        return self._loaded

    def load(self, path):
        filmstripImage = QtGui.QImage(path, "")
        if not filmstripImage.isNull():
            self._strip = filmstripImage
            self._loaded = True

    def mouseMoveEvent(self, event):
        self.showFrameAtX(event.position().toPoint().x())
        QtWidgets.QLabel.mouseMoveEvent(self, event)


class SourcePreviewWidget(QtWidgets.QWidget):
    """Thumbnail by default; on hover, the filmstrip scrubbed to the cursor."""

    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self.setAttribute(Qt.WA_Hover, True)

        self._thumbnail = ThumbnailWidget(self)
        self._thumbnail.setGeometry(
            QtCore.QRect(0, 0, SOURCE_PREVIEW_WIDTH, SOURCE_PREVIEW_HEIGHT)
        )
        self._thumbnail.show()

        self._filmstrip = FilmstripWidget(self)
        self._filmstrip.setGeometry(
            QtCore.QRect(0, 0, SOURCE_PREVIEW_WIDTH, SOURCE_PREVIEW_HEIGHT)
        )
        self._filmstrip.hide()

    def setFallback(self, pixmap):
        self._thumbnail.setFallback(pixmap)

    def loadStrip(self, path):
        self._filmstrip.load(path)

    def loadThumbnail(self, path):
        self._thumbnail.load(path)

    def event(self, event):
        if event.type() == QtCore.QEvent.HoverEnter:
            if self._filmstrip.isLoaded():
                self._filmstrip.showFrameAtX(
                    self.mapFromGlobal(QtGui.QCursor.pos()).x()
                )
                self._filmstrip.show()
                self._thumbnail.hide()
            return True
        if event.type() == QtCore.QEvent.HoverLeave:
            self._filmstrip.hide()
            self._thumbnail.show()
            return True
        return QtWidgets.QWidget.event(self, event)


class NodeModel(QtGui.QStandardItemModel):
    """QStandardItemModel with modified drag and drop mime types."""

    def __init__(self, parent):
        QtGui.QStandardItemModel.__init__(self, parent)

    def mimeTypes(self):
        return list(QtGui.QStandardItemModel.mimeTypes(self)) + [
            "text/uri-list",
            "text/plain",
        ]

    def mimeData(self, indices):
        d = QtGui.QStandardItemModel.mimeData(self, indices)
        urls = []
        text = []

        #
        #  rvnode URL looks like:
        #
        #      rvnode://RVID/NODETYPE/NODENAME/PATH/TO/MEDIA
        #
        #  RVID can be nothing or an open port on a machine and possibly user like
        #  rvnode://me@foo:12332/....  right now we only support the empty RVID
        #
        try:
            for index in indices:
                n = nodeFromIndex(index, self)
                ntype = commands.nodeType(n)
                rvid = "%s@%s:%s" % (
                    commands.remoteLocalContactName(),
                    commands.myNetworkHost(),
                    commands.myNetworkPort(),
                )

                if ntype == "RVSourceGroup":
                    media = commands.getStringProperty("%s_source.media.movie" % n)
                    text.append("RVFileSource %s.media.movie = %s\n" % (n, media))
                    for m in media:
                        urls.append(
                            QtCore.QUrl("rvnode://%s/%s/%s/%s" % (rvid, ntype, n, m))
                        )
                else:
                    text.append("%s %s\n" % (ntype, n))
                    urls.append(QtCore.QUrl("rvnode://%s/%s/%s" % (rvid, ntype, n)))

            d.setText("".join(text))
            d.setUrls(urls)
        except Exception as exc:
            print("CAUGHT %s\n" % exc)

        return d


class NodeTreeView(QtWidgets.QTreeView):
    """The session tree, with drag and drop constrained.

    QStandardItemModel would otherwise unconditionally accept items from models
    that have nothing to do with the session manager, and it reports inconsistent
    state during a drag (notably while emitting itemChanged), which is why the sort
    that follows a drop runs off a timer instead of inline.
    """

    def __init__(self, parent):
        QtWidgets.QTreeView.__init__(self, parent)
        self._dropAction = Qt.IgnoreAction
        self._draggedNodePaths = []
        self._draggingNonFolders = False
        self._viewModel = None
        self._sortFolders = []
        self._foldersItem = None
        self._sortTimer = QtCore.QTimer(self)
        self._sortTimer.setSingleShot(True)
        self._sortTimer.timeout.connect(self.sortFolders)

    def sortFolderChildren(self, folder):
        if commands.nodeType(folder) == "RVFolderGroup":
            if folder not in self._sortFolders:
                self._sortFolders.append(folder)

    def selectedNodePaths(self):
        indices = self.selectionModel().selectedIndexes()
        paths = []

        for index in indices:
            if index.column() == 0:
                path = []
                while True:
                    item = self._viewModel.itemFromIndex(index)
                    path.append(itemNode(item))
                    index = index.parent()
                    if not index.isValid():
                        break
                paths.append(path)

        return paths

    def filteredDraggedPaths(self, F):
        return [path for path in self._draggedNodePaths if F(path)]

    def dragEnterEvent(self, event):
        sourceWidget = event.source()
        mimeData = event.mimeData()

        if sourceWidget is self:
            self._draggedNodePaths = self.selectedNodePaths()
            self._draggingNonFolders = False

            for path in self._draggedNodePaths:
                if (
                    commands.nodeExists(path[0])
                    and commands.nodeType(path[0]) != "RVFolderGroup"
                ):
                    self._draggingNonFolders = True

            if self._foldersItem is not None:
                self._foldersItem.setFlags(
                    Qt.ItemIsEnabled
                    if self._draggingNonFolders
                    else Qt.ItemIsDropEnabled | Qt.ItemIsEnabled
                )

            QtWidgets.QAbstractItemView.dragEnterEvent(self, event)
        elif sourceWidget is not None:
            pass  # allow it to be rejected
        else:
            print("No like source: %s\n" % str(sourceWidget))
            # don't accept it
            print("--formats--\n")
            for f in mimeData.formats():
                print("%s\n" % f)

            if mimeData.hasUrls():
                print("--urls--\n")
                for u in mimeData.urls():
                    print("%s\n" % u.toString())

            if mimeData.hasText():
                print("--text--\n")
                print("%s\n" % mimeData.text())

    def dragMoveEvent(self, event):
        index = self.indexAt(event.position().toPoint())
        item = self._viewModel.itemFromIndex(index)

        if item is None:
            event.ignore()
            return

        node = itemNode(item)

        if item.column() != 0:
            event.ignore()
            return

        if event.dropAction() == Qt.CopyAction and commands.nodeExists(node):
            outs = commands.nodeConnections(node)[1]
            ntype = commands.nodeType(node)

            for path in self._draggedNodePaths:
                if len(path) > 1 and commands.nodeExists(path[1]):
                    if ntype != "RVFolderGroup":
                        #
                        #  don't allow dropping on a non-folder sibling either,
                        #  this is basically a reorder/copy
                        #
                        for out in outs:
                            if out == path[1]:
                                event.ignore()
                                return

                    if path[1] == node:
                        event.ignore()
                        return

        QtWidgets.QTreeView.dragMoveEvent(self, event)

    def dropEvent(self, event):
        self._dropAction = event.dropAction()
        QtWidgets.QTreeView.dropEvent(self, event)

        self._draggedNodePaths = []
        self._dropAction = Qt.IgnoreAction
        self._sortTimer.start(_model_settle_ms())

    def sortFolders(self):
        for folder in self._sortFolders:
            item = itemOfNode(self._viewModel, folder)
            if item is not None:
                assignSortOrder(item)

        self._sortFolders = []


class InputsView(QtWidgets.QListView):
    """The inputs list, forcing a copy for drops coming from the session tree."""

    def __init__(self, treeView, parent, dropCleanup=None):
        self._viewTreeView = treeView
        QtWidgets.QListView.__init__(self, parent)
        self._dropTimer = QtCore.QTimer(self)
        self._dropTimer.setSingleShot(True)
        if dropCleanup is not None:
            self._dropTimer.timeout.connect(dropCleanup)

    def dragEnterEvent(self, event):
        if event.source() is self._viewTreeView:
            # force a copy from the tree view
            event.setDropAction(Qt.CopyAction)

        QtWidgets.QAbstractItemView.dragEnterEvent(self, event)

    def dropEvent(self, event):
        QtWidgets.QListView.dropEvent(self, event)
        if event.source() is self._viewTreeView:
            # update the tree if the drop came from there
            self._dropTimer.start(_model_settle_ms())


class EventFilter(QtCore.QObject):
    """Forwards events to RV's main view so shortcuts keep working in the dock."""

    def __init__(self, parent):
        QtCore.QObject.__init__(self, parent)

    def eventFilter(self, obj, event):
        view = qtutils.sessionGLView()
        return view.eventFilter(obj, event)


class SessionManagerMode(rv.rvtypes.MinorMode):
    #
    #  Some helper functions. Some of the Qt interface is a bit
    #  verbose and/or I'm too inexperienced to know how to do this in
    #  a more succinct way.
    #

    def colorAdjustedIcon(self, rpath, invertSense):
        bg = QtWidgets.QApplication.palette().color(
            QtGui.QPalette.Active, QtGui.QPalette.Window
        )
        icon0 = QtGui.QImage(rpath.replace("48x48", "out"), "")
        icon1 = QtGui.QImage(rpath, "")
        swap = invertSense != self._darkUI
        qimage = icon0 if swap else icon1

        icon = QtGui.QIcon(QtGui.QPixmap.fromImage(qimage, Qt.AutoColor))

        icon.addPixmap(
            QtGui.QPixmap.fromImage(icon1, Qt.AutoColor),
            QtGui.QIcon.Selected,
            QtGui.QIcon.Off,
        )

        return icon

    def auxFilePath(self, icon):
        return os.path.join(
            self.supportPath(sys.modules[__name__], "session_manager"), icon
        )

    def auxIcon(self, name, colorAdjust=False):
        if colorAdjust:
            return self.colorAdjustedIcon(":images/" + name, False)
        return QtGui.QIcon(":images/" + name)

    def splitterMoved(self, pos, index):
        propName = "#Session.sm_window.splitter"
        fpos = float(pos) / float(self._splitter.height())

        if not commands.propertyExists(propName):
            commands.newProperty(propName, commands.FloatType, 1)

        setFloatProp(propName, fpos)

    def selectInputsRange(self, selectionList):
        smodel = self._inputsView.selectionModel()

        for row in selectionList:
            index = self._inputsModel.index(row, 0, QtCore.QModelIndex())
            smodel.select(index, QtCore.QItemSelectionModel.Select)

    def iconForNode(self, node):
        typeName = commands.nodeType(node)
        cprop = node + ".sm_state.componentSubType"

        if commands.propertyExists(cprop):
            prop = commands.getIntProperty(cprop)

            if len(prop) > 0:
                front = prop[0]
                if front == ViewSubComponent:
                    return self._viewIcon
                if front == LayerSubComponent:
                    return self._layerIcon
                if front == ChannelSubComponent:
                    return self._channelIcon

        for name, icon in self._typeIcons:
            if name == typeName:
                return icon
        return self._unknownTypeIcon

    def viewEditModeActivated(self, event):
        event.reject()
        commands.sendInternalEvent("session-manager-load-ui", commands.viewNode())

    def enterQuittingState(self, event):
        #
        # Set quitting flag in response to imminent session deletion. Note
        # that this relies on the fact that this mode receives the
        # before-session-deletion event prior to the ModeManager mode.
        #

        self._quitting = True
        event.reject()

    def onCategoryStateChanged(self, event):
        if self._active and not commands.isEventCategoryEnabled("sessionmanager_category"):
            self.toggle()
        event.reject()

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)

        if self._dockWidget is not None:
            self._dockWidget.installEventFilter(self._eventFilter)

        try:
            s = str(commands.readSettings("SessionManager", "showOnStartup", "no"))

            if s == "last":
                commands.writeSettings("Tools", "show_session_manager", True)
        except Exception:
            commands.writeSettings("SessionManager", "showOnStartup", "no")
            commands.writeSettings("Tools", "show_session_manager", False)

        self._dockWidget.show()
        self.updateTree()
        commands.sendInternalEvent("session-manager-load-ui", commands.viewNode())

    def deactivate(self):
        rv.rvtypes.MinorMode.deactivate(self)

        if self._dockWidget is not None:
            self._dockWidget.removeEventFilter(self._eventFilter)

        try:
            s = str(commands.readSettings("SessionManager", "showOnStartup", "no"))

            if s == "last" and not self._quitting:
                commands.writeSettings("Tools", "show_session_manager", False)
        except Exception:
            commands.writeSettings("SessionManager", "showOnStartup", "no")
            commands.writeSettings("Tools", "show_session_manager", False)

        self._lazySetInputsTimer.stop()
        self._lazyUpdateTimer.stop()
        self._dockWidget.hide()

    def setNodeStatus(self, node, status):
        items = mapItems(
            self._viewModel,
            lambda i: itemNode(i) == node and not itemIsSubComponent(i),
        )

        for i in items:
            sitem = i.parent().child(i.row(), 2)

            if sitem is None:
                i.parent().setChild(i.row(), 2, QtGui.QStandardItem(status))
            else:
                sitem.setText(status)

    def viewByIndex(self, index, model):
        item = model.itemFromIndex(index)
        node = itemNode(item)
        subType = itemSubComponentType(item)

        self._disableUpdates = True

        try:
            viewChange = False
            if commands.viewNode() != node:
                commands.setViewNode(node)
                viewChange = True

            if subType != NotASubComponent:
                setImageRequest(subComponentPropValue(item), not viewChange)
        except Exception:
            pass

        self._disableUpdates = False
        self.updateInputs(commands.viewNode())

    def itemPressed(self, index, model):
        item0 = model.itemFromIndex(index)
        sindex = index.sibling(index.row(), 0)
        item = model.itemFromIndex(sindex)
        subType = itemSubComponentType(item)

        if (
            item0.column() == 1
            and subType != NotASubComponent
            and subType != MediaSubComponent
        ):
            self.viewByIndex(sindex, model)

    def viewItemChanged(self, item):
        node = itemNode(item)
        subType = itemSubComponentType(item)
        parentItem = item.parent()
        parent = None if parentItem is None else itemNode(parentItem)
        nodePaths = self._viewTreeView.filteredDraggedPaths(lambda p: p[0] == node)

        if self._viewTreeView._dropAction == Qt.CopyAction:
            #
            #  You can get called *twice* here if you have multiple
            #  columns, but it will be giving you the 0th column
            #  only! so Just don't allow input copies from dnd
            #

            if not hasInput(parent, node):
                addInput(parent, node)
                item.setData(parent, Qt.UserRole + 1)
                if (
                    parent is not None
                    and commands.nodeExists(parent)
                    and commands.nodeType(parent) == "RVFolderGroup"
                ):
                    self._viewTreeView.sortFolderChildren(parent)
        elif self._viewTreeView._dropAction == Qt.MoveAction and nodePaths:
            parentExists = parent is not None and commands.nodeExists(parent)

            if parentExists:
                if not hasInput(parent, node):
                    addInput(parent, node)

            item.setData(parent if parentExists else "", Qt.UserRole + 1)

            for path in nodePaths:
                if len(path) > 1:
                    n = path[0]
                    p = path[1]

                    if commands.nodeExists(p) and (not parentExists or p != parent):
                        removeInput(p, n)

            if parentExists and commands.nodeType(parent) == "RVFolderGroup":
                self._viewTreeView.sortFolderChildren(parent)
        elif node != "" and subType == NotASubComponent:
            self._disableUpdates = True

            try:
                extra_commands.setUIName(node, item.text())
            except Exception:
                print("failed to set name on %s to %s\n" % (node, item.text()))  # bad

            self._disableUpdates = False

    def viewSelectionChanged(self, selected, deselected):
        indices = selected.indexes()

        if indices:
            index = indices[0]

            #
            #  Only consider top-level items
            #

            if index.parent().parent().row() == -1:
                rows = self._viewTreeView.selectionModel().selectedRows(0)
                if rows:
                    self.viewByIndex(rows[0], self._viewModel)

    def updateInputs(self, node):
        if self._disableUpdates or self._progressiveLoadingInProgress:
            return

        self._inputOrderLock = True

        topNode = None
        topIndex = self._inputsView.indexAt(QtCore.QPoint(0, 0))
        if topIndex.isValid():
            topNode = nodeFromIndex(topIndex, self._inputsModel)

        self._inputsModel.clear()
        connections = nodeInputs(node)

        for innode in connections:
            isSource = commands.nodeType(innode) == "RVSourceGroup"
            item = QtGui.QStandardItem(
                self.iconForNode(innode), extra_commands.uiName(innode)
            )

            item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsEnabled)
            item.setData(innode, Qt.UserRole + 2)
            item.setEditable(False)

            if isSource and self._previewsEnabled:
                item.setText("")
                item.setSizeHint(QtCore.QSize(-1, SOURCE_ROW_HEIGHT))

            self._inputsModel.appendRow(item)

            if isSource and self._previewsEnabled:
                self._inputsView.setIndexWidget(
                    self._inputsModel.indexFromItem(item), self.makeSourceRowWidget(innode)
                )

        self._inputOrderLock = False

        if topNode is not None:
            topItem = itemOfNode(self._inputsModel, topNode)
            if topItem is not None:
                self._inputsView.scrollTo(
                    self._inputsModel.indexFromItem(topItem),
                    QtWidgets.QAbstractItemView.PositionAtTop,
                )

    def selectViewableNode(self):
        node = commands.viewNode()
        if node is None:
            return

        cols = self._viewModel.columnCount(QtCore.QModelIndex())
        smodel = self._viewTreeView.selectionModel()
        items = mapItems(self._viewModel, lambda i: itemNode(i) == node)

        smodel.clear()

        for item in items:
            index = self._viewModel.indexFromItem(item)
            selection = QtCore.QItemSelection(
                index, index.sibling(index.row(), cols - 1)
            )

            smodel.select(selection, QtCore.QItemSelectionModel.SelectCurrent)
            self.updateInputs(node)
            self._viewTreeView.scrollTo(index, QtWidgets.QAbstractItemView.EnsureVisible)
            break

    def selectCurrentViewSlot(self, checked):
        self.selectViewableNode()

    def updateNavUI(self):
        n = commands.viewNode()

        if n is None:
            return

        self._viewLabel.setText(extra_commands.uiName(n))
        self._prevViewButton.setEnabled(commands.previousViewNode() is not None)
        self._nextViewButton.setEnabled(commands.nextViewNode() is not None)

    def afterGraphViewChange(self, event):
        event.reject()

        n = commands.viewNode()

        if n is None:
            return
        t = commands.nodeType(n)

        self.selectViewableNode()
        self.setNodeStatus(commands.viewNode(), "\u2714")

        self.updateNavUI()
        self.restoreTabState()

        #
        #  Disable inputs for the types we know don't allow any
        #

        self._inputsView.setEnabled(
            t != "RVSource"
            and t != "RVFileSource"
            and t != "RVImageSource"
            and t != "RVSourceGroup"
        )

        commands.sendInternalEvent("session-manager-load-ui", commands.viewNode())

    def addEditor(self, name, widget):
        item = QtWidgets.QTreeWidgetItem([name], QtWidgets.QTreeWidgetItem.Type)
        child = QtWidgets.QTreeWidgetItem([""], QtWidgets.QTreeWidgetItem.Type)

        widget.setAutoFillBackground(True)
        item.setIcon(0, QtGui.QIcon(":/images/radio_button_on_default.png"))
        item.setFlags(Qt.ItemIsEnabled)

        item.addChild(child)
        self._uiTreeWidget.addTopLevelItem(item)
        self._uiTreeWidget.setItemWidget(child, 0, widget)
        widget.show()
        item.setExpanded(True)

        self._editors.append(item)

    def useEditor(self, name):
        for e in self._editors:
            if name == e.text(0):
                e.setHidden(False)

    def reloadEditorTab(self):
        for e in self._editors:
            e.setHidden(True)
        commands.sendInternalEvent("session-manager-load-ui", commands.viewNode())

    def beforeGraphViewChange(self, event):
        for e in self._editors:
            e.setHidden(True)
        event.reject()
        self.saveTabState()
        self.setNodeStatus(commands.viewNode(), "")

    def nodeInputsChanged(self, event):
        if commands.viewNode() is None:
            return
        node = event.contents()
        if node == commands.viewNode():
            self.updateInputs(node)

        if (
            commands.nodeType(node) == "RVFolderGroup"
            and self._viewTreeView._dropAction == Qt.IgnoreAction
        ):
            self._lazyUpdateTimer.start(0)

        event.reject()

    def propertyChanged(self, event):
        prop = event.contents()
        parts = prop.split(".")
        node = parts[0]
        comp = parts[1]
        name = parts[2]

        #
        #  If a UI name changes we need to update the tree
        #  Or if someone else resorts the nodes.
        #

        if comp == "ui" and name == "name":
            self._lazyUpdateTimer.start(0)
            self.updateNavUI()
        elif comp == "sm_state" and (name == "sortKey" or name == "sortKeyParent"):
            self._lazyUpdateTimer.start(0)
        elif comp == "request" and name == "imageComponent":
            topNode = commands.nodeGroup(node)
            pval = commands.getStringProperty(prop)

            for item in subComponentItemsOfNode(self._viewModel, topNode):
                selected = contents_equal(pval, subComponentPropValue(item))
                checkitem = item.parent().child(item.row(), 1)

                checkitem.setIcon(
                    QtGui.QIcon(":/images/radio_button_blue_on.png")
                    if selected
                    else QtGui.QIcon(":/images/radio_button_dark.png")
                )

        event.reject()

    def setItemExpandedState(self, index, value):
        item = self._viewModel.itemFromIndex(index)
        node = itemNode(item)
        subComp = itemIsSubComponent(item)

        if subComp:
            setSubComponentExpanded(node, item, value == 1)
        else:
            if commands.nodeExists(node):
                parent = itemNode(item.parent())
                setExpandedInParent(node, parent, value == 1)
            else:
                propName = "#Session.sm_view.%s" % item.text()
                setIntProp(propName, value)

        resizeColumns(self._viewTreeView, self._viewModel)

    def viewContextMenuSlot(self, pos):
        if self._viewContextMenu is None:
            self._viewContextMenu = QtWidgets.QMenu(self._viewTreeView)

            folderMenu = self._viewContextMenu.addMenu(self._folderMenu)
            folderMenu.setIcon(self.auxIcon("foldr_48x48.png", True))

            createMenu = self._viewContextMenu.addMenu(self._createMenu)
            createMenu.setIcon(self.auxIcon("add_48x48.png", True))

            for a in self._viewContextMenuActions:
                self._viewContextMenu.addAction(a)

        self._viewContextMenu.exec(self._viewTreeView.mapToGlobal(pos))

    def newNodeStatusColumns(self, node):
        items = []

        for _ in range(2):
            item = QtGui.QStandardItem("")
            item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable)
            items.append(item)

        return items

    def newNodeSubComponent(
        self, subComponent, parentItem, media, fullName, node, parent, selected
    ):
        name = os.path.basename(fullName) if subComponent == MediaSubComponent else fullName
        item = QtGui.QStandardItem("default" if name == "" else name)

        if name == "":
            font = item.font()
            font.setItalic(True)
            item.setFont(font)

        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsEnabled)
        item.setData(parent, Qt.UserRole + 1)
        item.setData(node, Qt.UserRole + 2)
        item.setData(subComponent, Qt.UserRole + 4)
        item.setData(fullName, Qt.UserRole + 5)
        item.setData(media, Qt.UserRole + 7)
        item.setEditable(True)

        if subComponent == ViewSubComponent:
            item.setIcon(self._viewIcon)
        elif subComponent == LayerSubComponent:
            item.setIcon(self._layerIcon)
        elif subComponent == ChannelSubComponent:
            item.setIcon(self._channelIcon)

        sitems = self.newNodeStatusColumns(node)
        selitem = sitems[0]

        if subComponent != MediaSubComponent:
            selitem.setIcon(
                QtGui.QIcon(
                    ":/images/radio_button_blue_on.png"
                    if selected
                    else ":/images/radio_button_dark.png"
                )
            )

        addRow(parentItem, [item] + sitems)

        item.setData(hashedSubComponent(item), Qt.UserRole + 6)

        if subComponent != ChannelSubComponent and isSubComponentExpanded(node, item):
            self._viewTreeView.setExpanded(self._viewModel.indexFromItem(item), True)

        return item

    def makeSourceRowWidget(self, node):
        sourceNode = None
        try:
            sourceNode = sourceNodeOfGroup(node)
        except Exception as exc:
            print(
                "WARNING: Could not get source node for %s - %s\n"
                % (extra_commands.uiName(node), exc)
            )

        widget = QtWidgets.QWidget(None)
        layout = QtWidgets.QHBoxLayout(widget)
        widget.setObjectName("sourceRowWidget")
        layout.setContentsMargins(SOURCE_ROW_MARGIN, 0, SOURCE_ROW_MARGIN, 0)
        layout.setSpacing(SOURCE_ROW_SPACING)

        preview = SourcePreviewWidget(widget)
        preview.setFixedSize(QtCore.QSize(SOURCE_PREVIEW_WIDTH, SOURCE_PREVIEW_HEIGHT))
        preview.setFallback(
            self._fallbackSourceIcon.pixmap(
                QtCore.QSize(SOURCE_PREVIEW_WIDTH, SOURCE_PREVIEW_HEIGHT)
            )
        )

        meta = ""

        if sourceNode is not None:
            # Fetch filmstrip/thumbnail paths. The local plugin has a lower priority of
            # 10 for ordering. This means any custom plugin of higher priority will be used first.
            # This allows users to override the local plugin with a custom plugin by making sure
            # the ordering is less than 10 and using event.accept() to prevent the local plugin from running.
            thumbnailPath = commands.sendInternalEvent(
                "session-manager-get-thumbnail-path", sourceNode
            )
            if thumbnailPath != "" and os.path.exists(thumbnailPath):
                preview.loadThumbnail(thumbnailPath)

                filmstripPath = commands.sendInternalEvent(
                    "session-manager-get-filmstrip-path", sourceNode
                )
                if filmstripPath != "" and os.path.exists(filmstripPath):
                    preview.loadStrip(filmstripPath)

            mediaPropertyPath = sourceNode + ".media.movie"
            if commands.propertyExists(mediaPropertyPath):
                movieProperty = commands.getStringProperty(mediaPropertyPath)
                if len(movieProperty) > 0:
                    parts = os.path.basename(movieProperty[0]).split(".")
                    if len(parts) > 1:
                        meta = parts[-1]

        layout.addWidget(preview)

        textWidget = QtWidgets.QWidget(widget)
        textLayout = QtWidgets.QVBoxLayout(textWidget)
        textWidget.setObjectName("sourceTextWidget")
        textLayout.setSpacing(SOURCE_TEXT_SPACING)

        nameLabel = QtWidgets.QLabel(extra_commands.uiName(node), textWidget)
        nameLabel.setObjectName("sourceNameLabel")
        textLayout.addWidget(nameLabel)

        metaLabel = QtWidgets.QLabel("\u2014" if meta == "" else meta, textWidget)
        metaLabel.setObjectName("sourceMetaLabel")
        textLayout.addWidget(metaLabel)
        textLayout.addStretch(1)

        layout.addWidget(textWidget, 1)

        return widget

    def newNodeRow(self, parentItem, node, parent, recursive=False):
        ntype = commands.nodeType(node)
        item = QtGui.QStandardItem(extra_commands.uiName(node))
        folder = ntype == "RVFolderGroup"
        source = ntype == "RVSourceGroup"
        sortKey = sortKeyInParent(node, parent)
        toolTip = toolTipFromProp(node)
        icon = self.iconForNode(node)

        item.setFlags(
            Qt.ItemIsSelectable
            | Qt.ItemIsDragEnabled
            | Qt.ItemIsEnabled
            | (Qt.ItemIsDropEnabled if folder else Qt.NoItemFlags)
        )
        item.setData(parent, Qt.UserRole + 1)
        item.setData(node, Qt.UserRole + 2)
        item.setData(sortKey, Qt.UserRole + 3)
        item.setData(NotASubComponent, Qt.UserRole + 4)
        item.setEditable(True)
        item.setIcon(icon)
        item.setRowCount(0)

        statusItems = self.newNodeStatusColumns(node)
        if node == commands.viewNode():
            statusItems[1].setText("\u2714")
        addRow(parentItem, [item] + statusItems)

        if source and self._previewsEnabled:
            item.setText("")
            item.setSizeHint(QtCore.QSize(-1, SOURCE_ROW_HEIGHT))
            self._viewTreeView.setIndexWidget(
                self._viewModel.indexFromItem(item), self.makeSourceRowWidget(node)
            )

        #
        #  Tabs in tooltips make win32 Qt crash.
        #

        item.setToolTip("" if toolTip is None else toolTip.replace("\t", " "))

        if folder and recursive:
            for n in commands.nodeConnections(node)[0]:
                self.newNodeRow(item, n, node, recursive)

        if isExpandedInParent(node, parent):
            self._viewTreeView.setExpanded(self._viewModel.indexFromItem(item), True)

        if source:
            if not commands.propertyExists(node + ".sm_state.componentHash"):
                #
                #  Don't do this for nodes representing parts of existing
                #  nodes. Those will have the property
                #  node.sm_state.componentHash
                #

                sourceNode = sourceNodeOfGroup(node)
                self._srcNodeKeys.append(sourceNode)
                self._grpNodeValues.append(node)
                pval = commands.getStringProperty(sourceNode + ".request.imageComponent")
                hasPval = len(pval) > 1
                iname = pval[-1] if hasPval else None
                itype = (
                    itemSubComponentTypeForName(pval[0]) if hasPval else NotASubComponent
                )

                try:
                    for info in commands.sourceMediaInfoList(sourceNode):
                        fileItem = self.newNodeSubComponent(
                            MediaSubComponent,
                            item,
                            info["file"],
                            info["file"],
                            node,
                            parent,
                            False,
                        )

                        font = fileItem.font()
                        font.setBold(True)
                        fileItem.setFont(font)
                        topItem = fileItem

                        for v in info["viewInfos"]:
                            if len(info["viewInfos"]) > 1 and v["name"] != "":
                                selected = itype == ViewSubComponent and iname == v["name"]

                                topItem = self.newNodeSubComponent(
                                    ViewSubComponent,
                                    fileItem,
                                    info["file"],
                                    v["name"],
                                    node,
                                    parent,
                                    selected,
                                )
                            else:
                                topItem = fileItem

                            nlayers = len(v["layers"])

                            for l in v["layers"]:
                                unnamed = l["name"] == ""
                                selected = itype == LayerSubComponent and iname == l["name"]

                                if nlayers > 1 and unnamed:
                                    layerItem = self.newNodeSubComponent(
                                        LayerSubComponent,
                                        topItem,
                                        info["file"],
                                        "",
                                        node,
                                        parent,
                                        selected,
                                    )
                                elif not unnamed:
                                    layerItem = self.newNodeSubComponent(
                                        LayerSubComponent,
                                        topItem,
                                        info["file"],
                                        l["name"],
                                        node,
                                        parent,
                                        selected,
                                    )
                                else:
                                    layerItem = topItem

                                for c in l["channels"]:
                                    selected = (
                                        itype == ChannelSubComponent and iname == c["name"]
                                    )

                                    self.newNodeSubComponent(
                                        ChannelSubComponent,
                                        layerItem,
                                        info["file"],
                                        c["name"],
                                        node,
                                        parent,
                                        selected,
                                    )

                            if v["layers"] and v["noLayerChannels"]:
                                selected = itype == LayerSubComponent and iname == ""

                                topItem = self.newNodeSubComponent(
                                    LayerSubComponent,
                                    topItem,
                                    info["file"],
                                    "",
                                    node,
                                    parent,
                                    selected,
                                )

                            for c in v["noLayerChannels"]:
                                selected = itype == ChannelSubComponent and iname == c["name"]

                                self.newNodeSubComponent(
                                    ChannelSubComponent,
                                    topItem,
                                    info["file"],
                                    c["name"],
                                    node,
                                    parent,
                                    selected,
                                )
                except Exception:
                    pass  # ignore
            else:
                try:
                    pname = node + ".sm_state.componentOfNode"
                    cnode = commands.getStringProperty(pname)[0]
                    emptyItem = QtGui.QStandardItem(
                        "(subcompoment of %s)" % extra_commands.uiName(cnode)
                    )
                    font = emptyItem.font()

                    font.setItalic(True)
                    emptyItem.setFont(font)
                    addRow(item, [emptyItem, QtGui.QStandardItem("")])
                except Exception:
                    pass

    def updateTree(self):
        if self._disableUpdates:
            return
        self._srcNodeKeys = []
        self._grpNodeValues = []
        self._viewModel.clear()
        self._viewModel.setHorizontalHeaderLabels(["Name", "*", "*"])
        self._viewTreeView.header().setMinimumSectionSize(-1)
        if commands.viewNode() is None:
            return

        try:
            self._viewModel.setSortRole(Qt.UserRole + 3)  # the sort key is an int

            viewNodes = commands.viewNodes()
            foldersItem = QtGui.QStandardItem("FOLDERS")
            sourcesItem = QtGui.QStandardItem("SOURCES")
            sequencesItem = QtGui.QStandardItem("SEQUENCES")
            stackItem = QtGui.QStandardItem("STACKS")
            layoutItem = QtGui.QStandardItem("LAYOUTS")
            otherItem = QtGui.QStandardItem("OTHER")
            categoryItems = [
                foldersItem,
                sourcesItem,
                sequencesItem,
                stackItem,
                layoutItem,
                otherItem,
            ]
            fgMac = QtGui.QBrush(QtGui.QColor(80, 80, 80, 255), Qt.SolidPattern)
            fgOther = QtGui.QBrush(QtGui.QColor(125, 125, 125, 255), Qt.SolidPattern)
            foreground = fgOther if self._darkUI else fgMac

            for item in categoryItems:
                item.setFlags(Qt.ItemIsEnabled)
                item.setForeground(foreground)
                item.setSizeHint(QtCore.QSize(-1, 25))
                item.setData("", Qt.UserRole + 1)
                item.setData("", Qt.UserRole + 2)
                item.setData(MU_INT_MAX, Qt.UserRole + 3)

            foldersItem.setFlags(Qt.ItemIsEnabled | Qt.ItemIsDropEnabled)
            self._viewTreeView._foldersItem = foldersItem

            categoryOfType = {
                "RVFileSource": sourcesItem,
                "RVImageSource": sourcesItem,
                "RVSourceGroup": sourcesItem,
                "RVSequenceGroup": sequencesItem,
                "RVStackGroup": stackItem,
                "RVLayoutGroup": layoutItem,
                "RVFolderGroup": foldersItem,
            }

            for node in viewNodes:
                ntype = commands.nodeType(node)
                outs = commands.nodeConnections(node)[1]

                folderParent = False
                for o in outs:
                    if commands.nodeType(o) == "RVFolderGroup":
                        folderParent = True

                if not folderParent:
                    self.newNodeRow(categoryOfType.get(ntype, otherItem), node, "", True)

            for item in categoryItems:
                if item.rowCount() != 0:
                    text = item.text()
                    propName = "#Session.sm_view.%s" % text

                    if not commands.propertyExists(propName):
                        commands.newProperty(propName, commands.IntType, 1)
                        commands.setIntProperty(propName, [1], True)

                    dummy1 = QtGui.QStandardItem("")
                    dummy2 = QtGui.QStandardItem("")
                    dummy1.setFlags(Qt.ItemIsEnabled)
                    dummy2.setFlags(Qt.ItemIsEnabled)
                    self._viewModel.appendRow([item, dummy1, dummy2])
                    self._viewTreeView.setExpanded(
                        self._viewModel.indexFromItem(item),
                        commands.getIntProperty(propName)[0] == 1,
                    )

            self._viewModel.sort(0, Qt.AscendingOrder)
            self._viewModel.invisibleRootItem().setFlags(Qt.ItemIsEnabled)
            self.selectViewableNode()

            resizeColumns(self._viewTreeView, self._viewModel)
        except Exception as exc:
            print("%s\n" % exc)

    def updateTreeEvent(self, event):
        event.reject()
        if self._progressiveLoadingInProgress:
            return
        self.updateTree()

    def updateNodePreviewEvent(self, event):
        event.reject()
        if not self._previewsEnabled:
            return
        sourceNode = event.contents()

        node = None
        for i in range(len(self._srcNodeKeys)):
            if self._srcNodeKeys[i] == sourceNode:
                node = self._grpNodeValues[i]
                break
        if node is None:
            return

        item = itemOfNode(self._viewModel, node)
        if item is not None:
            self._viewTreeView.setIndexWidget(
                self._viewModel.indexFromItem(item), self.makeSourceRowWidget(node)
            )

        inputItem = itemOfNode(self._inputsModel, node)
        if inputItem is not None:
            self._inputsView.setIndexWidget(
                self._inputsModel.indexFromItem(inputItem), self.makeSourceRowWidget(node)
            )

    def beforeProgressiveLoading(self, event):
        event.reject()
        self._progressiveLoadingInProgress = True

    def afterProgressiveLoading(self, event):
        event.reject()
        self._progressiveLoadingInProgress = False
        self.updateTree()
        self.updateInputs(commands.viewNode())

    def newColorSlot(self, color):
        css = "QPushButton{background-color:rgb(%d,%d,%d);}" % (
            color.red(),
            color.green(),
            color.blue(),
        )
        self._cidColorButton.setStyleSheet(css)
        self._cidColor = color

    def chooseColorSlot(self, checked):
        self._colorDialog.open()
        self._colorDialog.setCurrentColor(self._cidColor)

    def renameByType(self, node, inputs):
        n = len(inputs)
        basename = commands.nodeType(node)

        if re.match("^RV", basename):
            basename = basename[2:]
        if re.search("Group$", basename):
            basename = basename[:-5]

        name = ""

        if n == 0:
            name = "Empty %s" % basename
        elif n < 3:
            name = "%s of " % basename

            for i in range(n):
                if i > 0 and n > 2:
                    name += ","
                if i > 0:
                    name += " "
                if i == n - 1 and n > 1:
                    name += "and "
                name += extra_commands.uiName(inputs[i])
        else:
            name = "%s of %d views " % (basename, n)

        extra_commands.setUIName(node, name)

    def componentAndFolderNodeFromHash(self, hash, node):
        folder = None
        cnode = None

        for n in commands.nodes():
            if commands.nodeType(n) == "RVSourceGroup" and cnode is None:
                propName = n + ".sm_state.componentHash"

                if commands.propertyExists(propName):
                    try:
                        p = commands.getStringProperty(propName)
                        pn = commands.getStringProperty(n + ".sm_state.componentOfNode")

                        if p and p[0] == hash and pn and pn[0] == node:
                            # cnode is still unset here: the Mu original returns the
                            # match without recording it, and callers depend on the
                            # resulting None to build a fresh component node.
                            return (cnode, folder)
                    except Exception:
                        pass
            elif commands.nodeType(n) == "RVFolderGroup":
                pname = n + ".sm_state.componentFolderOfNode"
                if commands.propertyExists(pname):
                    p = commands.getStringProperty(pname)
                    if p and p[0] == node:
                        folder = n

        return (cnode, folder)

    def newSubComponentNode(
        self, hash, subType, filename, fullName, compPropValue, node, folder
    ):
        snode = commands.addSourceVerbose([filename])
        nodeName = extra_commands.uiName(node)
        groupNode = commands.nodeGroup(snode)
        dname = "default" if fullName == "" else fullName

        if folder is None:
            folder = commands.newNode("RVFolderGroup", "%s_components" % node)
            extra_commands.setUIName(folder, "Components of %s" % extra_commands.uiName(node))
            setStringProp(folder + ".sm_state.componentFolderOfNode", node)
            setExpandedInParent(folder, "", False)

        inputs = list(nodeInputs(folder))
        inputs.append(groupNode)
        commands.setNodeInputs(folder, inputs)

        setStringProp(groupNode + ".sm_state.componentOfNode", node)
        setStringProp(groupNode + ".sm_state.componentHash", hash)
        setIntProp(groupNode + ".sm_state.componentSubType", subType)

        if subType == MediaSubComponent:
            extra_commands.setUIName(groupNode, nodeName + " (Media %s)" % dname)
        elif subType == ViewSubComponent:
            extra_commands.setUIName(groupNode, nodeName + " (View %s)" % dname)
            setNodeRequest(snode, compPropValue)
        elif subType == LayerSubComponent:
            extra_commands.setUIName(groupNode, nodeName + " (Layer %s)" % dname)
            setNodeRequest(snode, compPropValue)
        elif subType == ChannelSubComponent:
            extra_commands.setUIName(groupNode, nodeName + " (Channel %s)" % dname)
            setNodeRequest(snode, compPropValue)

        extra_commands.displayFeedback(
            "NOTE: Created %s" % extra_commands.uiName(groupNode), 5
        )
        return groupNode

    def sourceFromSubComponent(self, item, node):
        hash = hashedSubComponent(item)
        cnode, folder = self.componentAndFolderNodeFromHash(hash, node)

        if cnode is not None:
            return cnode

        mediaItem = None
        viewItem = None
        layerItem = None

        i = item
        while i is not None and itemSubComponentType(i) != NotASubComponent:
            t = itemSubComponentType(i)
            if t == MediaSubComponent:
                mediaItem = i
                break
            if t == LayerSubComponent:
                layerItem = i
            elif t == ViewSubComponent:
                viewItem = i
            i = i.parent()

        subType = itemSubComponentType(item)
        filename = itemSubComponentValue(mediaItem)
        fullName = itemSubComponentValue(item)

        return self.newSubComponentNode(
            hash,
            subType,
            filename,
            fullName,
            subComponentPropValue(item),
            node,
            folder,
        )

    def selectedConvertedSubComponents(self):
        indices = self._viewTreeView.selectionModel().selectedIndexes()
        nodes = []

        for index in indices:
            if index.column() == 0:
                item = self._viewModel.itemFromIndex(index)
                n = itemNode(item)

                if commands.nodeExists(n):
                    if itemIsSubComponent(item):
                        self._disableUpdates = True
                        snode = self.sourceFromSubComponent(item, n)
                        self._disableUpdates = False
                        nodes.append(snode)
                    else:
                        nodes.append(n)

        return nodes

    def selectedNodes(self):
        indices = self._viewTreeView.selectionModel().selectedIndexes()
        nodes = []

        for index in indices:
            if index.column() == 0:
                n = itemNode(self._viewModel.itemFromIndex(index))
                if commands.nodeExists(n):
                    nodes.append(n)

        return nodes

    def selectedNodesEvent(self, event):
        """Answer "session-manager-selected-nodes" with one node name per line.

        Mu packages used to reach the selection by importing this module and calling
        theMode().selectedNodes(). A Mu `require` cannot resolve a Python module, so
        the cross-package API is exposed as an internal event instead, which works
        the same from either language and does not tie the caller to the
        implementation the mode happens to be written in. Newline-separated because
        an event's return content is a single string; node names cannot contain
        newlines.
        """
        event.setReturnContent("\n".join(self.selectedNodes()))

    def selectedItems(self):
        indices = self._viewTreeView.selectionModel().selectedIndexes()
        items = []

        for index in indices:
            if index.column() == 0:
                items.append(self._viewModel.itemFromIndex(index))

        return items

    def addNodeOfType(self, typename):
        nodes = self.selectedConvertedSubComponents()
        n = commands.newNode(typename, "")

        if n is None or not setInputs(n, nodes):
            if n is not None:
                commands.deleteNode(n)
        else:
            self.renameByType(n, nodes)
            commands.setViewNode(n)

        return n

    def addNodeByTypeName(self):
        if self._newNodeDialog is None:
            m = qtutils.sessionWindow()

            self._newNodeDialog = loadUIFile(self.auxFilePath("new_node.ui"), m)
            self._nodeTypeCombo = self._newNodeDialog.findChild(
                QtWidgets.QComboBox, "comboBox"
            )
            self._nodeTypeCombo.addItems(commands.nodeTypes(True))
            icon = self.auxIcon("new_48x48.png", True)
            label = self._newNodeDialog.findChild(QtWidgets.QLabel, "pictureLabel")
            label.setPixmap(
                icon.pixmap(QtCore.QSize(48, 48), QtGui.QIcon.Normal, QtGui.QIcon.Off)
            )

            def makeNewNodeOfType():
                self.addNodeOfType(self._nodeTypeCombo.currentText())

            self._newNodeDialog.accepted.connect(makeNewNodeOfType)

        self._newNodeDialog.show()

    def addMovieProc(self, fmtspec):
        if self._createImageDialog is None:
            m = qtutils.sessionWindow()

            self._createImageDialog = loadUIFile(
                self.auxFilePath("create_image_dialog.ui"), m
            )
            self._cidWidth = self._createImageDialog.findChild(
                QtWidgets.QLineEdit, "widthEdit"
            )
            self._cidHeight = self._createImageDialog.findChild(
                QtWidgets.QLineEdit, "heightEdit"
            )
            self._cidFPS = self._createImageDialog.findChild(
                QtWidgets.QLineEdit, "fpsEdit"
            )
            self._cidLength = self._createImageDialog.findChild(
                QtWidgets.QLineEdit, "lengthEdit"
            )
            self._cidPic = self._createImageDialog.findChild(
                QtWidgets.QLabel, "pictureLabel"
            )
            self._cidGroupBox = self._createImageDialog.findChild(
                QtWidgets.QGroupBox, "groupBox"
            )
            self._cidColorButton = self._createImageDialog.findChild(
                QtWidgets.QPushButton, "colorButton"
            )
            self._cidColorLabel = self._createImageDialog.findChild(
                QtWidgets.QLabel, "colorLabel"
            )

            f1 = float(commands.readSettings("General", "fps", 24.0))

            self._cidFPS.setText("%g" % f1)

            def makeImage():
                mp = self._cidFMTSpec % (
                    "width=%s,height=%s,fps=%s,start=1,end=%s,red=%g,green=%g,blue=%g"
                    % (
                        self._cidWidth.text(),
                        self._cidHeight.text(),
                        self._cidFPS.text(),
                        self._cidLength.text(),
                        self._cidColor.redF(),
                        self._cidColor.greenF(),
                        self._cidColor.blueF(),
                    )
                )
                s = commands.addSourceVerbose([mp])

                extra_commands.setUIName(commands.nodeGroup(s), self._cidName)

            self._createImageDialog.accepted.connect(makeImage)
            self._cidColorButton.clicked.connect(self.chooseColorSlot)

        icon = QtGui.QIcon()
        ptype = fmtspec.split(",")[0]

        self._cidColorButton.setVisible(True)
        self._cidColorLabel.setVisible(True)
        self._cidColorButton.setEnabled(False)
        self._cidColorLabel.setEnabled(False)

        if ptype == "srgbcolorchart":
            self._cidName = "SRGBMacbethColorChart"
            icon = self.auxIcon("colorchart_48x48.png", True)
            self._cidColorButton.setStyleSheet(
                "QPushButton { background-color: rgb(128,128,128); }"
            )
            self._cidColorButton.setVisible(False)
            self._cidColorLabel.setVisible(False)
            self._cidColor = QtGui.QColor(0, 0, 0, 255)
        elif ptype == "acescolorchart":
            self._cidName = "ACESMacbethColorChart"
            icon = self.auxIcon("colorchart_48x48.png", True)
            self._cidColorButton.setStyleSheet(
                "QPushButton { background-color: rgb(128,128,128); }"
            )
            self._cidColorButton.setVisible(False)
            self._cidColorLabel.setVisible(False)
            self._cidColor = QtGui.QColor(0, 0, 0, 255)
        elif ptype == "smptebars":
            self._cidName = "SMTPEColorBars"
            icon = self.auxIcon("ntscbars_48x48.png", True)
            self._cidColorButton.setStyleSheet(
                "QPushButton { background-color: rgb(128,128,128); }"
            )
            self._cidColorButton.setVisible(False)
            self._cidColorLabel.setVisible(False)
            self._cidColor = QtGui.QColor(0, 0, 0, 255)
        elif ptype == "blank":
            self._cidName = "Blank"
            icon = self.auxIcon("video_48x48.png", True)
            self._cidColorButton.setStyleSheet(
                "QPushButton { background-color: rgb(128,128,128); }"
            )
            self._cidColorButton.setVisible(False)
            self._cidColorLabel.setVisible(False)
            self._cidColor = QtGui.QColor(0, 0, 0, 255)
            self._cidWidth.setVisible(False)
            self._cidHeight.setVisible(False)
        elif ptype == "black":
            self._cidName = "Black"
            icon = self.auxIcon("video_48x48.png", True)
            self._cidColorButton.setStyleSheet(
                "QPushButton { background-color: rgb(0,0,0); }"
            )
            self._cidColor = QtGui.QColor(0, 0, 0, 255)
        elif ptype == "solid":
            self._cidName = "SolidColor"
            icon = self.auxIcon("video_48x48.png", True)
            self._cidColorButton.setStyleSheet(
                "QPushButton { background-color: rgb(128,128,128); }"
            )
            self._cidColorButton.setEnabled(True)
            self._cidColorLabel.setEnabled(True)
            self._cidColor = QtGui.QColor(128, 128, 128, 255)

        self._cidPic.setPixmap(
            icon.pixmap(QtCore.QSize(48, 48), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        )
        self._cidGroupBox.setTitle(self._cidName)
        self._cidFMTSpec = fmtspec
        self._createImageDialog.show()

    def addThingSlot(self, checked, thingstring):
        if re.match(r".+\.movieproc$", thingstring):
            self.addMovieProc(thingstring)
        elif thingstring == "":
            self.addNodeByTypeName()
        else:
            self.addNodeOfType(thingstring)

    def newFolderSlot(self, checked, which):
        paths = self._viewTreeView.selectedNodePaths()
        folder = commands.newNode("RVFolderGroup", "Folder")

        nodes = [path[0] for path in paths]

        if paths:
            first = paths[0]

            if which != 1 and nodes:
                if not setInputs(folder, nodes):
                    if folder is not None:
                        commands.deleteNode(folder)
                        return

            self._disableUpdates = True

            if which == 2:
                for path in paths:
                    if len(path) > 1 and commands.nodeExists(path[1]):
                        removeInput(path[1], path[0])

            if commands.nodeExists(first[1]):
                addInput(first[1], folder)

                setSortKeyInParent(folder, first[1], sortKeyInParent(first[0], first[1]))

            self._disableUpdates = False

        self._disableUpdates = True
        self.renameByType(folder, [] if which == 1 else nodes)
        self._disableUpdates = False

        if paths:
            commands.setViewNode(folder)

    def deleteViewableSlot(self, checked):
        items = self.selectedItems()

        for item in items:
            node = itemNode(item)
            parent = itemParentNode(item)
            outs = commands.nodeConnections(node)[1]
            parentType = commands.nodeType(parent) if commands.nodeExists(parent) else ""

            nfolders = 0
            for o in outs:
                if commands.nodeType(o) == "RVFolderGroup":
                    nfolders += 1

            if parentType == "RVFolderGroup" and nfolders > 1:
                removeInput(parent, node)
            else:
                self._disableUpdates = True

                try:
                    #
                    #  Another weird situation with orphaned
                    #  items. Just avoid it by preventing updates.
                    #

                    commands.deleteNode(node)
                except Exception as obj:
                    print("Error: %s, failed to delete '%s'\n" % (obj, node))

                self._disableUpdates = False

        self._lazyUpdateTimer.start(0)

    def editViewInfoSlot(self, checked):
        indices = self._viewTreeView.selectionModel().selectedIndexes()
        if not indices:
            return
        index = indices[0]
        self._viewTreeView.edit(index)

    def reorderSelected(self, up, checked):
        indices = self._inputsView.selectionModel().selectedIndexes()

        if not indices:
            return

        inputs = nodeInputs(commands.viewNode())
        minRow = min(indices[0].row(), indices[-1].row())
        maxRow = max(indices[0].row(), indices[-1].row())

        if (up and minRow == 0) or (not up and maxRow == len(inputs) - 1):
            return

        numRows = self._inputsModel.rowCount(QtCore.QModelIndex())
        selectionSizes = []
        selectionSize = 0
        for i in range(numRows):
            index = self._inputsModel.index(i, 0, QtCore.QModelIndex())
            included = includes(indices, index)
            if included:
                selectionSize += 1
            elif selectionSize > 0 or len(selectionSizes) > 0:
                selectionSizes.append(selectionSize)
                selectionSize = 0
        if selectionSize > 0:
            selectionSizes.append(selectionSize)

        includedList = []
        newNodes = [""] * numRows
        sizeIndex = 0
        for i in range(numRows):
            index = self._inputsModel.index(i, 0, QtCore.QModelIndex())
            included = includes(indices, index)
            newIndex = index.row()
            includedInc = -1 if up else 1
            excludedInc = -1 * includedInc * selectionSizes[sizeIndex]
            if included:
                newIndex = newIndex + includedInc
                includedList.append(newIndex)
            elif (
                not included
                and newIndex >= minRow + includedInc
                and newIndex <= maxRow + includedInc
            ):
                newIndex = newIndex + excludedInc
                if sizeIndex < len(selectionSizes) - 1:
                    sizeIndex += 1
            newNodes[newIndex] = nodeFromIndex(index, self._inputsModel)

        try:
            setInputs(commands.viewNode(), newNodes)
            self.selectInputsRange(includedList)
        except Exception as exc:
            print("FAILED: %s\n" % exc)

        commands.redraw()  # don't think this is necessary

    def sortInputs(self, up, checked):
        if self._inputOrderLock or commands.viewNode() is None:
            return

        node = commands.viewNode()
        inputs = nodeInputs(node)

        sorted_ = []

        for i in range(len(inputs)):
            source = inputs[i]
            media = extra_commands.uiName(source)

            found = False
            tmp = []

            for s in sorted_:
                order = _compare(media, extra_commands.uiName(s))
                if found or (up and order > 0) or (not up and order < 0):
                    # while s comes before item, add s to the list
                    tmp.append(s)
                else:
                    # insert item before this source
                    tmp.append(source)
                    tmp.append(s)
                    found = True
            if not found:
                # stick on the end
                sorted_.append(source)
            else:
                sorted_ = tmp

        if not setInputs(node, sorted_):
            self.updateInputs(node)

        if commands.nodeType(node) == "RVFolderGroup":
            for i in range(len(sorted_)):
                n = sorted_[i]
                setSortKeyInParent(n, node, i)
            self.updateTree()

    def rebuildInputsFromList(self):
        if self._inputOrderLock or commands.viewNode() is None:
            return

        num = self._inputsModel.rowCount(QtCore.QModelIndex())
        vnode = commands.viewNode()

        nodes = []

        self._disableUpdates = True

        for row in range(num):
            item = self._inputsModel.item(row, 0)

            if item is not None:
                node = itemNode(item)

                try:
                    if itemIsSubComponent(item):
                        hash = itemSubComponentHash(item)
                        cnode, folder = self.componentAndFolderNodeFromHash(hash, node)

                        if cnode is None:
                            fullName = itemSubComponentValue(item)
                            filename = itemSubComponentMedia(item)
                            subType = itemSubComponentType(item)
                            pval = subComponentPropValue(item)
                            snode = self.newSubComponentNode(
                                hash, subType, filename, fullName, pval, node, folder
                            )

                            nodes.append(snode)
                        else:
                            nodes.append(cnode)
                    else:
                        nodes.append(node)
                except Exception:
                    pass

        commands.setViewNode(vnode)
        self._disableUpdates = False
        if not setInputs(vnode, nodes):
            self.updateInputs(vnode)

    def inputRowsRemovedSlot(self, parent, start, end):
        if self._inputOrderLock or commands.viewNode() is None:
            return
        self._lazySetInputsTimer.start(100)

    def printRows(self):
        num = self._inputsModel.rowCount(QtCore.QModelIndex())

        print("-\n")
        for row in range(num):
            item = self._inputsModel.item(row, 0)
            print("row %d -> %s\n" % (row, "nil" if item is None else item.text()))

    def showRows(self, event):
        self.printRows()

    def inputRowsInsertedSlot(self, parent, start, end):
        if self._inputOrderLock or commands.viewNode() is None:
            return
        self._lazySetInputsTimer.start(100)

    def inputsDeleteSlot(self, checked):
        if self._inputOrderLock or commands.viewNode() is None:
            return

        indices = self._inputsView.selectionModel().selectedIndexes()
        inputs = nodeInputs(commands.viewNode())

        newNodes = []

        for i in range(len(inputs)):
            index = self._inputsModel.index(i, 0, QtCore.QModelIndex())

            if not includes(indices, index):
                newNodes.append(nodeFromIndex(index, self._inputsModel))

        try:
            setInputs(commands.viewNode(), newNodes)
        except Exception as exc:
            print("FAILED: %s\n" % exc)

        commands.redraw()  # don't think this is necessary

    def saveTabState(self):
        prop = "%s.sm_state.tab" % commands.viewNode()
        setIntProp(prop, self._tabWidget.currentIndex())

    def restoreTabState(self):
        vnode = commands.viewNode()

        if vnode is not None:
            prop = "%s.sm_state.tab" % vnode

            if commands.propertyExists(prop):
                state = commands.getIntProperty(prop)[0]
                self._tabWidget.setCurrentIndex(state)
            elif commands.nodeType(vnode) == "RVSourceGroup":
                self._tabWidget.setCurrentIndex(1)

    def tabChangeSlot(self, index):
        self.saveTabState()

    def navButtonClicked(self, which, checked):
        self._disableUpdates = True

        try:
            if which == "next" and commands.nextViewNode() is not None:
                commands.setViewNode(commands.nextViewNode())
            if which == "prev" and commands.previousViewNode() is not None:
                commands.setViewNode(commands.previousViewNode())
        except Exception:
            pass

        self._disableUpdates = False
        self.updateInputs(commands.viewNode())

    def mainWinVisTimeout(self):
        #
        #  Don't adjust mode activity whcn main window
        #  is minimized.
        #
        if qtutils.sessionWindow().isMinimized():
            return

        if not self._dockWidget.isVisible() and self._active:
            self.toggle()
        if self._dockWidget.isVisible() and not self._active:
            self.toggle()

    def visibilityChanged(self, vis):
        #
        #  We want to avoid shutting down the mode when the window
        #  is minimized, but the min status is not correct unless
        #  we ask a little later ;-)
        #
        self._mainWinVisTimer.start(0)

    def configSlot(self, checked, onstart, show):
        commands.writeSettings("SessionManager", "showOnStartup", onstart)
        commands.writeSettings("Tools", "show_session_manager", show)

    def togglePreviews(self, checked):
        self._previewsEnabled = checked
        commands.writeSettings("SessionManager", "previewsEnabled", checked)
        if not checked:
            commands.sendInternalEvent("session-manager-previews-disabled", "")
        else:
            commands.sendInternalEvent("session-manager-previews-enabled", "")
        self.updateTree()

    def __init__(self, name):
        rv.rvtypes.MinorMode.__init__(self)

        self._darkUI = True
        self._inputOrderLock = False
        self._editors = []
        self._quitting = False
        self._disableUpdates = False
        self._srcNodeKeys = []
        self._grpNodeValues = []

        self._css = None
        self._createImageDialog = None
        self._newNodeDialog = None
        self._nodeTypeCombo = None
        self._viewContextMenu = None
        self._viewContextMenuActions = []
        self._cidName = ""
        self._cidFMTSpec = ""
        self._cidColor = QtGui.QColor(0, 0, 0, 255)
        self._selectedSubComp = QtGui.QColor()

        previewsEnv = os.environ.get("RV_SESSION_MANAGER_USE_THUMBNAILS", None)
        if previewsEnv is not None and previewsEnv == "0":
            self._previewsEnabled = False
        else:
            self._previewsEnabled = bool(
                commands.readSettings("SessionManager", "previewsEnabled", True)
            )

        self._progressiveLoadingInProgress = commands.loadTotal() != 0

        self.init(
            name,
            [
                ("new-node", self.updateTreeEvent, "New user node"),
                ("source-modified", self.updateTreeEvent, "New source media"),
                ("source-group-complete", self.updateTreeEvent, "Source group complete"),
                (
                    "before-progressive-loading",
                    self.beforeProgressiveLoading,
                    "before loading",
                ),
                (
                    "after-progressive-loading",
                    self.afterProgressiveLoading,
                    "after loading",
                ),
                ("after-node-delete", self.updateTreeEvent, "Node deleted"),
                ("after-clear-session", self.updateTreeEvent, "Session Cleared"),
                ("after-graph-view-change", self.afterGraphViewChange, "Update session UI"),
                (
                    "before-graph-view-change",
                    self.beforeGraphViewChange,
                    "Update session UI",
                ),
                ("graph-node-inputs-changed", self.nodeInputsChanged, "Update session UI"),
                ("graph-state-change", self.propertyChanged, "Maybe update session UI"),
                ("key-down--@", self.showRows, "show'em"),
                (
                    "before-session-deletion",
                    self.enterQuittingState,
                    "Store quitting before session goes away",
                ),
                (
                    "view-edit-mode-activated",
                    self.viewEditModeActivated,
                    "Per-view edit mode activated, load UI",
                ),
                (
                    "event-category-state-changed",
                    self.onCategoryStateChanged,
                    "Category state changed",
                ),
                (
                    "session-manager-preview-available",
                    self.updateNodePreviewEvent,
                    "Update preview widget for completed thumbnail",
                ),
                (
                    "session-manager-selected-nodes",
                    self.selectedNodesEvent,
                    "Report the tree selection to other packages",
                ),
            ],
            None,
            None,
        )

        #
        #  Every widget below is parented to the session window, so the wrapper for
        #  it has to outlive them: dropping the last Python reference to a wrapper
        #  obtained from wrapInstance() takes the widgets parented to it down with
        #  it, and the panel then raises "Internal C++ object already deleted".
        #
        self._mainWindow = qtutils.sessionWindow()
        m = self._mainWindow

        self._dockWidget = QtWidgets.QDockWidget("Session Manager", m, Qt.Widget)
        self._baseWidget = loadUIFile(self.auxFilePath("session_manager.ui"), m)
        self._treeViewBase = self._baseWidget.findChild(QtWidgets.QWidget, "treeView")
        self._addButton = self._baseWidget.findChild(QtWidgets.QToolButton, "addButton")
        self._folderButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "folderButton"
        )
        self._deleteButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "deleteButton"
        )
        self._configButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "configButton"
        )
        self._editViewInfoButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "renameButton"
        )
        self._homeButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "selectCurrentButton"
        )
        self._inputsViewBase = self._baseWidget.findChild(
            QtWidgets.QWidget, "inputsListView"
        )
        self._tabWidget = self._baseWidget.findChild(QtWidgets.QTabWidget, "tabWidget")
        self._orderUpButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "orderUpButton"
        )
        self._orderDownButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "orderDownButton"
        )
        self._sortAscButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "sortAscButton"
        )
        self._sortDescButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "sortDescButton"
        )
        self._inputsDeleteButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "inputsDeleteButton"
        )
        self._uiTreeWidget = self._baseWidget.findChild(
            QtWidgets.QTreeWidget, "uiTreeWidget"
        )
        self._splitter = self._baseWidget.findChild(QtWidgets.QSplitter, "splitter")
        self._viewLabel = self._baseWidget.findChild(QtWidgets.QLabel, "viewLabel")
        self._prevViewButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "prevViewButton"
        )
        self._nextViewButton = self._baseWidget.findChild(
            QtWidgets.QToolButton, "nextViewButton"
        )

        self._lazySetInputsTimer = QtCore.QTimer(self._dockWidget)
        self._lazyUpdateTimer = QtCore.QTimer(self._dockWidget)
        self._mainWinVisTimer = QtCore.QTimer(self._dockWidget)

        self._lazySetInputsTimer.setSingleShot(True)
        self._lazyUpdateTimer.setSingleShot(True)
        self._mainWinVisTimer.setSingleShot(True)

        vbox = QtWidgets.QVBoxLayout(self._treeViewBase)
        vbox.setContentsMargins(0, 0, 0, 0)
        self._viewTreeView = NodeTreeView(self._treeViewBase)
        vbox.addWidget(self._viewTreeView)

        ivbox = QtWidgets.QVBoxLayout(self._inputsViewBase)
        ivbox.setContentsMargins(0, 0, 0, 0)
        self._inputsView = InputsView(
            self._viewTreeView, self._inputsViewBase, self.updateTree
        )
        ivbox.addWidget(self._inputsView)
        self._inputsView.setObjectName("inputsViewList")

        if self._css is not None:
            self._baseWidget.setStyleSheet(self._css)
        self._dockWidget.setWidget(self._baseWidget)
        self._dockWidget.setTitleBarWidget(
            self._baseWidget.findChild(QtWidgets.QWidget, "navPanel")
        )
        self._dockWidget.setObjectName(name)
        self._eventFilter = EventFilter(qtutils.sessionWindow())
        self._dockWidget.installEventFilter(self._eventFilter)

        self._viewModel = NodeModel(m)
        self._inputsModel = QtGui.QStandardItemModel(m)

        self._viewTreeView._viewModel = self._viewModel

        self._viewModel.setHorizontalHeaderLabels(["Name", "*", "*"])
        self._viewTreeView.header().setMinimumSectionSize(-1)

        self._viewTreeView.setModel(self._viewModel)
        self._viewTreeView.setDragEnabled(True)
        self._viewTreeView.setAcceptDrops(True)
        self._viewTreeView.setDropIndicatorShown(True)
        self._viewTreeView.setHeaderHidden(False)
        self._viewTreeView.setSelectionMode(
            QtWidgets.QAbstractItemView.ExtendedSelection
        )
        self._viewTreeView.setEditTriggers(QtWidgets.QAbstractItemView.EditKeyPressed)
        self._viewTreeView.setContextMenuPolicy(Qt.CustomContextMenu)
        self._viewTreeView.setDragDropMode(QtWidgets.QAbstractItemView.DragDrop)
        self._viewTreeView.setDefaultDropAction(Qt.MoveAction)
        self._viewTreeView.setExpandsOnDoubleClick(False)
        self._viewTreeView.setIndentation(TREE_VIEW_INDENTATION)

        self._inputsView.setModel(self._inputsModel)
        self._inputsView.setDragEnabled(True)
        self._inputsView.setAcceptDrops(True)
        self._inputsView.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self._inputsView.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self._inputsView.setDefaultDropAction(Qt.MoveAction)
        self._inputsView.setDropIndicatorShown(True)
        self._inputsView.setDragDropMode(QtWidgets.QAbstractItemView.DragDrop)
        self._inputsView.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)

        m.addDockWidget(Qt.LeftDockWidgetArea, self._dockWidget)

        addAction = QtGui.QAction(
            self.auxIcon("add_48x48.png", True), "Create View", self._addButton
        )
        folderAction = QtGui.QAction(
            self.auxIcon("foldr_48x48.png", True), "Create Folder", self._folderButton
        )
        deleteAction = QtGui.QAction(
            self.auxIcon("trash_48x48.png", True), "Delete View", self._deleteButton
        )
        configAction = QtGui.QAction(
            self.auxIcon("confg_48x48.png", True), "Configure", self._configButton
        )
        editInfoAction = QtGui.QAction(
            self.auxIcon("sinfo_48x48.png", True),
            "Edit View Info",
            self._editViewInfoButton,
        )
        orderUpAction = QtGui.QAction(
            self.auxIcon("up_48x48.png", True),
            "Move Input Higher in List",
            self._orderUpButton,
        )
        orderDownAction = QtGui.QAction(
            self.auxIcon("down_48x48.png", True),
            "Move Input Lower in List",
            self._orderDownButton,
        )
        sortAscAction = QtGui.QAction("A-Z", self._sortAscButton)
        sortDescAction = QtGui.QAction("Z-A", self._sortDescButton)
        inputsDeleteAction = QtGui.QAction(
            self.auxIcon("trash_48x48.png", True),
            "Delete Input",
            self._inputsDeleteButton,
        )
        prevViewAction = QtGui.QAction(
            self.auxIcon("back_48x48.png", True), "Previous View", self._prevViewButton
        )
        nextViewAction = QtGui.QAction(
            self.auxIcon("forwd_48x48.png", True), "Next View", self._nextViewButton
        )
        homeAction = QtGui.QAction(
            self.auxIcon("home_48x48.png", True),
            "Select Current View",
            self._homeButton,
        )

        #
        #  Cache all the icons ahead of time (this was seriously
        #  slowing things down before). Put the icons in *reverse*
        #  order of likelyhood they'll appear. i.e. first in list is
        #  least likely to be needed, last is most likely.
        #

        self._typeIcons = []

        for t in [
            ("RVSourceGroup", "videofile_48x48.png"),
            ("RVImageSource", "videofile_48x48.png"),
            ("RVSwitchGroup", "shuffle_48x48.png"),
            ("RVRetimeGroup", "tempo_48x48.png"),
            ("RVLayoutGroup", "lgicn_48x48.png"),
            ("RVStackGroup", "photoalbum_48x48.png"),
            ("RVSequenceGroup", "playlist_48x48.png"),
            ("RVFolderGroup", "foldr_48x48.png"),
            ("RVFileSource", "videofile_48x48.png"),
        ]:
            self._typeIcons.append((t[0], self.auxIcon(t[1], True)))

        self._viewIcon = self.auxIcon("view.png", True)
        self._videoIcon = self.auxIcon("video_48x48.png", True)
        self._channelIcon = self.auxIcon("channel.png", True)
        self._layerIcon = self.auxIcon("layer.png", True)
        self._unknownTypeIcon = self.auxIcon("new_48x48.png", True)
        self._fallbackSourceIcon = QtGui.QIcon(
            self.auxFilePath("fallback_thumbnail.png")
        )

        self._addButton.setDefaultAction(addAction)
        self._deleteButton.setDefaultAction(deleteAction)
        self._editViewInfoButton.setDefaultAction(editInfoAction)
        self._addButton.setPopupMode(QtWidgets.QToolButton.InstantPopup)

        self._configButton.setDefaultAction(configAction)
        self._configButton.setPopupMode(QtWidgets.QToolButton.InstantPopup)

        self._colorDialog = QtWidgets.QColorDialog(m)
        self._colorDialog.setOption(QtWidgets.QColorDialog.ShowAlphaChannel, False)

        self._orderUpButton.setDefaultAction(orderUpAction)
        self._orderDownButton.setDefaultAction(orderDownAction)
        self._sortAscButton.setDefaultAction(sortAscAction)
        self._sortDescButton.setDefaultAction(sortDescAction)
        self._inputsDeleteButton.setDefaultAction(inputsDeleteAction)

        self._prevViewButton.setDefaultAction(prevViewAction)
        self._nextViewButton.setDefaultAction(nextViewAction)
        self._homeButton.setDefaultAction(homeAction)

        addMenu = QtWidgets.QMenu("New Viewable", self._addButton)
        addSequence = addMenu.addAction(
            self.auxIcon("playlist_48x48.png", True), "Sequence"
        )
        addStack = addMenu.addAction(
            self.auxIcon("photoalbum_48x48.png", True), "Stack"
        )
        addSwitch = addMenu.addAction(self.auxIcon("shuffle_48x48.png", True), "Switch")
        addFolder = addMenu.addAction(self.auxIcon("foldr_48x48.png", True), "Folder")
        addLayout = addMenu.addAction(self.auxIcon("lgicn_48x48.png", True), "Layout")
        addRetime = addMenu.addAction(self.auxIcon("tempo_48x48.png", True), "Retime")

        addColorize = None
        addOCIO = None
        addDynamic = None
        addUserNode = None

        #
        #  For now, remove the rv/rvsdi/rvx dependency here.  Hide Dynamic node from everyone.
        #
        if True or commands.shortAppName() == "rvsdi" or commands.shortAppName() == "rvx":
            addColorize = addMenu.addAction(self.auxIcon("new_48x48.png", True), "Color")
            addOCIO = addMenu.addAction(self.auxIcon("new_48x48.png", True), "OCIO")
            if os.environ.get("RV_ENABLE_DYNAMIC_NODE", None) is not None:
                addDynamic = addMenu.addAction(
                    self.auxIcon("new_48x48.png", True), "Dynamic"
                )
            addUserNode = addMenu.addAction(
                self.auxIcon("new_48x48.png", True), "New Node by Type..."
            )

        addMenu.addSeparator()
        addSRGBCChart = addMenu.addAction(
            self.auxIcon("colorchart_48x48.png", True), "SRGB Color Chart..."
        )
        addACESCChart = addMenu.addAction(
            self.auxIcon("colorchart_48x48.png", True), "ACES Color Chart..."
        )
        addCBars = addMenu.addAction(
            self.auxIcon("ntscbars_48x48.png", True), "Color Bars..."
        )
        addBlack = addMenu.addAction(self.auxIcon("video_48x48.png", True), "Black...")
        addColor = addMenu.addAction(self.auxIcon("video_48x48.png", True), "Color...")
        addBlank = addMenu.addAction(self.auxIcon("video_48x48.png", True), "Blank...")

        menuActions = [
            (addStack, "RVStackGroup"),
            (addFolder, "RVFolderGroup"),
            (addLayout, "RVLayoutGroup"),
            (addSequence, "RVSequenceGroup"),
            (addRetime, "RVRetimeGroup"),
            (addSwitch, "RVSwitchGroup"),
            (addCBars, "smptebars,%s.movieproc"),
            (addSRGBCChart, "srgbcolorchart,%s.movieproc"),
            (addACESCChart, "acescolorchart,%s.movieproc"),
            (addBlack, "black,%s.movieproc"),
            (addColor, "solid,%s.movieproc"),
            (addBlank, "blank,%s.movieproc"),
        ]

        if True or commands.shortAppName() == "rvsdi" or commands.shortAppName() == "rvx":
            if os.environ.get("RV_ENABLE_DYNAMIC_NODE", None) is not None:
                menuActions = [
                    (addStack, "RVStackGroup"),
                    (addFolder, "RVFolderGroup"),
                    (addLayout, "RVLayoutGroup"),
                    (addSequence, "RVSequenceGroup"),
                    (addRetime, "RVRetimeGroup"),
                    (addSwitch, "RVSwitchGroup"),
                    (addColorize, "RVColor"),
                    (addOCIO, "RVOCIO"),
                    (addDynamic, "Dynamic"),
                    (addUserNode, ""),
                    (addSRGBCChart, "srgbcolorchart,%s.movieproc"),
                    (addACESCChart, "acescolorchart,%s.movieproc"),
                    (addCBars, "smptebars,%s.movieproc"),
                    (addBlack, "black,%s.movieproc"),
                    (addColor, "solid,%s.movieproc"),
                    (addBlank, "blank,%s.movieproc"),
                ]
            else:
                menuActions = [
                    (addStack, "RVStackGroup"),
                    (addFolder, "RVFolderGroup"),
                    (addLayout, "RVLayoutGroup"),
                    (addSequence, "RVSequenceGroup"),
                    (addRetime, "RVRetimeGroup"),
                    (addSwitch, "RVSwitchGroup"),
                    (addColorize, "RVColor"),
                    (addOCIO, "RVOCIO"),
                    (addUserNode, ""),
                    (addSRGBCChart, "srgbcolorchart,%s.movieproc"),
                    (addACESCChart, "acescolorchart,%s.movieproc"),
                    (addCBars, "smptebars,%s.movieproc"),
                    (addBlack, "black,%s.movieproc"),
                    (addColor, "solid,%s.movieproc"),
                    (addBlank, "blank,%s.movieproc"),
                ]

        self._addButton.setMenu(addMenu)
        self._addButton.setArrowType(Qt.NoArrow)
        self._createMenu = addMenu

        folderMenu = QtWidgets.QMenu("New Folder", self._folderButton)
        newFolderAction = folderMenu.addAction("Empty Folder")
        newFolder2Action = folderMenu.addAction("From Selection")
        newFolder3Action = folderMenu.addAction("From Copy of Selection")

        self._folderButton.setDefaultAction(folderAction)
        self._folderButton.setMenu(folderMenu)
        self._folderButton.setArrowType(Qt.NoArrow)
        self._folderButton.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        self._folderMenu = folderMenu

        configMenu = QtWidgets.QMenu("Config", self._configButton)
        configAlwaysOn = configMenu.addAction("Always Show at Start Up")
        configNeverOn = configMenu.addAction("Never Show at Start Up")
        configLastOn = configMenu.addAction("Restore Last State at Start Up")
        configGroup = QtGui.QActionGroup(self._configButton)

        for a in [configAlwaysOn, configNeverOn, configLastOn]:
            a.setCheckable(True)
            configGroup.addAction(a)

        configMenu.addSeparator()
        previewToggle = configMenu.addAction("Show Source Previews")

        previewToggle.setCheckable(True)
        previewToggle.setChecked(self._previewsEnabled)

        if previewsEnv is not None and previewsEnv == "0":
            previewToggle.setEnabled(False)

        self._configButton.setMenu(configMenu)

        try:
            configState = str(
                commands.readSettings("SessionManager", "showOnStartup", "no")
            )

            if configState == "yes":
                configAlwaysOn.setChecked(True)
            elif configState == "last":
                configLastOn.setChecked(True)
            else:
                configNeverOn.setChecked(True)
        except Exception:
            pass

        for action, protocol in menuActions:
            action.triggered.connect(
                lambda checked=False, p=protocol: self.addThingSlot(checked, p)
            )

        self._viewContextMenuActions = [deleteAction, editInfoAction, homeAction]

        homeAction.triggered.connect(self.selectCurrentViewSlot)
        deleteAction.triggered.connect(self.deleteViewableSlot)
        editInfoAction.triggered.connect(self.editViewInfoSlot)
        orderUpAction.triggered.connect(
            lambda checked=False: self.reorderSelected(True, checked)
        )
        orderDownAction.triggered.connect(
            lambda checked=False: self.reorderSelected(False, checked)
        )
        sortAscAction.triggered.connect(
            lambda checked=False: self.sortInputs(True, checked)
        )
        sortDescAction.triggered.connect(
            lambda checked=False: self.sortInputs(False, checked)
        )
        inputsDeleteAction.triggered.connect(self.inputsDeleteSlot)
        prevViewAction.triggered.connect(
            lambda checked=False: self.navButtonClicked("prev", checked)
        )
        nextViewAction.triggered.connect(
            lambda checked=False: self.navButtonClicked("next", checked)
        )
        self._tabWidget.currentChanged.connect(self.tabChangeSlot)
        self._inputsModel.rowsRemoved.connect(self.inputRowsRemovedSlot)
        self._inputsModel.rowsInserted.connect(self.inputRowsInsertedSlot)
        self._lazySetInputsTimer.timeout.connect(self.rebuildInputsFromList)
        self._lazyUpdateTimer.timeout.connect(self.updateTree)
        self._mainWinVisTimer.timeout.connect(self.mainWinVisTimeout)
        self._colorDialog.currentColorChanged.connect(self.newColorSlot)
        self._splitter.splitterMoved.connect(self.splitterMoved)
        configAlwaysOn.triggered.connect(
            lambda checked=False: self.configSlot(checked, "yes", True)
        )
        configNeverOn.triggered.connect(
            lambda checked=False: self.configSlot(checked, "no", False)
        )
        configLastOn.triggered.connect(
            lambda checked=False: self.configSlot(checked, "last", True)
        )
        previewToggle.toggled.connect(self.togglePreviews)
        self._viewModel.itemChanged.connect(self.viewItemChanged)
        self._viewTreeView.expanded.connect(
            lambda index: self.setItemExpandedState(index, 1)
        )
        self._viewTreeView.collapsed.connect(
            lambda index: self.setItemExpandedState(index, 0)
        )
        self._viewTreeView.customContextMenuRequested.connect(self.viewContextMenuSlot)
        self._viewTreeView.doubleClicked.connect(
            lambda index: self.viewByIndex(index, self._viewModel)
        )
        self._viewTreeView.pressed.connect(
            lambda index: self.itemPressed(index, self._viewModel)
        )
        self._inputsView.doubleClicked.connect(
            lambda index: self.viewByIndex(index, self._inputsModel)
        )
        newFolderAction.triggered.connect(
            lambda checked=False: self.newFolderSlot(checked, 1)
        )
        newFolder2Action.triggered.connect(
            lambda checked=False: self.newFolderSlot(checked, 2)
        )
        newFolder3Action.triggered.connect(
            lambda checked=False: self.newFolderSlot(checked, 3)
        )

        #
        #  Create the props on the display node we'll use
        #

        self.updateTree()

        self._dockWidget.show()
        m.show()

        # The Mu original records itself in the Mu State (state.sessionManager) for
        # its sibling modes to find; the field is typed as the Mu class, so a Python
        # instance cannot be stored there. theMode() below serves the same purpose.
        global _theMode
        _theMode = self

        self._dockWidget.visibilityChanged.connect(self.visibilityChanged)

        self.updateNavUI()


_theMode = None


def createMode():
    return SessionManagerMode("session_manager")


def theMode():
    return _theMode


def selectedNodeLines():
    """The tree selection as one node name per line, for Mu callers.

    Mu packages used to hold the mode object and call selectedNodes() on it, which
    worked whether or not the panel was open, because the tree view exists from the
    constructor onward. An internal event cannot reproduce that: RV only dispatches
    events to *active* modes, and session_manager is `load: delay`, so a closed panel
    would report an empty selection and silently change those callers' menu states.

    This is reachable through Mu's python module for as long as the mode is
    constructed, which restores the original semantics. Returns "" when the Python
    implementation is not the one loaded, so callers fall back to the event.
    """
    mode = theMode()

    if mode is None:
        return ""

    return "\n".join(mode.selectedNodes())
