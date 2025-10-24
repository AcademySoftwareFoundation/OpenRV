#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import rvtypes, commands


def groupMemberOfType(node, memberType):
    for n in commands.nodesInGroup(node):
        if commands.nodeType(n) == memberType:
            return n
    return None


def isCollapsed(retime):
    prop = retime + ".explicit.active"
    return commands.propertyExists(prop) and commands.getIntProperty(prop)[0] == 1


class CollapseMissingFrames(rvtypes.MinorMode):
    def collapseInView(self, event):
        sources = commands.sourcesAtFrame(commands.frame())
        for source in sources:
            self.collapse(source)

    def collapseAll(self, event):
        if self.toggleCollapseAll():
            sources = commands.nodesOfType("RVFileSource")
            for source in sources:
                self.collapse(source, True)

    def collapseMaybe(self, event):
        event.reject()
        if self._collapseAll:
            group = event.contents().split(";")[0]
            source = groupMemberOfType(group, "RVFileSource")
            self.collapse(source, True)

    def collapse(self, source, force=False):
        linPG = groupMemberOfType(commands.nodeGroup(source), "RVLinearizePipelineGroup")
        retime = groupMemberOfType(linPG, "RVRetime")
        if not force and retime is not None and isCollapsed(retime):
            commands.setIntProperty(retime + ".explicit.active", [0])
        else:
            media = commands.getStringProperty(source + ".media.movie")[0]
            if commands.fileKind(media) == commands.ImageFileKind:
                if retime is None:
                    nodes = commands.getStringProperty(linPG + ".pipeline.nodes")
                    nodes += ["RVRetime"]
                    commands.setStringProperty(linPG + ".pipeline.nodes", nodes, True)
                    retime = groupMemberOfType(linPG, "RVRetime")
                else:
                    commands.setIntProperty(retime + ".explicit.active", [0])
                rangeInfo = commands.nodeRangeInfo(source)
                fullset = int(rangeInfo["end"]) - int(rangeInfo["start"]) + 1
                partial = commands.existingFramesInSequence(media)
                if force or len(partial) != fullset:
                    commands.setIntProperty(
                        retime + ".explicit.firstOutputFrame",
                        [int(rangeInfo["start"])],
                        True,
                    )
                    commands.setIntProperty(retime + ".explicit.inputFrames", partial, True)
                    commands.setIntProperty(retime + ".explicit.active", [1])

    def collapsed(self):
        sources = commands.sourcesAtFrame(commands.frame())
        for source in sources:
            linPG = groupMemberOfType(commands.nodeGroup(source), "RVLinearizePipelineGroup")
            retime = groupMemberOfType(linPG, "RVRetime")
            if retime is None or not isCollapsed(retime):
                return commands.UncheckedMenuState
        return commands.CheckedMenuState

    def allCollapsed(self):
        if self._collapseAll:
            return commands.CheckedMenuState
        return commands.UncheckedMenuState

    def toggleCollapseAll(self):
        self._collapseAll = not self._collapseAll
        commands.writeSettings("COLLAPSE_MISSING", "collapseAll", self._collapseAll)
        return self._collapseAll

    def __init__(self):
        rvtypes.MinorMode.__init__(self)

        self.init(
            "collapse_missing_frames",
            [("source-group-complete", self.collapseMaybe, "Source Color Setup")],
            None,
            [
                (
                    "Image",
                    [
                        ("_", None),
                        (
                            "Collapse Image Sequence",
                            self.collapseInView,
                            "",
                            self.collapsed,
                        ),
                        (
                            "Collapse All Image Sequences",
                            self.collapseAll,
                            "",
                            self.allCollapsed,
                        ),
                    ],
                )
            ],
        )

        self._collapseAll = commands.readSettings("COLLAPSE_MISSING", "collapseAll", False)


def createMode():
    return CollapseMissingFrames()
