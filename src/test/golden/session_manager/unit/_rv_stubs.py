"""Import the real session_manager port with the RV bindings faked out (gate 5).

Gate 5 exists to pin the behavior of the **ported code**, so these tests import the
actual modules from ``src/plugins/rv-packages/session_manager/`` rather than
restating their logic. What has to be faked is only the boundary: the ``rv.*``
modules are C++ bindings that exist solely inside a running RV process, and
VERIFICATION.md requires unit tests not to need one.

``FakeGraph`` stands in for the parts of ``rv.commands`` the package touches — a
property store plus a node graph — so a test can set up a graph, call into the port,
and assert on the properties it wrote. PySide6 is real: the widget and model code is
a large part of what was ported, and stubbing Qt would only test the stubs. That is
why gate 5 runs under RV's bundled interpreter, which is where PySide6 lives.
"""
from __future__ import annotations

import os
import sys
import types

#  realpath first: __file__ is relative when this module is imported through a
#  relative sys.path entry, and the ".." walk then climbs out of the repo and
#  silently yields /plugins/rv-packages/session_manager.
PKG_DIR = os.path.abspath(
    os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "..", "..", "..", "..",
        "plugins", "rv-packages", "session_manager",
    )
)


#  The single session window; see _sessionWindow() below.
_SESSION_WINDOW = None


class PropertyError(Exception):
    """Stands in for the exception RV raises for a bad property access."""


