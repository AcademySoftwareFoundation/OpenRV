#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import rvtypes, commands

import platform
import re
from six.moves.urllib.parse import urlparse

#
# Per-pipeline-group snapshot of the "pipeline.nodes" value taken just before
# SYN nodes were inserted, keyed by the pipeline group node name. Used to
# restore the original pipeline when the feature is disabled.
#
DEFAULT_PIPE = {}

#
# The native RV defaults, used as the restore target when we are reloading a
# session whose pipeline is already SYN managed (so the persisted SYN pipeline
# is not mistaken for the user's "default" pipeline).
#
DEFAULT_RV_PIPE = {
    "RVLinearizePipelineGroup": ["RVLinearize", "RVLensWarp"],
    "RVDisplayPipelineGroup": ["RVDisplayColor"],
}

# Source data attribute that holds an embedded ICC profile blob.
ICC_DATA_KEY = "ColorSpace/ICC/Data"

# Self-contained, package-owned preference (off by default). Stored in this
# package's own QSettings group so the feature needs no entry in RV's
# Preferences dialog.
SETTINGS_GROUP = "color_profile_setup"
SETTINGS_KEY = "enabled"


def groupMemberOfType(node, memberType):
    for n in commands.nodesInGroup(node):
        if commands.nodeType(n) == memberType:
            return n
    return None


def normalizeProfileURL(url):
    #
    # device.systemProfileURL is a file:// URL. Convert it to a filesystem
    # path the same way source_setup.py does for ICCDisplayTransform.
    #
    path = urlparse(url.replace("%20", " ")).path
    if "windows" in platform.platform().lower() and re.match("^/.:.*$", path):
        path = path[1:]
    return path


