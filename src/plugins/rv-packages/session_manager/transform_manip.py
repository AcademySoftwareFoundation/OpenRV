#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""2D transform manip — Python port of transform_manip.mu.

Method and function names follow the Mu original rather than PEP 8 so the two can
be read side by side, and so each Mu method maps to one Python method.
"""
import math

import rv.commands as commands
import rv.rvtypes

from PySide6.QtCore import Qt

from session_manager import menuItem, setFloatProp, setStringProp

from OpenGL.GL import *
from OpenGL.GLU import *

# rvui.globalConfig bg and fg. The Mu Configuration object has no Python binding and
# neither entry is reassigned at runtime.
CONFIG_BG = (0.0, 0.0, 0.0, 0.75)
CONFIG_FG = (0.75, 0.75, 0.75, 1.0)

NoControl = "NoControl"
FreeTranslation = "FreeTranslation"
TopLeftCorner = "TopLeftCorner"
TopRightCorner = "TopRightCorner"
BotLeftCorner = "BotLeftCorner"
BotRightCorner = "BotRightCorner"


def _add(a, b):
    return (a[0] + b[0], a[1] + b[1])


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1])


def _scale(a, s):
    return (a[0] * s, a[1] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1]


def mag(a):
    return math.sqrt(a[0] * a[0] + a[1] * a[1])


def normalize(a):
    return _scale(a, 1.0 / mag(a))


def _glVertex(v):
    glVertex2f(v[0], v[1])


def setupProjection(w, h, vflip=False):
    """glyph.setupProjection; the Mu glyph module has no Python binding."""
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    if vflip:
        gluOrtho2D(0.0, w - 1, h - 1, 0.0)
    else:
        gluOrtho2D(0.0, w - 1, 0.0, h - 1)

    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()


def drawCircleFan(x, y, w, start, end, ainc, outline=False):
    a0 = start * math.pi * 2.0
    a1 = end * math.pi * 2.0

    glBegin(GL_LINE_STRIP if outline else GL_TRIANGLE_FAN)
    if not outline:
        glVertex2f(x, y)

    a = a0
    while a < a1:
        glVertex2f(math.sin(a) * w + x, math.cos(a) * w + y)
        a += ainc

    glVertex2f(math.sin(a1) * w + x, math.cos(a1) * w + y)
    glEnd()


def triangleGlyph(outline):
    glBegin(GL_LINE_LOOP if outline else GL_TRIANGLES)
    glVertex2f(-0.5, 0.0)
    glVertex2f(0.5, -0.5)
    glVertex2f(0.5, 0.5)
    glEnd()


def circleGlyph(outline):
    drawCircleFan(0.0, 0.0, 0.5, 0.0, 1.0, 0.3, outline)


def tformCircle(outline):
    glMatrixMode(GL_MODELVIEW)
    glPushMatrix()
    glScalef(0.2333, 0.2333, 0.2333)
    circleGlyph(outline)
    glPopMatrix()


def tformTriangle(angle, outline):
    glMatrixMode(GL_MODELVIEW)
    glPushMatrix()
    glRotatef(angle, 0.0, 0.0, 1.0)
    glScalef(0.25, 0.25, 0.25)
    glTranslatef(-1.3, 0.0, 0.0)
    triangleGlyph(outline)
    glPopMatrix()


def translateIconGlyph(outline):
    tformCircle(outline)

    a = 0.0
    while a <= 360.0:
        tformTriangle(a, outline)
        a += 90.0


def closestPointOnLine(p, a, b):
    dir = normalize(_sub(b, a))
    u = dot(_sub(p, a), dir)

    return _add(_scale(dir, u), a)


def computeGC(corners):
    gc = (0.0, 0.0)
    for c in corners:
        gc = _add(gc, c)
    return _scale(gc, 1.0 / float(len(corners)))


def tagValue(tags, name):
    for t in tags:
        (n, v) = t
        if n == name:
            return v

    return None


def nodeAspect(node):
    geom = commands.nodeImageGeometry(commands.viewNode(), commands.frame())
    pa = geom["pixelAspect"]
    xps = pa if pa > 1.0 else 1.0
    yps = pa if pa < 1.0 else 1.0

    return (geom["width"] * xps) / (geom["height"] / yps)


class EditNodePair(object):
    def __init__(self, tformNode, inputNode):
        self.tformNode = tformNode
        self.inputNode = inputNode


class TransformManip(rv.rvtypes.MinorMode):
    def editNode(self, name):
        for enode in self._editNodes:
            if enode.tformNode == name:
                return enode
        return None

    def activeImageIndex(self):
        for i in commands.renderedImages():
            v = tagValue(i["tags"], "tmanip_state")
            if v is not None and v != "":
                return i["index"]

        return -1

    def setManipState(self, p, value):
        if p is not None:
            if commands.nodeExists(p.tformNode):
                setStringProp(p.tformNode + ".tag.tmanip_state", value)

    def control(self, index, event):
        corners = commands.imageGeometryByIndex(index)
        p = event.pointer()
        gc = computeGC(corners)

        for c in corners:
            v = _sub(p, c)

            if abs(v[0]) < 25 and abs(v[1]) < 25:
                if c[0] < gc[0]:
                    return (TopLeftCorner if c[1] > gc[1] else BotLeftCorner, gc, c)
                else:
                    return (TopRightCorner if c[1] > gc[1] else BotRightCorner, gc, c)

        return (FreeTranslation, gc, gc)

    #
    #  Cursor shapes are passed as .value rather than int(): PySide6 6.5 makes
    #  Qt.CursorShape a plain enum.Enum, so int() on one raises TypeError. Mu hands
    #  setCursor the enum directly and it arrives as an int. Same trap as
    #  Qt.CheckState in the checkbox slots.
    #
    def move(self, event):
        last = self._currentEditNode
        self._currentEditNode = None
        self._control = NoControl
        commands.setCursor(Qt.CursorShape.ArrowCursor.value)

        for p in commands.imagesAtPixel(event.pointer()):
            if p["inside"]:
                v = tagValue(p["tags"], "tmanip")

                if v is not None:
                    self._currentEditNode = self.editNode(v)
                    self.setManipState(self._currentEditNode, "hover")
                    (con, gc, corner) = self.control(p["index"], event)
                    self._control = con
                    self._gc = gc
                    self._corner = corner

                    if self._control == TopRightCorner:
                        commands.setCursor(Qt.CursorShape.SizeBDiagCursor.value)
                    elif self._control == BotLeftCorner:
                        commands.setCursor(Qt.CursorShape.SizeBDiagCursor.value)
                    elif self._control == TopLeftCorner:
                        commands.setCursor(Qt.CursorShape.SizeFDiagCursor.value)
                    elif self._control == BotRightCorner:
                        commands.setCursor(Qt.CursorShape.SizeFDiagCursor.value)
                    elif self._control == FreeTranslation:
                        commands.setCursor(Qt.CursorShape.OpenHandCursor.value)
                    else:
                        commands.setCursor(Qt.CursorShape.WhatsThisCursor.value)
                    break

        if last is not self._currentEditNode:
            if last is not None:
                self.setManipState(last, "")
            commands.redraw()

        event.reject()

    def push(self, event):
        if self._currentEditNode is not None:
            commands.setCursor(Qt.CursorShape.ClosedHandCursor.value)
            self.setManipState(self._currentEditNode, "editing")

            if self.activeImageIndex() == -1:
                return

            self._downPoint = event.pointer()
            self._didDrag = False
            self._editing = True
            commands.redraw()

    def drag(self, event):
        if self._currentEditNode is not None:
            index = self.activeImageIndex()
            commands.setCursor(Qt.CursorShape.ClosedHandCursor.value)

            if index == -1:
                return

            tformNode = self._currentEditNode.tformNode
            inputNode = self._currentEditNode.inputNode
            transProp = "%s.transform.translate" % tformNode
            scaleProp = "%s.transform.scale" % tformNode
            trans = commands.getFloatProperty(transProp)
            scale = commands.getFloatProperty(scaleProp)
            corners = commands.imageGeometryByIndex(index)
            a = corners[0]
            b = corners[1]
            c = corners[2]
            d = corners[3]
            pp = event.pointer()
            dp = self._downPoint
            ip = _sub(pp, dp)
            ba = mag(_sub(b, a))
            da = mag(_sub(d, a))
            aspect = ba / da
            dx = ip[0] / ba * scale[0] * aspect
            dy = ip[1] / da * scale[1]

            if self._control == FreeTranslation:
                setFloatProp(transProp, [trans[0] + dx, trans[1] + dy])
            else:
                #
                #  The diagonal is only defined for a corner grab. control()
                #  returns (FreeTranslation, gc, gc) when the pointer is not near a
                #  corner, so _corner - _gc is exactly (0,0) on a free translation
                #  and normalize() would divide by zero — as would `/ downDist`
                #  right after, since a zero direction makes both distances 0.
                #
                #  Mu computes all of this unconditionally (transform_manip.mu:225)
                #  and gets away with it: its float division yields inf/nan instead
                #  of raising, and the FreeTranslation branch only reads dx/dy, so
                #  the nans are never used. Python raises, so the same values have
                #  to be computed where they are actually needed. Corner drags are
                #  unaffected either way.
                #
                diagDir = normalize(_sub(self._corner, self._gc))
                diagDist = dot(_sub(pp, self._gc), diagDir)
                downDist = dot(_sub(self._downPoint, self._gc), diagDir)
                diff = diagDist - downDist
                scl = (diagDist - diff / 2.0) / downDist
                sv = _scale(diagDir, diff)
                sdx = sv[0] / ba * scale[0] * aspect
                sdy = sv[1] / da * scale[1]

                setFloatProp(
                    transProp, [trans[0] + sdx / 2.0, trans[1] + sdy / 2.0]
                )
                newscale = max(scale[0] * scl, 0.01)
                setFloatProp(
                    scaleProp, [newscale, scale[1] * newscale / scale[0]]
                )

            self._downPoint = pp
            self._didDrag = True
            commands.redraw()

    def release(self, event):
        if self._editing:
            self.setManipState(self._currentEditNode, "hover")
            commands.setCursor(Qt.CursorShape.OpenHandCursor.value)
        else:
            commands.setCursor(Qt.CursorShape.ArrowCursor.value)

        self._didDrag = False
        self._editing = False

    def resetAll(self, event):
        for enode in self._editNodes:
            tformNode = enode.tformNode
            transProp = "%s.transform.translate" % tformNode
            scaleProp = "%s.transform.scale" % tformNode
            rotProp = "%s.transform.rotate" % tformNode

            setFloatProp(transProp, [0.0, 0.0])
            setFloatProp(scaleProp, [1.0, 1.0])
            setFloatProp(rotProp, [0.0])

        commands.redraw()

    def fitAll(self, event):
        aspect = nodeAspect(commands.viewNode())

        for enode in self._editNodes:
            tformNode = enode.tformNode
            transProp = "%s.transform.translate" % tformNode
            scaleProp = "%s.transform.scale" % tformNode
            rotProp = "%s.transform.rotate" % tformNode

            inaspect = nodeAspect(tformNode)
            s = aspect / inaspect

            setFloatProp(transProp, [0.0, 0.0])
            setFloatProp(scaleProp, [s, s])
            setFloatProp(rotProp, [0.0])

        commands.redraw()

    def removeTags(self):
        for x in self._editNodes:
            node = x.tformNode
            pmanip = node + ".tag.tmanip"
            pstate = node + ".tag.tmanip_state"

            for p in [pmanip, pstate]:
                if commands.propertyExists(p):
                    commands.deleteProperty(p)

    def findEditingNodes(self, setStates=True):
        infos = commands.metaEvaluateClosestByType(commands.frame(), "RVTransform2D")
        (ins, outs) = commands.nodeConnections(commands.viewNode(), False)

        self._editNodes = []

        # happens when shutting down or deletion
        if len(infos) != len(ins):
            return

        for i in range(len(infos)):
            info = infos[i]
            pname = info["node"] + ".tag.tmanip"
            sname = info["node"] + ".tag.tmanip_state"

            self._editNodes.append(EditNodePair(info["node"], ins[i]))

            if setStates or not commands.propertyExists(pname):
                setStringProp(pname, info["node"])
                setStringProp(sname, "")

    def nodeInputsChanged(self, event):
        node = event.contents()
        vnode = commands.viewNode()

        # Don't set the node states in this case
        if vnode is not None and node == vnode:
            self.findEditingNodes(False)

    def afterGraphViewChange(self, event):
        self.findEditingNodes()
        event.reject()

    def beforeGraphViewChange(self, event):
        self.removeTags()
        event.reject()

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)
        self.findEditingNodes()

    def deactivate(self):
        rv.rvtypes.MinorMode.deactivate(self)
        commands.setCursor(Qt.CursorShape.ArrowCursor.value)
        self.removeTags()

    def menu(self):
        return [
            (
                "Layout",
                [
                    ("_", None),
                    menuItem(
                        "Fit All Images",
                        "",
                        "viewmode_category",
                        self.fitAll,
                        lambda: commands.NeutralMenuState,
                    ),
                    menuItem(
                        "Reset All Manips",
                        "",
                        "viewmode_category",
                        self.resetAll,
                        lambda: commands.NeutralMenuState,
                    ),
                ],
            )
        ]

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        self._editNodes = []
        self._currentEditNode = None
        self._control = NoControl
        self._gc = (0.0, 0.0)
        self._corner = (0.0, 0.0)
        self._downPoint = (0.0, 0.0)

        self.init(
            "transform_manip",
            None,  # no global
            [
                ("pointer--move", self.move, "Search for Image"),
                ("pointer-1--push", self.push, "Grab Tile"),
                ("pointer-1--drag", self.drag, "Move/Scale Tile"),
                ("pointer-1--release", self.release, ""),
                (
                    "graph-node-inputs-changed",
                    self.nodeInputsChanged,
                    "Update session UI",
                ),
                ("after-graph-view-change", self.afterGraphViewChange, "Update UI"),
                ("before-graph-view-change", self.beforeGraphViewChange, "Update UI"),
                ("stylus-pen--move", self.move, "Search for Nearest Edge"),
                ("stylus-pen--push", self.push, "Move"),
                ("stylus-pen--drag", self.drag, "Move"),
                ("stylus-pen--release", self.release, ""),
            ],
            self.menu(),
            #
            #  manip events must be processed nearly last, since
            #  they cover the screen.
            #
            "zza",
        )

        self._editing = False
        self._didDrag = False

    def render(self, event):
        if self._currentEditNode is None:
            return

        domain = event.domain()
        bg = CONFIG_BG
        fg = CONFIG_FG
        index = self.activeImageIndex()

        if index == -1:
            return

        setupProjection(domain[0], domain[1], event.domainVerticalFlip())

        try:
            corners = commands.imageGeometryByIndex(index)
            gc = computeGC(corners)

            self._gc = gc
            glEnable(GL_BLEND)
            glEnable(GL_LINE_SMOOTH)
            glEnable(GL_POINT_SMOOTH)
            glLineWidth(2.0)

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

            def drawCorners(corners, mult, width):
                for i in range(len(corners)):
                    i0 = 3 if i == 0 else i - 1
                    i1 = (i + 1) % 4
                    c = corners[i]
                    c0 = corners[i0]
                    c1 = corners[i1]
                    m0 = mag(_sub(c0, c))
                    m1 = mag(_sub(c1, c))
                    dir0 = _scale(_sub(c0, c), 1.0 / m0)
                    dir1 = _scale(_sub(c1, c), 1.0 / m1)
                    nmult = 0.0 if (m1 / 2.0 < mult or m0 / 2.0 < mult) else mult

                    glBegin(GL_LINES)
                    _glVertex(_add(c, _scale(dir0, nmult)))
                    _glVertex(_sub(c, _scale(normalize(dir0), width)))
                    _glVertex(_add(c, _scale(dir1, nmult)))
                    _glVertex(_sub(c, _scale(normalize(dir1), width)))
                    glEnd()

            glColor4f(1.0, 1.0, 1.0, 0.5)
            glBegin(GL_LINE_LOOP)
            for c in corners:
                _glVertex(c)
            glEnd()

            glColor4f(0.0, 0.0, 0.0, 0.5)
            glLineWidth(8.0)
            drawCorners(corners, 25, 0.0)
            glLineWidth(6.0)
            glColor4f(1.0, 1.0, 1.0, 0.5)
            drawCorners(corners, 25, 0.0)

            glLineWidth(1.5)

            glPushMatrix()
            glTranslatef(gc[0], gc[1], 0.0)
            glScalef(25.0, 25.0, 25.0)
            glColor4f(bg[0], bg[1], bg[2], bg[3] * 0.5)
            circleGlyph(False)
            circleGlyph(True)
            glPopMatrix()

            glPushMatrix()
            glTranslatef(gc[0], gc[1], 0.0)
            glScalef(25.0, 25.0, 25.0)
            glColor4f(fg[0], fg[1], fg[2], fg[3])
            translateIconGlyph(False)
            glColor4f(fg[0] * 0.5, fg[1] * 0.5, fg[2] * 0.5, fg[3] * 0.5)
            glLineWidth(1.0)
            translateIconGlyph(True)
            glPopMatrix()

            glDisable(GL_BLEND)
        except Exception:
            # ignore it
            pass


def createMode():
    return TransformManip()