class FakeGraph:
    """The slice of RV's session the package reads and writes.

    Property names are the ones the port passes to rv.commands: either fully
    qualified (``node.component.name``) or one of RV's ``#Type``/``#View``/
    ``#Session`` shorthands, which resolve against the current view node.
    """

    INT, FLOAT, STRING = "int", "float", "string"

    def __init__(self):
        self.props = {}            # name -> (type, [values])
        self.nodes = {}            # node -> type
        self.groups = {}           # group -> [member nodes]
        self.connections = {}      # node -> [inputs]
        self.uiNames = {}
        self.viewNode = None
        self.viewNodes = []
        self.deleted = []
        self.reloaded = 0
        self.redraws = 0
        self.events = []           # (name, contents) from sendInternalEvent
        self.feedback = []
        self.alerts = []
        self.settings = {}
        self.enabledCategories = None   # None = every category enabled
        self.nodeCounter = 0
        self.inFrame = 1                # playback in/out, distinct from cut.in/out
        self.outFrame = 100
        self.fps = 24.0

    # -- name resolution ---------------------------------------------------

    def resolve(self, name):
        """Expand RV's ``#`` property shorthands against the current view node."""
        if not name.startswith("#"):
            return name

        head, _, rest = name.partition(".")
        kind = head[1:]

        if kind == "Session":
            return "rv.session." + rest
        if kind == "View":
            return "%s.%s" % (self.viewNode, rest)

        # "#RVStack.composite.type" means the RVStack inside the current view.
        for node in self.nodesInGroup(self.viewNode):
            if self.nodes.get(node) == kind:
                return "%s.%s" % (node, rest)
        if self.nodes.get(self.viewNode) == kind:
            return "%s.%s" % (self.viewNode, rest)
        return "%s.%s" % (self.viewNode, rest)

    # -- graph construction (test-side helpers) ----------------------------

    def addNode(self, node, nodeType, group=None, inputs=None):
        self.nodes[node] = nodeType
        self.connections.setdefault(node, list(inputs or []))
        if group is not None:
            self.groups.setdefault(group, []).append(node)
        return node

    def addSourceGroup(self, name="sourceGroup", media="movie.mov"):
        group = self.addNode(name, "RVSourceGroup")
        src = self.addNode(name + "_source", "RVFileSource", group=group)
        self.seedString(src + ".media.movie", [media])
        #  request.imageComponent is always present on a real source; newNodeRow
        #  reads it unguarded. Empty means "no sub-component selected".
        self.seedString(src + ".request.imageComponent", [])
        self.viewNodes.append(group)
        return group

    # -- rv.commands surface ----------------------------------------------

    def propertyExists(self, name):
        return self.resolve(name) in self.props

    def newProperty(self, name, propType, width):
        self.props[self.resolve(name)] = (propType, [])

    def deleteProperty(self, name):
        self.props.pop(self.resolve(name), None)

    def _get(self, name, propType):
        full = self.resolve(name)
        if full not in self.props:
            raise PropertyError("no property %s" % full)
        actual, values = self.props[full]
        if actual != propType:
            raise PropertyError(
                "badPropertyType: %s is %s, read as %s" % (full, actual, propType)
            )
        return list(values)

    def _set(self, name, values, propType, allowResize):
        """Write an existing property, or raise the way RV does for a missing one.

        set*Property's third argument is ``allowResize``, NOT "create if missing":
        RV's getProperty() throws badProperty before setIntProperty ever looks at
        the flag, which is the entire reason extra_commands.cprop() exists. This
        stub used to treat it as create-if-missing, which made it forgiving enough
        that gutting _cprop() in the port left every test passing even though real
        RV would have raised at each of those call sites.

        Tests seed the graph with seedProperty() instead, the stand-in for
        newProperty().
        """
        full = self.resolve(name)

        if full not in self.props:
            raise PropertyError("badProperty: no property %s" % full)

        actual, existing = self.props[full]

        if actual != propType:
            raise PropertyError(
                "badPropertyType: %s is %s, written as %s" % (full, actual, propType)
            )

        if not allowResize and len(values) != len(existing):
            raise PropertyError(
                "property %s holds %d value(s), written with %d and allowResize false"
                % (full, len(existing), len(values))
            )

        self.props[full] = (propType, list(values))

    def seedProperty(self, name, propType, values):
        """Create a property outright, as newProperty() would. Test-side only."""
        self.props[self.resolve(name)] = (propType, list(values))

    def seedInt(self, name, values):
        self.seedProperty(name, self.INT, values)

    def seedFloat(self, name, values):
        self.seedProperty(name, self.FLOAT, values)

    def seedString(self, name, values):
        self.seedProperty(name, self.STRING, values)

    def getIntProperty(self, name, *a):
        return self._get(name, self.INT)

    def getFloatProperty(self, name, *a):
        return self._get(name, self.FLOAT)

    def getStringProperty(self, name, *a):
        return self._get(name, self.STRING)

    def setIntProperty(self, name, values, allowResize=False):
        self._set(name, values, self.INT, allowResize)

    def setFloatProperty(self, name, values, allowResize=False):
        self._set(name, values, self.FLOAT, allowResize)

    def setStringProperty(self, name, values, allowResize=False):
        self._set(name, values, self.STRING, allowResize)

    def nodeExists(self, node):
        return node in self.nodes

    def nodeType(self, node):
        return self.nodes.get(node, "")

    def nodesInGroup(self, group):
        return list(self.groups.get(group, []))

    def nodeGroup(self, node):
        for group, members in self.groups.items():
            if node in members:
                return group
        return None

    def nodes_(self):
        return list(self.nodes)

    def nodeConnections(self, node, traverse=False):
        """(inputs, outputs), as RV returns them.

        The outputs half is derived rather than stubbed out: deleteViewableSlot
        counts how many folders a node feeds to decide between unlinking it and
        deleting it outright, so an always-empty outputs list would make it delete
        a node that two folders share.
        """
        outputs = [n for n, ins in self.connections.items() if node in ins]
        return (list(self.connections.get(node, [])), outputs)

    def setNodeInputs(self, node, inputs):
        self.connections[node] = list(inputs)

    def testNodeInputs(self, node, inputs):
        for i in inputs:
            if i not in self.nodes:
                return "no such node: %s" % i
        return None

    def newNode(self, nodeType, name=None):
        self.nodeCounter += 1
        node = name or "%s%06d" % (nodeType, self.nodeCounter)
        self.addNode(node, nodeType)
        self.viewNodes.append(node)
        return node

    def addSourceVerbose(self, media=None):
        self.nodeCounter += 1
        group = "sourceGroup%06d" % self.nodeCounter
        self.addSourceGroup(group, media=(media or ["movie.mov"])[0])
        return group + "_source"

    def deleteNode(self, node):
        self.deleted.append(node)
        self.nodes.pop(node, None)
        self.connections.pop(node, None)
        if node in self.viewNodes:
            self.viewNodes.remove(node)

    def setViewNode(self, node):
        self.viewNode = node

    def uiName(self, node):
        return self.uiNames.get(node, node)

    def setUIName(self, node, name):
        self.uiNames[node] = name

    def isEventCategoryEnabled(self, category):
        if self.enabledCategories is None:
            return True
        return category in self.enabledCategories

    def sendInternalEvent(self, name, contents="", sender=""):
        self.events.append((name, contents))
        return ""

    def setInPoint(self, frame):
        self.inFrame = frame

    def setOutPoint(self, frame):
        self.outFrame = frame

    def setFPS(self, fps):
        self.fps = fps

    def redraw(self):
        self.redraws += 1

    def reload(self):
        self.reloaded += 1


