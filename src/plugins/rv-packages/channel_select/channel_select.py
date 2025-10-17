#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import commands as rvc
from rv import extra_commands as rve
from rv import rvtypes as rvt


def getColorSelectNodes():
    try:
        return rvc.nodesOfType("ChannelSelect")
    except Exception:
        pass

    return None


def channelIs(node, num):
    active = rvc.getIntProperty(node + ".node.active")[0]
    chan = rvc.getIntProperty(node + ".parameters.channel")[0]

    return active and (chan == num)


class ColorSelectMode(rvt.MinorMode):
    def readPrefs(self):
        self._postDisplaySelect = bool(rvc.readSettings("channel_select", "postDisplaySelect", False))

    def writePrefs(self):
        rvc.writeSettings("channel_select", "postDisplaySelect", self._postDisplaySelect)

    def addChannelSelect(self):
        if self._postDisplaySelect:
            groups = []
            for dg in rvc.nodesOfType("RVDisplayGroup"):
                pgs = rve.nodesInGroupOfType(dg, "RVDisplayPipelineGroup")
                if pgs:
                    groups.append(pgs[0])
        else:
            groups = rvc.nodesOfType("RVViewPipelineGroup")

        for g in groups:
            nodes = rvc.getStringProperty(g + ".pipeline.nodes")

            if "ChannelSelect" not in nodes:
                nodes += ["ChannelSelect"]
                rvc.setStringProperty(g + ".pipeline.nodes", nodes, True)

                #
                #  When we add a node, we want it to be inactive.  Inactive shading
                #  nodes have no performance impact on rendering.
                #
                n = rve.nodesInGroupOfType(g, "ChannelSelect")[0]
                rvc.setIntProperty(n + ".node.active", [0], True)

    def toggleChannel(self, num):
        def foo(event):
            self.addChannelSelect()

            nodes = getColorSelectNodes()
            if not nodes:
                return

            for n in nodes:
                if num == 4 or channelIs(n, num):
                    #
                    #  Go back to full color display
                    #
                    rvc.setIntProperty(n + ".node.active", [0], True)

                else:
                    rvc.setIntProperty(n + ".node.active", [1], True)
                    rvc.setIntProperty(n + ".parameters.channel", [num], True)

            rvc.redraw()

        return foo

    def togglePostDisplaySelect(self, event):
        self._postDisplaySelect = not self._postDisplaySelect
        self.writePrefs()

    def postDisplaySelectState(self):
        if self._postDisplaySelect:
            return rvc.CheckedMenuState
        return rvc.UncheckedMenuState

    def __init__(self):
        rvt.MinorMode.__init__(self)

        self.readPrefs()

        self.init(
            "color_select_mode",
            [
                ("key-down--r", self.toggleChannel(0), "select red channel"),
                ("key-down--g", self.toggleChannel(1), "select green channel"),
                ("key-down--b", self.toggleChannel(2), "select blue channel"),
                ("key-down--a", self.toggleChannel(3), "select alpha channel"),
                ("key-down--c", self.toggleChannel(4), "all channels"),
                ("key-down--l", self.toggleChannel(5), "show luminance"),
            ],
            None,
            [
                (
                    "View",
                    [
                        (
                            "Post-Display Channel Selection",
                            self.togglePostDisplaySelect,
                            None,
                            self.postDisplaySelectState,
                        )
                    ],
                )
            ],
        )


def createMode():
    return ColorSelectMode()
