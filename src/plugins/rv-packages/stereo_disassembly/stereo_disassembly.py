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

import os
import sys
import re


def deb(s):
    if False:
        print("stereoDis: " + s + "\n", file=sys.stderr)


deb("loading module")


def linPipeGroups():
    return rve.nodesInEvalPath(rvc.frame(), "RVLinearizePipelineGroup")


def groupHasStereoDis(group):
    return stereoDisOfGroup(group) is not None


def stereoDisOfGroup(group):
    for node in rvc.nodesInGroup(group):
        if rvc.nodeType(node).startswith("StereoDisassembly"):
            return node
    return None


class StereoDisMode(rvt.MinorMode):
    def sdNodeType(self):
        nodeType = "StereoDisassembly"
        nodeType += "FramePacked" if not self._squeezed else ""
        return nodeType

    def readPrefs(self):

        self._expectTopBot = rvc.readSettings(
            "stereo_disassembly", "expectTopBot", True
        )
        self._swapEyes = rvc.readSettings("stereo_disassembly", "swapEyes", True)
        self._squeezed = rvc.readSettings("stereo_disassembly", "squeezed", True)

    def writePrefs(self):

        rvc.writeSettings("stereo_disassembly", "expectTopBot", self._expectTopBot)
        rvc.writeSettings("stereo_disassembly", "swapEyes", self._swapEyes)
        rvc.writeSettings("stereo_disassembly", "squeezed", self._squeezed)

    def addDisNode(self, event):

        for pg in linPipeGroups():

            deb("addDisNode: checking g %s hasSD %s" % (pg, str(groupHasStereoDis(pg))))

            if not groupHasStereoDis(pg):
                nodes = rvc.getStringProperty(pg + ".pipeline.nodes")
                nodes.append(self.sdNodeType())
                rvc.setStringProperty(pg + ".pipeline.nodes", nodes, True)

            sd = stereoDisOfGroup(pg)

            deb("new SD node: %s" % sd)

            if self._expectTopBot:
                rvc.setIntProperty(sd + ".parameters.topBottom", [1], True)
            else:
                rvc.setIntProperty(sd + ".parameters.topBottom", [0], True)

            if self._swapEyes:
                rvc.setIntProperty(sd + ".parameters.swapEyes", [1], True)
            else:
                rvc.setIntProperty(sd + ".parameters.swapEyes", [0], True)

            if not self._squeezed:
                sg = rvc.nodeGroup(pg)
                src = None
                for node in rvc.nodesInGroup(sg):
                    if rvc.nodeType(node) in ["RVFileSource", "RVImageSource"]:
                        src = node
                        break
                if src is not None:
                    info = rvc.sourceMediaInfo(src, None)
                    size = [info["uncropWidth"], info["uncropHeight"]]
                    if self._expectTopBot:
                        size = [size[0], size[1] / 2]
                    else:
                        size = [size[0] / 2, size[1]]
                    rvc.setIntProperty(sd + ".output.autoSize", [0], True)
                    rvc.setIntProperty(sd + ".output.size", size, True)

    def removeDisNode(self, event):

        for pg in linPipeGroups():

            deb(
                "removeDisNode: checking g %s hasSD %s"
                % (pg, str(groupHasStereoDis(pg)))
            )

            if groupHasStereoDis(pg):
                sd = stereoDisOfGroup(pg)
                nodes = rvc.getStringProperty(pg + ".pipeline.nodes")
                newNodes = []
                for n in nodes:
                    if n != rvc.nodeType(sd):
                        newNodes.append(n)
                rvc.setStringProperty(pg + ".pipeline.nodes", newNodes, True)

    def toggleTopBotPref(self, event):

        self._expectTopBot = not self._expectTopBot
        self.writePrefs()

    def toggleSwapEyesPref(self, event):

        self._swapEyes = not self._swapEyes
        self.writePrefs()

    def toggleSqueezed(self, event):

        self._squeezed = not self._squeezed
        self.writePrefs()

    def disabledMenuState(self):

        return rvc.DisabledMenuState

    def noDisNodesState(self):

        for pg in linPipeGroups():

            deb("checking g %s hasSD %s" % (pg, str(groupHasStereoDis(pg))))

            if not groupHasStereoDis(pg):
                return rvc.UncheckedMenuState

        return rvc.DisabledMenuState

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

    def squeezedState(self):

        if not self._squeezed:
            return rvc.CheckedMenuState

        return rvc.UncheckedMenuState

    def __init__(self):

        deb("__init__")
        rvt.MinorMode.__init__(self)

        self._expectTopBot = True
        self._swapEyes = False
        self._squeezed = True

        self.readPrefs()

        self.init(
            "stereo_disassembly_mode",
            None,
            None,
            [
                (
                    "Image",
                    [
                        (
                            "Stereo",
                            [
                                ("_", None, None, None),
                                ("Disassembly", None, None, self.disabledMenuState),
                                (
                                    "    Add Disassembly Node",
                                    self.addDisNode,
                                    None,
                                    self.noDisNodesState,
                                ),
                                (
                                    "    Remove Disassembly Node",
                                    self.removeDisNode,
                                    None,
                                    self.haveDisNodesState,
                                ),
                                (
                                    "    Expect Top/Bottom By Default",
                                    self.toggleTopBotPref,
                                    None,
                                    self.expectTopBotState,
                                ),
                                (
                                    "    Expect Frame Packed By Default",
                                    self.toggleSqueezed,
                                    None,
                                    self.squeezedState,
                                ),
                                (
                                    "    Swap Eyes By Default",
                                    self.toggleSwapEyesPref,
                                    None,
                                    self.swapEyesState,
                                ),
                            ],
                        ),
                    ],
                )
            ],
        )

        deb("__init__ done")


def createMode():

    return StereoDisMode()