def install(graph=None):
    """Put fake ``rv.*`` modules in sys.modules and return the FakeGraph."""
    graph = graph or FakeGraph()

    commands = types.ModuleType("rv.commands")
    commands.IntType, commands.FloatType, commands.StringType = (
        FakeGraph.INT, FakeGraph.FLOAT, FakeGraph.STRING,
    )
    commands.NeutralMenuState = 0
    commands.UncheckedMenuState = 1
    commands.CheckedMenuState = 2
    commands.MixedStateMenuState = 3
    commands.DisabledMenuState = -1
    commands.ErrorAlert = 2

    for name in (
        "propertyExists", "newProperty", "deleteProperty",
        "getIntProperty", "getFloatProperty", "getStringProperty",
        "setIntProperty", "setFloatProperty", "setStringProperty",
        "nodeExists", "nodeType", "nodesInGroup", "nodeGroup",
        "nodeConnections", "setNodeInputs", "testNodeInputs",
        "newNode", "deleteNode", "setViewNode", "isEventCategoryEnabled",
        "sendInternalEvent", "redraw", "reload",
    ):
        setattr(commands, name, getattr(graph, name))

    commands.nodes = graph.nodes_
    commands.viewNode = lambda: graph.viewNode
    commands.viewNodes = lambda: list(graph.viewNodes)
    commands.nodeTypes = lambda *a: []
    commands.previousViewNode = lambda: None
    commands.nextViewNode = lambda: None
    commands.frame = lambda: 1
    commands.frameStart = lambda: 1
    commands.frameEnd = lambda: 100
    #
    #  The in/out points are the playback range the GUI shows, distinct from the
    #  source's own cut.in/cut.out properties. SourceGroup_edit_mode's whole job is
    #  keeping the two in step, so a stub that discarded the writes would let a
    #  one-directional sync pass.
    #
    commands.inPoint = lambda: graph.inFrame
    commands.outPoint = lambda: graph.outFrame
    commands.setInPoint = graph.setInPoint
    commands.setOutPoint = graph.setOutPoint
    commands.setFPS = graph.setFPS
    commands.sourcesRendered = lambda: []
    commands.renderedImages = lambda: []
    commands.sourceMediaInfo = lambda *a: {}
    commands.sourceMediaInfoList = lambda *a: []
    commands.loadTotal = lambda: 0
    commands.readSettings = lambda g, k, d: graph.settings.get((g, k), d)
    commands.writeSettings = lambda g, k, v: graph.settings.__setitem__((g, k), v)
    commands.displayFeedback = lambda msg, t=1: graph.feedback.append(msg)
    commands.alertPanel = lambda *a, **k: graph.alerts.append(a)
    commands.bind = lambda *a, **k: None
    commands.activateMode = lambda *a, **k: None
    #
    #  addSourceVerbose returns the *source* node, not the group. The package
    #  immediately calls nodeGroup() on the result, so a stub that handed back a
    #  bare group would make every caller write its properties onto None.
    #
    commands.addSourceVerbose = lambda media=None, tag="": graph.addSourceVerbose(media)
    commands.setCursor = lambda *a: None
    commands.shortAppName = lambda: "rv"
    commands.myNetworkHost = lambda: "localhost"
    commands.myNetworkPort = lambda: 0
    commands.remoteLocalContactName = lambda: ""
    commands.imageGeometryByIndex = lambda *a: []
    commands.imagesAtPixel = lambda *a, **k: []
    commands.nodeImageGeometry = lambda *a: {}
    commands.metaEvaluateClosestByType = lambda *a, **k: []

    extra = types.ModuleType("rv.extra_commands")
    extra.uiName = graph.uiName
    extra.setUIName = graph.setUIName
    extra.displayFeedback = commands.displayFeedback
    extra.cprop = lambda name, t: (
        None if graph.propertyExists(name) else graph.newProperty(name, t, 1)
    )

    def _extraSet(name, value):
        raise TypeError(
            "Bad argument (1) to function extra_commands.set: expecting dynamic array"
        )

    extra.set = _extraSet

    #
    #  RV's session window is a QMainWindow and the mode parents its dock to it, so a
    #  real one is the faithful stand-in — returning None makes addDockWidget() fail
    #  and the mode cannot be constructed at all. Held on the module so the wrapper
    #  outlives the widgets parented to it, which is the same lifetime rule the port
    #  itself has to observe.
    #
    qtutils = types.ModuleType("rv.qtutils")

    class _GLView(object):
        """Stands in for the main GL view; the mode only forwards events to it."""

        def __init__(self):
            self.forwarded = []

        def eventFilter(self, obj, event):
            self.forwarded.append((obj, event.type()))
            return False

    def _sessionWindow():
        #
        #  One window for the whole process, held in a module global rather than on
        #  this per-call qtutils stand-in. RV has exactly one session window, and a
        #  fresh QMainWindow per importPort() left dialogs from an earlier test
        #  parented to a window that had since been collected — which shows up as a
        #  segfault inside QDialog.show() much later in the run, in whichever test
        #  happened to be next.
        #
        global _SESSION_WINDOW

        if _SESSION_WINDOW is None:
            from PySide6 import QtWidgets as _QtWidgets

            _SESSION_WINDOW = _QtWidgets.QMainWindow()
        return _SESSION_WINDOW

    qtutils._window = None
    qtutils._glView = _GLView()
    qtutils.sessionWindow = _sessionWindow
    qtutils.sessionGLView = lambda: qtutils._glView

    runtime = types.ModuleType("rv.runtime")
    runtime.eval = lambda code, modules=None: ""

    rvtypes = types.ModuleType("rv.rvtypes")

    class MinorMode(object):
        def __init__(self):
            self._modeName = ""
            self._active = False

        def init(self, name, globalBindings, overrideBindings, menu=None,
                 sortKey=None, ordering=None):
            #
            #  Everything init() is handed is retained. RV keeps the bindings and the
            #  sort key internally with no accessor, but they are the whole
            #  registration contract of a mode — a dropped event binding or a changed
            #  sort key is invisible to a golden and changes when the mode runs.
            #
            self._modeName = name
            self._menu = menu
            self._globalBindings = globalBindings
            self._overrideBindings = overrideBindings
            self._sortKey = sortKey
            self._ordering = ordering

        def supportPath(self, module, packageName):
            return PKG_DIR

        def setMenu(self, menu):
            self._menu = menu

        def activate(self):
            self._active = True

        def deactivate(self):
            self._active = False

    rvtypes.MinorMode = MinorMode
    rvtypes.MinorMode.__module__ = "rv.rvtypes"

    rv = types.ModuleType("rv")
    rv.commands = commands
    rv.extra_commands = extra
    rv.qtutils = qtutils
    rv.runtime = runtime
    rv.rvtypes = rvtypes

    sys.modules.update({
        "rv": rv,
        "rv.commands": commands,
        "rv.extra_commands": extra,
        "rv.qtutils": qtutils,
        "rv.runtime": runtime,
        "rv.rvtypes": rvtypes,
    })

    if PKG_DIR not in sys.path:
        sys.path.insert(0, PKG_DIR)

    return graph