class ColorProfileSetupMode(rvtypes.MinorMode):
    """
    Automatically inserts the SYNLinearize and SYNDisplay OCIO nodes that
    previously had to be inserted by hand via a key-bound script.

    Source side:
        When media carries an embedded ICC profile (a data attribute), the
        source's RVLinearizePipelineGroup is switched to a single SYNLinearize
        node whose inTransform.data is set to the profile blob. Because the
        source group is persisted to .rv, this carries over to rvio for free.

    Display side:
        For each RVDisplayGroup whose device has a system ICC profile, the
        display pipeline is switched to a single SYNDisplay node whose
        outTransform.url points at the profile.

    rvio parity:
        RVDisplayGroup is NOT written to a session file and rvio renders
        through RVOutputGroup instead. So at session-write time the display
        SYNDisplay transform is baked into every RVOutputGroup's display
        pipeline (which IS persisted), then reverted after the write so the
        interactive graph is left unchanged.

    The feature is opt-in and self-contained: it is controlled by the
    "Automatic Color Profile Setup" checkbox in this package's own menu,
    backed by a package-owned preference (off by default). No entry is added
    to RV's Preferences dialog.

    ORDERING: sort key "source_setup", ordering 20 -- runs after the base
    source_setup (0) and ocio_source_setup (10).
    """

    #
    #   Source side
    #

    def _iccBlob(self, source):
        try:
            medias = commands.getStringProperty("%s.media.movie" % source)
            media = medias[0] if medias else ""
            data = commands.sourceDataAttributes(source, media)
        except Exception:
            return None

        if not data:
            return None

        dataDict = dict((name, blob) for (name, blob) in data)
        blob = dataDict.get(ICC_DATA_KEY)
        if blob is None:
            #
            # Fall back to the first data attribute. This mirrors the original
            # manually-bound script, which used attrs[0] regardless of name.
            #
            blob = data[0][1]
        if blob is None or len(blob) == 0:
            return None
        return blob

    def useSourceSYN(self, source):
        linPipe = groupMemberOfType(commands.nodeGroup(source), "RVLinearizePipelineGroup")
        if linPipe is None:
            return

        blob = self._iccBlob(source)
        if blob is None:
            # No embedded ICC profile: leave / restore the default pipeline.
            self.disableSourceSYN(source)
            return

        current = commands.getStringProperty(linPipe + ".pipeline.nodes")
        if linPipe not in DEFAULT_PIPE:
            if "SYNLinearize" in current:
                DEFAULT_PIPE[linPipe] = DEFAULT_RV_PIPE["RVLinearizePipelineGroup"]
            else:
                DEFAULT_PIPE[linPipe] = current

        # Match the original script: the linearize pipeline becomes just
        # SYNLinearize.
        if current != ["SYNLinearize"]:
            commands.setStringProperty(linPipe + ".pipeline.nodes", ["SYNLinearize"], True)

        syn = groupMemberOfType(linPipe, "SYNLinearize")
        if syn is not None:
            commands.setByteProperty(syn + ".inTransform.data", blob, True)
            print("INFO: SYNLinearize applied to %s" % source)
        commands.redraw()

    def disableSourceSYN(self, source):
        linPipe = groupMemberOfType(commands.nodeGroup(source), "RVLinearizePipelineGroup")
        if linPipe is None or linPipe not in DEFAULT_PIPE:
            return
        current = commands.getStringProperty(linPipe + ".pipeline.nodes")
        if current == DEFAULT_PIPE[linPipe]:
            return
        commands.setStringProperty(linPipe + ".pipeline.nodes", DEFAULT_PIPE[linPipe], True)
        commands.redraw()

    #
    #   Display side
    #

    def useDisplaySYN(self, group):
        url = commands.getStringProperty(group + ".device.systemProfileURL")
        if not url or url[0] == "":
            self.disableDisplaySYN(group)
            return

        dpipe = groupMemberOfType(group, "RVDisplayPipelineGroup")
        if dpipe is None:
            return

        path = normalizeProfileURL(url[0])
        current = commands.getStringProperty(dpipe + ".pipeline.nodes")

        # Already configured with this profile: nothing to do.
        if current == ["SYNDisplay"]:
            syn = groupMemberOfType(dpipe, "SYNDisplay")
            if syn is not None and commands.getStringProperty(syn + ".outTransform.url") == [path]:
                return

        if dpipe not in DEFAULT_PIPE:
            if "SYNDisplay" in current:
                DEFAULT_PIPE[dpipe] = DEFAULT_RV_PIPE["RVDisplayPipelineGroup"]
            else:
                DEFAULT_PIPE[dpipe] = current

        if current != ["SYNDisplay"]:
            commands.setStringProperty(dpipe + ".pipeline.nodes", ["SYNDisplay"], True)

        syn = groupMemberOfType(dpipe, "SYNDisplay")
        if syn is not None:
            commands.setStringProperty(syn + ".outTransform.url", [path], True)
            print("INFO: SYNDisplay applied to %s" % group)
        commands.redraw()

    def disableDisplaySYN(self, group):
        dpipe = groupMemberOfType(group, "RVDisplayPipelineGroup")
        if dpipe is None or dpipe not in DEFAULT_PIPE:
            return
        current = commands.getStringProperty(dpipe + ".pipeline.nodes")
        if current == DEFAULT_PIPE[dpipe]:
            return
        commands.setStringProperty(dpipe + ".pipeline.nodes", DEFAULT_PIPE[dpipe], True)
        commands.redraw()

    #
    #   Whole-graph helpers
    #

    def _sources(self):
        result = []
        for t in ("RVFileSource", "RVImageSource"):
            result += commands.nodesOfType(t)
        return result

    def _applyDisplays(self):
        for group in commands.nodesOfType("RVDisplayGroup"):
            self.useDisplaySYN(group)

    def applyAll(self):
        for source in self._sources():
            self.useSourceSYN(source)
        self._applyDisplays()

    def restoreAll(self):
        for source in self._sources():
            self.disableSourceSYN(source)
        for group in commands.nodesOfType("RVDisplayGroup"):
            self.disableDisplaySYN(group)

    def _currentDisplayProfilePath(self):
        #
        # Pick a display profile to bake into the output group(s). Prefer the
        # first display with a non-empty system profile.
        #
        for group in commands.nodesOfType("RVDisplayGroup"):
            url = commands.getStringProperty(group + ".device.systemProfileURL")
            if url and url[0] != "":
                return normalizeProfileURL(url[0])
        return None

    #
    #   Event handlers
    #

    def sourceSetup(self, event):
        event.reject()  # allow other source-group-complete handlers to run
        if not self._enabled:
            return
        group = event.contents().split(";;")[0]
        fileSource = groupMemberOfType(group, "RVFileSource")
        imageSource = groupMemberOfType(group, "RVImageSource")
        source = fileSource if imageSource is None else imageSource
        if source is not None:
            self.useSourceSYN(source)
        # A source's embedded profile does not drive the display; the display
        # is configured from its own device system profile.
        self._applyDisplays()

    def checkForDisplayGroup(self, event):
        event.reject()
        if not self._enabled:
            return
        try:
            node = event.contents()
            if commands.nodeType(node) == "RVDisplayGroup":
                self.useDisplaySYN(node)
        except Exception:
            pass

    def beforeSessionRead(self, event):
        event.reject()
        self._readingSession = True

    def afterSessionRead(self, event):
        event.reject()
        self._readingSession = False
        if self._enabled:
            self._applyDisplays()

    def beforeSessionWrite(self, event):
        #
        # rvio parity: bake the display SYNDisplay transform into every output
        # group so the exported .rv reproduces it under rvio (RVDisplayGroup is
        # not persisted; RVOutputGroup is, and rvio renders through it).
        #
        event.reject()
        self._bakedOutputGroups = {}
        if not self._enabled:
            return
        path = self._currentDisplayProfilePath()
        if path is None:
            return
        for og in commands.nodesOfType("RVOutputGroup"):
            dpipe = groupMemberOfType(og, "RVDisplayPipelineGroup")
            if dpipe is None:
                continue
            self._bakedOutputGroups[dpipe] = commands.getStringProperty(dpipe + ".pipeline.nodes")
            if "SYNDisplay" not in self._bakedOutputGroups[dpipe]:
                commands.setStringProperty(dpipe + ".pipeline.nodes", ["SYNDisplay"], True)
            syn = groupMemberOfType(dpipe, "SYNDisplay")
            if syn is not None:
                commands.setStringProperty(syn + ".outTransform.url", [path], True)
                print("INFO: SYNDisplay baked into %s for rvio parity" % og)

    def afterSessionWrite(self, event):
        #
        # Revert the output groups to their pre-write state so the parity bake
        # does not alter the live interactive graph.
        #
        event.reject()
        for dpipe, nodes in self._bakedOutputGroups.items():
            try:
                current = commands.getStringProperty(dpipe + ".pipeline.nodes")
                if current != nodes:
                    commands.setStringProperty(dpipe + ".pipeline.nodes", nodes, True)
            except Exception:
                pass
        self._bakedOutputGroups = {}

    #
    #   Menu toggle (self-contained, package-owned preference)
    #

    def toggleEnabled(self, event):
        self._enabled = not self._enabled
        commands.writeSettings(SETTINGS_GROUP, SETTINGS_KEY, self._enabled)
        if self._enabled:
            self.applyAll()
        else:
            self.restoreAll()
        commands.redraw()

    def enabledState(self):
        return commands.CheckedMenuState if self._enabled else commands.UncheckedMenuState

    def _buildMenu(self):
        return [
            (
                "Color",
                [
                    (
                        "Automatic Color Profile Setup",
                        self.toggleEnabled,
                        None,
                        self.enabledState,
                    )
                ],
            )
        ]

    def __init__(self):
        rvtypes.MinorMode.__init__(self)

        self._enabled = bool(commands.readSettings(SETTINGS_GROUP, SETTINGS_KEY, False))
        self._readingSession = False
        self._bakedOutputGroups = {}

        self.init(
            "color-profile-setup",
            None,
            [
                ("source-group-complete", self.sourceSetup, "Color and Geometry Management"),
                ("before-session-read", self.beforeSessionRead, ""),
                ("after-session-read", self.afterSessionRead, ""),
                ("graph-new-node", self.checkForDisplayGroup, ""),
                ("graph-node-inputs-changed", self.checkForDisplayGroup, ""),
                ("before-session-write", self.beforeSessionWrite, ""),
                ("before-session-write-copy", self.beforeSessionWrite, ""),
                ("after-session-write", self.afterSessionWrite, ""),
                ("after-session-write-copy", self.afterSessionWrite, ""),
            ],
            self._buildMenu(),
            "source_setup",
            20,
        )  # "source_setup" key shared with source_setup and ocio_source_setup


def createMode():
    return ColorProfileSetupMode()
