#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from __future__ import print_function

from rv import commands as rvc
from rv import extra_commands as rve
from rv import rvtypes as rvt

import rv.runtime

import sys


def deb(s):
    if False:
        print("latlong: " + s + "\n", file=sys.stderr)


deb("loading module")


def colorPipeGroups():
    return rve.nodesInEvalPath(rvc.frame(), "RVColorPipelineGroup")


def groupHasViewer(group):
    return len(rve.nodesInGroupOfType(group, "LatLongViewer")) != 0


def viewerOfGroup(group):
    nodes = rve.nodesInGroupOfType(group, "LatLongViewer")
    node = None
    if len(nodes) != 0:
        node = nodes[0]
    return node


class LatLongViewerMode(rvt.MinorMode):
    def modeToggled(self, event):
        deb("local mode toggled: %s" % event.contents())
        event.reject()

        parts = event.contents().split("|")
        if len(parts) != 2 or parts[0] != "sync" or parts[1] != "true":
            return

        self._syncActivated = True
        deb("sync on!")

    def render(self, event):
        if self._syncActivated:
            deb("Adding regex to sync")
            self._syncActivated = False
            code = """
            {
                rvtypes.State s = commands.data();
                sync.RemoteSync mode = s.sync;
                mode.addSendPattern (regex("#LatLongViewer\\..*"));
            }"""
            rv.runtime.eval(code, ["sync", "rvtypes", "commands"])

    def readPrefs(self):
        self.adjustAllowed = bool(rvc.readSettings("lat_long_viewer", "adjustAllowed", True))

    def writePrefs(self):
        rvc.writeSettings("lat_long_viewer", "adjustAllowed", self.adjustAllowed)

    def addViewer(self, event):
        for pg in colorPipeGroups():
            deb("addViewer: checking g %s hasViewer %s" % (pg, str(groupHasViewer(pg))))

            if not groupHasViewer(pg):
                nodes = rvc.getStringProperty(pg + ".pipeline.nodes")
                nodes = ["LatLongViewer"] + nodes
                rvc.setStringProperty(pg + ".pipeline.nodes", nodes, True)

            vw = viewerOfGroup(pg)

            deb("new Viewer node: %s" % vw)

            rvc.setIntProperty(vw + ".node.active", [1], True)

            """
            if (self._swapEyes) :
                rvc.setIntProperty(vw + ".parameters.swapEyes", [ 1 ], True)
            else :
                rvc.setIntProperty(vw + ".parameters.swapEyes", [ 0 ], True)
            """

    def removeDisNode(self, event):
        for pg in linPipeGroups():
            deb("removeDisNode: checking g %s hasSD %s" % (pg, str(groupHasStereoDis(pg))))

            if groupHasStereoDis(pg):
                nodes = rvc.getStringProperty(pg + ".pipeline.nodes")
                newNodes = []
                for n in nodes:
                    if n != "StereoDisassembly":
                        newNodes.append(n)
                rvc.setStringProperty(pg + ".pipeline.nodes", newNodes, True)

    def toggleTopBotPref(self, event):
        self._expectTopBot = not self._expectTopBot
        self.writePrefs()

    def toggleSwapEyesPref(self, event):
        self._swapEyes = not self._swapEyes
        self.writePrefs()

    def disabledMenuState(self):
        return rvc.DisabledMenuState

    def uncheckedMenuState(self):
        return rvc.UncheckedMenuState

    def haveDisNodesState(self):
        for pg in linPipeGroups():
            if groupHasStereoDis(pg):
                return rvc.UncheckedMenuState

        return rvc.DisabledMenuState

    def expectTopBotState(self):
        if self._expectTopBot:
            return rvc.CheckedMenuState

        return rvc.UncheckedMenuState

    def swapEyesState(self):
        if self._swapEyes:
            return rvc.CheckedMenuState

        return rvc.UncheckedMenuState

    def addTopLevel(self, event):
        newViewer = rvc.newNode("LatLongViewer", "LatLong Viewer")
        rvc.setNodeInputs(newViewer, [rvc.viewNode()])
        rvc.setViewNode(newViewer)

    def toggleViewers(self, event):
        viewers = rve.nodesInEvalPath(rvc.frame(), "LatLongViewer")

        if len(viewers) == 0:
            return

        active = 0
        for v in viewers:
            if rvc.getIntProperty(v + ".node.active")[0] != 1:
                active = 1

        for v in viewers:
            rvc.setIntProperty(v + ".node.active", [active], True)

    def adjustFocalLength(self, event):
        rv.runtime.eval(
            'rvui.startParameterMode("#LatLongViewer.parameters.focalLength", 100.0, 7.0, 1.0)(nil);',
            [],
        )
        pass

    def toggleAdjust(self, event):
        self.adjustAllowed = not self.adjustAllowed
        self.writePrefs()

    def pointerPush(self, event):
        deb("pointerPush")
        deb("pointer %s" % str(event.pointer()))

        # leave the event for the pixel inspector
        if not self.adjustAllowed:
            event.reject()
            return

        viewers = rve.nodesInEvalPath(rvc.frame(), "LatLongViewer")
        if len(viewers) == 0:
            return

        p = event.pointer()
        self.downPointX = p[0]
        self.downPointY = p[1]

        self.downRotateX = rvc.getFloatProperty(viewers[0] + ".parameters.rotateX")[0]
        self.downRotateY = rvc.getFloatProperty(viewers[0] + ".parameters.rotateY")[0]

    def pointerDrag(self, event):
        deb("pointerDrag")
        deb("pointer %s" % str(event.pointer()))

        # leave the event for the pixel inspector
        if not self.adjustAllowed:
            event.reject()
            return

        viewers = rve.nodesInEvalPath(rvc.frame(), "LatLongViewer")
        if len(viewers) == 0:
            return

        p = event.pointer()
        dx = p[0] - self.downPointX
        dy = p[1] - self.downPointY

        domain = event.domain()
        dx = dx / domain[0]
        dy = dy / domain[1]
        deb("dx %s dy %s" % (str(dx), str(dy)))

        rvc.setFloatProperty("#LatLongViewer.parameters.rotateX", [self.downRotateX + -dy * 180.0], True)
        rvc.setFloatProperty("#LatLongViewer.parameters.rotateY", [self.downRotateY + dx * 360.0], True)

    def missingState(self):
        for pg in colorPipeGroups():
            deb("checking g %s hasViewer %s" % (pg, str(groupHasViewer(pg))))

            if not groupHasViewer(pg):
                return rvc.UncheckedMenuState

        return rvc.DisabledMenuState

    def haveSourcesState(self):
        if len(rvc.sources()) > 0:
            return rvc.UncheckedMenuState
        else:
            return rvc.DisabledMenuState

    def haveInactiveViewerState(self):
        viewers = rve.nodesInEvalPath(rvc.frame(), "LatLongViewer")

        if len(viewers) == 0:
            return rvc.DisabledMenuState

        for v in viewers:
            if rvc.getIntProperty(v + ".node.active")[0] != 1:
                return rvc.UncheckedMenuState

        return rvc.CheckedMenuState

    def haveViewerState(self):
        viewers = rve.nodesInEvalPath(rvc.frame(), "LatLongViewer")

        if len(viewers) == 0:
            return rvc.DisabledMenuState

        return rvc.UncheckedMenuState

    def adjustAllowedState(self):
        if self.adjustAllowed:
            return rvc.CheckedMenuState

        return rvc.UncheckedMenuState

    def __init__(self):
        deb("__init__")
        rvt.MinorMode.__init__(self)

        self.adjustAllowed = True

        self.readPrefs()

        self.downPointX = 0.0
        self.downPointY = 0.0
        self.downRotateX = 0.0
        self.downRotateY = 0.0

        self.init(
            "lat_long_viewer_mode",
            [
                ("mode-toggled", self.modeToggled, ""),
                (
                    "stylus-pen--shift--push",
                    self.pointerPush,
                    "Start LatLong View Adjust",
                ),
                ("stylus-pen--shift--drag", self.pointerDrag, "LatLong View Adjust"),
                (
                    "pointer-1--shift--push",
                    self.pointerPush,
                    "Start LatLong View Adjust",
                ),
                ("pointer-1--shift--drag", self.pointerDrag, "LatLong View Adjust"),
            ],
            None,
            [
                (
                    "Image",
                    [
                        ("_", None, None, None),
                        ("Lat-Long Viewer", None, None, self.disabledMenuState),
                        ("    Add To Source", self.addViewer, None, self.missingState),
                        (
                            "    Add As Top-Level View",
                            self.addTopLevel,
                            None,
                            self.haveSourcesState,
                        ),
                        (
                            "    Enabled",
                            self.toggleViewers,
                            None,
                            self.haveInactiveViewerState,
                        ),
                        (
                            "    Adjust Focal Length",
                            self.adjustFocalLength,
                            None,
                            self.haveViewerState,
                        ),
                        (
                            "    Adjust View with Shift-Drag",
                            self.toggleAdjust,
                            None,
                            self.adjustAllowedState,
                        ),
                    ],
                )
            ],
        )

        deb("__init__ done")
        self._syncActivated = False


def createMode():
    return LatLongViewerMode()