PORT_MODULES = (
    "session_manager",
    "Composite_edit_mode",
    "FolderGroup_edit_mode",
    "LayoutGroup_edit_mode",
    "RetimeGroup_edit_mode",
    "SequenceGroup_edit_mode",
    "SourceGroup_edit_mode",
    "StackGroup_edit_mode",
    "Stack_edit_mode",
    "SwitchGroup_edit_mode",
    "Switch_edit_mode",
    "transform_manip",
)


def importPort(moduleName="session_manager", graph=None):
    """Fresh import of a ported module against a fresh FakeGraph.

    Every port module is dropped from sys.modules first, session_manager included
    even when a sibling was asked for. Each module binds ``rv.commands`` at import
    time and the siblings additionally do ``from session_manager import ...``, so a
    surviving session_manager would hand the sibling helpers still closed over the
    previous test's graph — which shows up as writes landing nowhere.
    """
    graph = install(graph)
    for name in PORT_MODULES:
        sys.modules.pop(name, None)
    module = __import__(moduleName)
    return module, graph


def requiresPySide6():
    """Skip reason when PySide6 is absent, else None.

    Gate 5 is meant to run under RV's bundled interpreter (run_unit_tests.sh picks
    it up); a stock python3 usually has no PySide6 and would report every
    widget-level test as an error rather than as a skip.
    """
    try:
        import PySide6  # noqa: F401
    except ImportError:
        return "PySide6 not available — run gate 5 via harness/run_unit_tests.sh"
    return None
