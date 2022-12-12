#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import commands, rvtypes, extra_commands
import os


class KnownError(Exception):
    pass


class CustomMatteMinorMode(rvtypes.MinorMode):
    def __init__(self):
        rvtypes.MinorMode.__init__(self)
        self._order = []
        self._mattes = {}
        self._currentMatte = ""
        self.init("custom-mattes-mode", None, None, None)

        try:
            definition = os.environ["RV_CUSTOM_MATTE_DEFINITIONS"]
        except KeyError:
            definition = str(
                commands.readSettings("CUSTOM_MATTES", "customMattesDefinition", "")
            )
        try:
            if os.path.exists(definition):
                self.updateMattesFromFile(definition)
        except KnownError as inst:
            print((str(inst)))
        self.setMenuAndBindings()

        lastMatte = str(commands.readSettings("CUSTOM_MATTES", "customMatteName", ""))
        for matte in self._order:
            if matte == lastMatte:
                self.selectMatte(matte)(None)

    def currentMatteState(self, m):
        def matteState():
            if m != "" and self._currentMatte == m:
                return commands.CheckedMenuState
            return commands.UncheckedMenuState

        return matteState

    def selectMatte(self, matte):

        # Create a method that is specific to each matte for setting the
        # relavent session node properties to display the matte
        def select(event):
            self._currentMatte = matte
            if matte == "":
                commands.setIntProperty("#Session.matte.show", [0], True)
                extra_commands.displayFeedback("Disabling mattes", 2.0)
            else:
                m = self._mattes[matte]
                commands.setFloatProperty(
                    "#Session.matte.aspect", [float(m["ratio"])], True
                )
                commands.setFloatProperty(
                    "#Session.matte.heightVisible", [float(m["heightVisible"])], True
                )
                commands.setFloatProperty(
                    "#Session.matte.centerPoint",
                    [float(m["centerX"]), float(m["centerY"])],
                    True,
                )
                commands.setIntProperty("#Session.matte.show", [1], True)
                extra_commands.displayFeedback("Using %s matte" % m["text"], 2.0)
            commands.writeSettings("CUSTOM_MATTES", "customMatteName", matte)

        return select

    def selectMattesFile(self, event):
        definition = commands.openFileDialog(True, False, False, None, None)[0]
        try:
            self.updateMattesFromFile(definition)
        except KnownError as inst:
            print((str(inst)))
        self.setMenuAndBindings()

    def setMenuAndBindings(self):

        # Walk through all of the mattes adding a menu entry as well as a
        # hotkey binding for alt + index number
        # NOTE: The bindings will only matter for the first 9 mattes since you
        # can't really press "alt-10".
        matteItems = []
        bindings = []
        if len(self._order) > 0:
            matteItems.append(
                ("No Matte", self.selectMatte(""), "alt `", self.currentMatteState(""))
            )
            bindings.append(("key-down--alt--`", ""))

            for i, m in enumerate(self._order):
                matteItems.append(
                    (
                        m,
                        self.selectMatte(m),
                        "alt %d" % (i + 1),
                        self.currentMatteState(m),
                    )
                )
                bindings.append(("key-down--alt--%d" % (i + 1), m))
        else:
            matteItems = [
                (
                    "RV_CUSTOM_MATTE_DEFINITIONS UNDEFINED",
                    None,
                    None,
                    lambda: commands.DisabledMenuState,
                )
            ]

        # Always add the option to choose a new defintion file
        matteItems += [("_", None)]
        matteItems += [("Choose Definition File...", self.selectMattesFile, None, None)]

        # Clear the menu then add the new entries
        matteMenu = [("View", [("_", None), ("Custom Mattes", matteItems)])]
        commands.defineModeMenu("custom-mattes-mode", matteMenu, True)

        # Create hotkeys for each matte
        for b in bindings:
            (event, matte) = b
            commands.bind(
                "custom-mattes-mode", "global", event, self.selectMatte(matte), ""
            )

    def updateMattesFromFile(self, filename):

        # Make sure the definition file exists
        if not os.path.exists(filename):
            raise KnownError(
                "ERROR: Custom Mattes Mode: Non-existent mattes"
                + " definition file: '%s'" % filename
            )

        # Clear existing key bindings
        for i in range(len(self._order)):
            commands.unbind(
                "custom-mattes-mode", "global", "key-down--alt--%d" % (i + 1)
            )

        # Walk through the lines of the definition file collecting matte
        # parameters
        order = []
        mattes = {}
        for line in open(filename).readlines():
            tokens = line.strip("\n").split(",")
            if len(tokens) == 6:
                order.append(tokens[0])
                mattes[tokens[0]] = {
                    "name": tokens[0],
                    "ratio": tokens[1],
                    "heightVisible": tokens[2],
                    "centerX": tokens[3],
                    "centerY": tokens[4],
                    "text": tokens[5],
                }

        # Make sure we got some valid mattes
        if len(order) == 0:
            self._order = []
            self._mattes = {}
            raise KnownError(
                "ERROR: Custom Mattes Mode: Empty mattes"
                + " definition file: '%s'" % filename
            )

        # Save the definition path and assign the mattes
        commands.writeSettings("CUSTOM_MATTES", "customMattesDefinition", filename)
        self._order = order
        self._mattes = mattes


def createMode():
    return CustomMatteMinorMode()
