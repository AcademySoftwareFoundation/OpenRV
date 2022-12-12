#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from __future__ import print_function

from rv import commands, extra_commands, rvtypes

import os
import sys
import re


def deb(s):
    if False:
        print("stereo_autoload: %s\n" % s, file=sys.stderr)


class StereoAutoloadMode(rvtypes.MinorMode):
    def readPrefs(self):
        deb(
            "readSettings %s %s\n"
            % (
                str(commands.readSettings("stereo_autoload", "autoStereo", False)),
                str(commands.readSettings("stereo_autoload", "autoAlways", False)),
            )
        )

        self._autoLoadStereo = bool(
            commands.readSettings("stereo_autoload", "autoStereo", False)
        )
        self._autoLoadAlways = bool(
            commands.readSettings("stereo_autoload", "autoAlways", False)
        )

        deb(
            "    after read: %s %s\n"
            % (str(self._autoLoadStereo), str(self._autoLoadAlways))
        )

    def writePrefs(self):
        deb("writing settings %s %s\n" % (self._autoLoadStereo, self._autoLoadAlways))
        commands.writeSettings("stereo_autoload", "autoStereo", self._autoLoadStereo)
        commands.writeSettings("stereo_autoload", "autoAlways", self._autoLoadAlways)

    def toggleAutoLoadStereo(self, event):
        self._autoLoadStereo = not self._autoLoadStereo
        self.writePrefs()
        if self._autoLoadStereo and self.stereoOn():
            self.updateAllSources()

    def toggleAutoLoadAlways(self, event):
        self._autoLoadAlways = not self._autoLoadAlways
        self.writePrefs()
        if self._autoLoadAlways:
            self.updateAllSources()

    def autoLoadStereoState(self):
        if self._autoLoadStereo:
            return commands.CheckedMenuState
        return commands.UncheckedMenuState

    def autoLoadAlwaysState(self):
        if self._autoLoadAlways:
            return commands.CheckedMenuState
        return commands.UncheckedMenuState

    def updatePairsFromEnv(self, pairs, varName):
        #
        # Check env var for replacement pairs of strings
        # designating left/right eyes
        #
        val = os.getenv(varName)
        if val:
            elems = val.split(":")
            if len(elems) % 2 != 0:
                print(
                    "ERROR: eyes env var '%s' does not have even number of elements.\n"
                    % varName,
                    file=sys.stderr,
                )
                return
            #
            # don't assign to "pairs" since that will make new local var
            #
            del pairs[:]
            for i in range(0, len(elems), 2):
                pairs.append((elems[i], elems[i + 1]))

    def initStereoPairs(self):
        #
        # Setup pairs of strings designating left/right eyes
        #
        self._namePairs = [
            ("left", "right"),
            ("Left", "Right"),
            ("LEFT", "RIGHT"),
            ("le", "re"),
            ("LE", "RE"),
        ]
        self._charPairs = [("l", "r"), ("L", "R")]

        self.updatePairsFromEnv(self._namePairs, "RV_STEREO_NAME_PAIRS")
        self.updatePairsFromEnv(self._charPairs, "RV_STEREO_CHAR_PAIRS")

        self._pairs = self._namePairs + self._charPairs

    def countViews(self, sourceNode):
        #
        # Check current media, count views to see if
        # we already have both eyes.
        #
        nviews = 0
        leftMedia = None

        mediaInfos = commands.sourceMediaInfoList(sourceNode)
        for i in mediaInfos:
            if i["hasVideo"]:
                if not leftMedia:
                    leftMedia = i["file"]
                if len(i["viewInfos"]) > 1:
                    nviews += 2
                else:
                    nviews += 1

        return (nviews, leftMedia)

    def swapLeftForRight(self, leftMedia):

        rightMedia = leftMedia
        for pair in self._pairs:
            rightMedia = re.sub(
                "(^|[._/,\- ])%s([._/,\- ])" % pair[0],
                "\g<1>%s\g<2>" % pair[1],
                rightMedia,
            )

        if rightMedia == leftMedia:
            return None

        ret = [rightMedia]

        #
        # Add lower case targets
        #
        rightMedia = leftMedia
        for pair in self._pairs:
            rightMedia = re.sub(
                "(^|[._/,\- ])%s([._/,\- ])" % pair[0],
                "\g<1>%s\g<2>" % pair[1].lower(),
                rightMedia,
            )

        if not rightMedia in ret:
            ret.append(rightMedia)

        #
        # Add upper case targets
        #
        rightMedia = leftMedia
        for pair in self._pairs:
            rightMedia = re.sub(
                "(^|[._/,\- ])%s([._/,\- ])" % pair[0],
                "\g<1>%s\g<2>" % pair[1].upper(),
                rightMedia,
            )

        if not rightMedia in ret:
            ret.append(rightMedia)

        return ret

    def updateSource(self, sourceNode, action):
        deb("updateSource %s %s" % (sourceNode, action))
        if not commands.nodeExists(sourceNode):
            return

        (numViews, leftMedia) = self.countViews(sourceNode)

        if numViews > 1 or not leftMedia:
            #
            # Already has both eyes, or is audio-only source
            #
            return

        #
        # Find potential right eye media
        #
        rightMediaList = self.swapLeftForRight(leftMedia)
        if not rightMediaList:
            print(
                "INFO: Unable to find right eye media for '%s'\n" % leftMedia,
                file=sys.stderr,
            )
            return

        #
        # Test that it actually exists
        #
        rightMedia = None
        for m in rightMediaList:
            if len(commands.existingFilesInSequence(m)) != 0:
                rightMedia = m
                break

        if not rightMedia:
            for m in rightMediaList:
                print(
                    "INFO: Right eye media '%s' does not exist\n" % m, file=sys.stderr
                )
            return

        #
        # Add right eye media to source
        #
        print("INFO: Adding right eye '%s'\n" % rightMedia, file=sys.stderr)
        media = commands.getStringProperty(sourceNode + ".media.movie", 0, 10)
        media.append(rightMedia)
        commands.setSourceMedia(sourceNode, media, "")

    def updateAllSources(self, event=None):
        for s in commands.nodesOfType("RVFileSource"):
            self.updateSource(s, "stereo on")

    def updateVisibleSources(self, event=None):
        for s in extra_commands.nodesInEvalPath(commands.frame(), "RVFileSource", None):
            self.updateSource(s, "force")

    def viewStereoOn(self):
        return (
            "off" != commands.getStringProperty("#RVDisplayStereo.stereo.type", 0, 1)[0]
        )

    def presentationStereoOn(self):
        s = commands.videoState()

        return s and "Stereo" in s["dataFormat"]

    def stereoOn(self):
        return self.viewStereoOn() or self.presentationStereoOn()

    def sourceCompleteCallback(self, event):
        event.reject()
        deb(
            "sourceComplete %s auto stereo %s auto always %s stereo %s\n"
            % (
                event.contents(),
                self._autoLoadStereo,
                self._autoLoadAlways,
                self.stereoOn(),
            )
        )
        if not self._autoLoadAlways and not (self._autoLoadStereo and self.stereoOn()):
            return

        contentElems = event.contents().split(";;")
        if len(contentElems) != 2:
            return

        (sourceName, action) = tuple(contentElems)
        if action != "session":
            self.updateSource(sourceName + "_source", action)

    def stateChangeCallback(self, event):
        event.reject()
        if event.contents() != "displayGroup_stereo.stereo.type":
            return

        deb(
            "stateChange %s auto stereo %s auto always %s stereo %s\n"
            % (
                event.contents(),
                self._autoLoadStereo,
                self._autoLoadAlways,
                self.stereoOn(),
            )
        )

        nowOn = self.viewStereoOn()
        wasOn = self._currentViewStereoOn
        self._currentViewStereoOn = nowOn

        if not nowOn or wasOn or self._autoLoadAlways or not self._autoLoadStereo:
            #
            # Stereo's not on, or was already on, or we already checked, or we're not autoloading at all
            #
            return

        self.updateAllSources()

    def videoChangeCallback(self, event):
        event.reject()
        deb(
            "videoChange %s auto stereo %s auto always %s stereo %s\n"
            % (
                event.contents(),
                self._autoLoadStereo,
                self._autoLoadAlways,
                self.stereoOn(),
            )
        )

        if (
            self.presentationStereoOn()
            and not self.viewStereoOn()
            and not self._autoLoadAlways
            and self._autoLoadStereo
        ):
            #
            # Prestentation mode is in stereo, controller is not, we didn't already check, and we want to check in stereo view modes
            #
            self.updateAllSources()

    def __init__(self):

        rvtypes.MinorMode.__init__(self)

        self.initStereoPairs()

        self._currentViewStereoOn = self.viewStereoOn()

        self._autoLoadStereo = False
        self._autoLoadAlways = False

        self.readPrefs()

        self.init(
            "stereo-autoload-mode",
            [
                ("source-group-complete", self.sourceCompleteCallback, ""),
                ("graph-state-change", self.stateChangeCallback, ""),
                ("output-video-device-changed", self.videoChangeCallback, ""),
            ],
            None,
            [
                (
                    "File",
                    [
                        (
                            "Options",
                            [
                                (
                                    "Auto-load Right Eye for Stereo (in stereo view modes only)",
                                    self.toggleAutoLoadStereo,
                                    None,
                                    self.autoLoadStereoState,
                                ),
                                (
                                    "Auto-load Right Eye for Stereo (always)",
                                    self.toggleAutoLoadAlways,
                                    None,
                                    self.autoLoadAlwaysState,
                                ),
                            ],
                        )
                    ],
                ),
                (
                    "Image",
                    [
                        (
                            "Stereo",
                            [
                                ("_", None, None, None),
                                (
                                    "Find and Load Right Eye",
                                    self.updateVisibleSources,
                                    None,
                                    None,
                                ),
                            ],
                        )
                    ],
                ),
                (
                    "View",
                    [
                        (
                            "Stereo",
                            [
                                ("_", None, None, None),
                                (
                                    "Find and Load All Right Eyes",
                                    self.updateAllSources,
                                    None,
                                    None,
                                ),
                            ],
                        )
                    ],
                ),
            ],
        )


def createMode():
    return StereoAutoloadMode()
