#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import rvtypes, commands, extra_commands
import os

CDLS = ["cc", "ccc", "cdl"]
LUTS = [
    "3dl",
    "a3d",
    "csp",
    "cub",
    "cube",
    "mga",
    "rv3dlut",
    "rvchlut",
    "rvlut",
    "txt",
    "vf",
]

dspIntProps = ["sRGB", "Rec709"]
dspFltProps = ["gamma"]
linIntProps = ["logtype", "sRGB2linear", "Rec709ToLinear"]
linFltProps = ["fileGamma"]


class CustomLUTMenuMode(rvtypes.MinorMode):
    _autoLoadDLUT = False
    _autoLoadCXfrm = {}
    _autoFormat = {}
    _autoLevel = {}
    _autoLUT = {}
    _noLinConv = False
    _noDispCor = False
    _preferEnv = False
    _onAllDisplays = False
    _activeCXfrm = True
    _dirs = {"DLUT": "", "LLUT": "", "FLUT": "", "PLUT": ""}
    lutNodes = {
        "DLUT": "@RVDisplayColor",
        "LLUT": "#RVLookLUT",
        "FLUT": "#RVLinearize",
        "PLUT": "#RVCacheLUT",
    }
    _setups = {}

    def setDefaultDisplayLUT(self, event):
        deflut = commands.getStringProperty(self.lutNodes["DLUT"] + ".lut.file")[0]
        if deflut != "":
            commands.writeSettings("CUSTOM_LUTS", "defaultDisplayLut", deflut)
        else:
            commands.writeSettings("CUSTOM_LUTS", "defaultDisplayLut", "NONE")

    def toggleDefaultDisplayLUT(self, event):
        self._autoLoadDLUT = not self._autoLoadDLUT
        commands.writeSettings(
            "CUSTOM_LUTS", "loadDefaultDisplayLut", int(self._autoLoadDLUT)
        )
        if self._autoLoadDLUT:
            deflut = commands.getStringProperty(self.lutNodes["DLUT"] + ".lut.file")[0]
            extra_commands.displayFeedback("Default Display LUT: %s" % (deflut), 5.0)

    def applyOnAllDisplays(self, event):
        self._onAllDisplays = not self._onAllDisplays
        commands.writeSettings("CUSTOM_LUTS", "onAllDisplays", self._onAllDisplays)

        if self._onAllDisplays:
            lut = commands.getStringProperty(self.lutNodes["DLUT"] + ".lut.file")[0]
            self.setLUT(lut, self.lutNodes["DLUT"])

    def toggleNoLinearization(self, event):
        self._noLinConv = not self._noLinConv
        commands.writeSettings("CUSTOM_LUTS", "linearNoConversion", self._noLinConv)

    def toggleNoLinearToDisplay(self, event):
        self._noDispCor = not self._noDispCor
        commands.writeSettings("CUSTOM_LUTS", "displayNoCorrection", self._noDispCor)

    def saveDirPref(self, event):
        self._preferEnv = not self._preferEnv
        commands.writeSettings("CUSTOM_LUTS", "preferEnvironment", self._preferEnv)

    def saveDefaultProps(self, lutNode, display=False):
        linNode = lutNode
        intProps = dspIntProps
        fltProps = dspFltProps
        if not display:
            groupNode = commands.nodeGroup(lutNode)
            linNodes = commands.closestNodesOfType("RVLinearize", groupNode, 0)
            if len(linNodes) == 0:
                return
            linNode = linNodes[0]
            intProps = linIntProps
            fltProps = linFltProps
        props = {}
        for prop in intProps:
            iprop = linNode + ".color." + prop
            props[prop] = commands.getIntProperty(iprop)[0]
            commands.setIntProperty(iprop, [0], True)
        for prop in fltProps:
            fprop = linNode + ".color." + prop
            props[prop] = commands.getFloatProperty(fprop)[0]
            commands.setFloatProperty(fprop, [1.0], True)
        lut = commands.getStringProperty(lutNode + ".lut.file")[0]
        if lut != "":
            commands.setIntProperty(lutNode + ".lut.active", [1], True)
        if self._activeCXfrm and linNode in self._setups.keys():
            return
        self._setups[linNode] = props

    def restoreDefaultProps(self, lutNode, display=False):
        linNode = lutNode
        intProps = dspIntProps
        fltProps = dspFltProps
        if not display:
            groupNode = commands.nodeGroup(lutNode)
            linNodes = commands.closestNodesOfType("RVLinearize", groupNode, 0)
            if len(linNodes) == 0:
                return
            linNode = linNodes[0]
            intProps = linIntProps
            fltProps = linFltProps
        if linNode in self._setups.keys():
            for prop in intProps:
                commands.setIntProperty(
                    linNode + ".color." + prop, [int(self._setups[linNode][prop])], True
                )
            for prop in fltProps:
                commands.setFloatProperty(
                    linNode + ".color." + prop,
                    [float(self._setups[linNode][prop])],
                    True,
                )
        commands.setIntProperty(lutNode + ".lut.active", [0], True)

    def setLUT(self, path, lutNode, display=False):
        if "@" in lutNode and self._onAllDisplays:
            for displayColor in commands.nodesOfType("RVDisplayColor"):
                self.setLUT(path, displayColor, True)
            return
        try:
            if path not in [None, ""]:
                commands.readLUT(path, lutNode)
                self.saveDefaultProps(lutNode, display)
            else:
                self.restoreDefaultProps(lutNode, display)
        except Exception as inst:
            extra_commands.displayFeedback(
                "LUT File Failed to Read: Check File %s" % path, 5.0
            )
            print(
                (
                    "ERROR: Failed to set LUT '%s' for %s: %s"
                    % (path, lutNode, str(inst))
                )
            )

    def sourceColorSetup(self, event):

        # IF No Linearization is set, turn off all To->Linear Conversions
        if self._noLinConv:
            source = event.contents().split(";")[0]
            linNodes = commands.closestNodesOfType("RVLinearize", source, 0)
            for linNode in linNodes:
                self.saveDefaultProps(linNode)

        # IF Auto load shot color transform
        for lut in self.lutNodes.keys():
            if self.lutNodes[lut].startswith("#") and self._autoLoadCXfrm[lut]:
                if self._autoFormat[lut] != "":
                    print(
                        (
                            "INFO: Searching for accompanying '%s'"
                            % self._autoFormat[lut]
                        )
                    )
                self.loadColorXfrm(lut, event)

        event.reject()

    def displayColorSetup(self, event):
        dnodes = [self.lutNodes["DLUT"]]
        if self._onAllDisplays:
            dnodes = commands.nodesOfType("RVDisplayColor")

        for dnode in dnodes:

            # IF No Display is set, turn off all Linear->To Display settings
            if self._noDispCor:
                self.saveDefaultProps(dnode, True)

            # If Auto Load default LUT is set, Load and set the Display LUT
            if self._autoLoadDLUT:
                lut = commands.readSettings("CUSTOM_LUTS", "defaultDisplayLut", "NONE")
                if lut != "NONE":
                    print(("INFO: Autoloading Default Display LUT: '%s'" % lut))
                    self.setLUT(lut, dnode, True)

        event.reject()

    def toggleCXfrm(self, event):
        for source in commands.nodesOfType("RVSourceGroup"):
            for lut in self.lutNodes.keys():
                try:
                    searchType = self.lutNodes[lut][1:]
                    xfrmNodes = commands.closestNodesOfType(searchType, source, 0)
                    xfrmNode = xfrmNodes[0]
                except:
                    continue

                try:
                    slope = commands.getFloatProperty(xfrmNode + ".CDL.slope")
                    offset = commands.getFloatProperty(xfrmNode + ".CDL.offset")
                    power = commands.getFloatProperty(xfrmNode + ".CDL.power")
                    sat = commands.getFloatProperty(xfrmNode + ".CDL.saturation")[0]
                    defCDL = (
                        slope == [1, 1, 1]
                        and offset == [0, 0, 0]
                        and power == [1, 1, 1]
                        and sat == 1
                    )
                except:
                    defCDL = True

                try:
                    lut = commands.getStringProperty(xfrmNode + ".lut.file")[0]
                    defLUT = lut == ""
                except:
                    defLUT = True

                if not defLUT or not defCDL:
                    try:
                        commands.setIntProperty(
                            xfrmNode + ".CDL.active", [int(not self._activeCXfrm)], True
                        )
                    except:
                        pass
                    if self._activeCXfrm:
                        self.restoreDefaultProps(xfrmNode)
                    else:
                        self.saveDefaultProps(xfrmNode)

        self._activeCXfrm = not self._activeCXfrm
        extra_commands.displayFeedback(
            "Custom Color is now %s" % ("active" if self._activeCXfrm else "inactive"),
            5.0,
        )

    def loadColorXfrm(self, lut, event):
        source = event.contents().split(";")[0]
        srcNode = source + "_source"
        movie = commands.getStringProperty(srcNode + ".media.movie")[0]
        xfrmFile = None
        xfrmExt = self._autoFormat[lut]
        containing = os.path.dirname(os.path.abspath(movie))
        if self._autoLevel[lut] < 0:
            try:
                xfrmFile = self._autoLUT[lut]
                xfrmExt = xfrmFile.split(".")[-1]
            except:
                print(("ERROR: Unable to determine extension from '%s'" % xfrmFile))
        else:
            dirUp = 0
            while dirUp < self._autoLevel[lut]:
                containing = os.path.dirname(containing)
                dirUp += 1
            for entry in os.listdir(containing):
                if entry.lower().endswith("." + self._autoFormat[lut]):
                    xfrmFile = containing + os.sep + entry
                    break

        if xfrmFile == None:
            extra_commands.displayFeedback(
                "Unable to find accompanying color"
                + " transform with extension '%s'" % self._autoFormat[lut],
                5.0,
            )
            print(
                (
                    "WARNING: No '%s' file found in '%s' for '%s'"
                    % (self._autoFormat[lut], containing, movie)
                )
            )
            return

        try:
            searchType = self.lutNodes[lut][1:]
            xfrmNode = extra_commands.associatedNode(searchType, srcNode)
        except:
            print(("ERROR: Unable to find transform node from '%s'" % searchType))
            return

        if xfrmExt in CDLS:
            try:
                commands.readCDL(xfrmFile, xfrmNode, True)
            except Exception as inst:
                extra_commands.displayFeedback(
                    "CDL File Failed to Read: Check File %s" % xfrmFile, 5.0
                )
                print(
                    (
                        "ERROR: Failed to set CDL '%s' for %s: %s"
                        % (xfrmFile, xfrmNode, str(inst))
                    )
                )

        elif xfrmExt in LUTS:
            self.setLUT(xfrmFile, xfrmNode)

        if not self._activeCXfrm:
            self.restoreDefaultProps(lutNode)

    def isAutoLoadingDisplayLUT(self):
        if self._autoLoadDLUT:
            return commands.CheckedMenuState
        else:
            return commands.UncheckedMenuState

    def isToLinearNoConversion(self):
        if self._noLinConv:
            return commands.CheckedMenuState
        else:
            return commands.UncheckedMenuState

    def isToDisplay(self):
        if self._noDispCor:
            return commands.CheckedMenuState
        else:
            return commands.UncheckedMenuState

    def onAllDisplays(self):
        if self._onAllDisplays:
            return commands.CheckedMenuState
        else:
            return commands.UncheckedMenuState

    def isActiveCXfrm(self):
        if self._activeCXfrm:
            return commands.CheckedMenuState
        else:
            return commands.UncheckedMenuState

    def preferEnvVar(self):
        if self._preferEnv:
            return commands.CheckedMenuState
        else:
            return commands.UncheckedMenuState

    def loadLUT(self, path, lutNode):
        def lutLoader(event):
            self.setLUT(path, lutNode)

        return lutLoader

    def isLoadedLUT(self, lut, lutNode):
        def loaded():
            try:
                on = int(commands.getIntProperty(lutNode + ".lut.active")[0])
                current = commands.getStringProperty(lutNode + ".lut.file")[0]
                currentBase = os.path.basename(current)
                lutBase = os.path.basename(lut)
                if on == 1 and currentBase == lutBase:
                    return commands.CheckedMenuState
                else:
                    return commands.UncheckedMenuState
            except:
                return commands.DisabledMenuState

        return loaded

    def setAutoFormat(self, lut, ext):
        def saveSetting(event):
            if (ext != None and self._autoLevel[lut] < 0) or (ext == None):
                self.setAutoFormatLevel(lut, 0)(None)
            self._autoFormat[lut] = ext
            self._autoLoadCXfrm[lut] = ext != None
            commands.writeSettings(
                "CUSTOM_LUTS", lut + "loadColorTransform", self._autoLoadCXfrm[lut]
            )
            if self._autoLoadCXfrm[lut]:
                extra_commands.displayFeedback(
                    "Autoload shot CDL/Look LUT: %s" % (ext), 5.0
                )
                commands.writeSettings(
                    "CUSTOM_LUTS", lut + "autoColorTransform", self._autoFormat[lut]
                )
            else:
                extra_commands.displayFeedback("Disabled Autoload CDL/Look LUT", 5.0)

        return saveSetting

    def isAutoFormat(self, lut, ext):
        def checkFormat():
            if (ext == None and not self._autoLoadCXfrm[lut]) or (
                self._autoLoadCXfrm[lut] and self._autoFormat[lut] == ext
            ):
                return commands.CheckedMenuState
            else:
                return commands.UncheckedMenuState

        return checkFormat

    def setAutoFormatLevel(self, lut, level):
        def saveAutoDirLevel(event):
            if level < 0:
                try:
                    self._autoLUT[lut] = commands.openFileDialog(
                        True, False, False, None, self._dirs[lut]
                    )[0]
                    commands.writeSettings(
                        "CUSTOM_LUTS", lut + "AutoLut", self._autoLUT[lut]
                    )
                    self.setAutoFormat(lut, "")(None)
                except:
                    return
            self._autoLevel[lut] = level
            commands.writeSettings(
                "CUSTOM_LUTS", lut + "autoFormatDirLevel", self._autoLevel[lut]
            )

        return saveAutoDirLevel

    def thisLevel(self, lut, level):
        def atLevel():
            if self._autoLevel[lut] == level:
                return commands.CheckedMenuState
            else:
                return commands.UncheckedMenuState

        return atLevel

    def listSupportedFormats(self, lut, allowCDL=False):
        fmtOpts = [("From", None, "", lambda: commands.DisabledMenuState)]
        for directory, level in [
            ("Same Directory", 0),
            ("Parent Directory", 1),
            ("Fixed Location...", -1),
        ]:
            fmtOpts.append(
                (
                    "  %s" % directory,
                    self.setAutoFormatLevel(lut, level),
                    "",
                    self.thisLevel(lut, level),
                )
            )
        fmtOpts += [("_", None)]

        if allowCDL:
            fmtOpts.append(("CDL", None, "", lambda: commands.DisabledMenuState))
            for ext in CDLS:
                title = "  " + ext.upper()
                fmtOpts.append(
                    (
                        title,
                        self.setAutoFormat(lut, ext),
                        "",
                        self.isAutoFormat(lut, ext),
                    )
                )

        fmtOpts.append(("LUT", None, "", lambda: commands.DisabledMenuState))
        for ext in LUTS:
            title = "  " + ext.upper()
            fmtOpts.append(
                (title, self.setAutoFormat(lut, ext), "", self.isAutoFormat(lut, ext))
            )

        fmtOpts += [
            ("_", None),
            (
                "None/Off",
                self.setAutoFormat(lut, None),
                "",
                self.isAutoFormat(lut, None),
            ),
        ]
        return fmtOpts

    def buildLUTMenu(self, typeStr):
        luts = []
        if self._dirs[typeStr] == "":
            luts = [
                (
                    "RV_CUSTOM_%s_DIR Undefined" % typeStr,
                    None,
                    None,
                    lambda: commands.DisabledMenuState,
                )
            ]
        else:
            lutNode = self.lutNodes[typeStr]
            if os.path.exists(self._dirs[typeStr]):
                dircontents = os.listdir(self._dirs[typeStr])
                dircontents.sort()
                for lut in dircontents:
                    ext = lut.split(".")[-1].lower()
                    if ext in LUTS:
                        path = self._dirs[typeStr] + "/" + lut
                        luts.append(
                            (
                                lut,
                                self.loadLUT(path, lutNode),
                                "",
                                self.isLoadedLUT(path, lutNode),
                            )
                        )
            luts += [
                ("_", None),
                ("Clear LUT Selection", self.loadLUT(None, lutNode), "", None),
            ]

        luts += [
            ("_", None),
            ("Change %s Directory..." % typeStr, self.changeLUTDir(typeStr), "", None),
        ]
        return luts

    def changeLUTDir(self, typeStr):
        def changeSpecificLUTDir(event):
            try:
                self._dirs[typeStr] = commands.openFileDialog(
                    True, False, True, None, None
                )[0]
                commands.defineModeMenu("custom-luts-mode", self.createMenu(), True)
                commands.writeSettings(
                    "CUSTOM_LUTS", "custom%sDir" % typeStr, self._dirs[typeStr]
                )
            except:
                pass

        return changeSpecificLUTDir

    def createMenu(self):
        return [
            (
                "Custom LUT",
                [
                    ("Display", None, None, lambda: commands.DisabledMenuState),
                    ("  Current Display LUT", self.buildLUTMenu("DLUT")),
                    (
                        "  Save Current Display LUT As Default",
                        self.setDefaultDisplayLUT,
                        "",
                        lambda: commands.UncheckedMenuState,
                    ),
                    (
                        "  Auto Load Default Display LUT On Launch",
                        self.toggleDefaultDisplayLUT,
                        "",
                        self.isAutoLoadingDisplayLUT,
                    ),
                    (
                        "  Force View->No Correction On Launch",
                        self.toggleNoLinearToDisplay,
                        "",
                        self.isToDisplay,
                    ),
                    (
                        "  Apply To All Displays",
                        self.applyOnAllDisplays,
                        "",
                        self.onAllDisplays,
                    ),
                    ("Sources", None, None, lambda: commands.DisabledMenuState),
                    ("  Current Look LUT", self.buildLUTMenu("LLUT")),
                    ("  Current File LUT", self.buildLUTMenu("FLUT")),
                    ("  Current Pre-Cache LUT", self.buildLUTMenu("PLUT")),
                    (
                        "  Auto Load Accompanying Look LUT",
                        self.listSupportedFormats("LLUT"),
                    ),
                    (
                        "  Auto Load Accompanying CDL/File LUT",
                        self.listSupportedFormats("FLUT", True),
                    ),
                    (
                        "  Auto Load Accompanying Pre-Cache LUT",
                        self.listSupportedFormats("PLUT"),
                    ),
                    (
                        "  Custom Source Assignments Enabled",
                        self.toggleCXfrm,
                        "/",
                        self.isActiveCXfrm,
                    ),
                    (
                        "  Force Color->No Conversion On Load",
                        self.toggleNoLinearization,
                        "",
                        self.isToLinearNoConversion,
                    ),
                    ("_", None),
                    (
                        "Prefer Environment LUT_DIR Over Last Used",
                        self.saveDirPref,
                        "",
                        self.preferEnvVar,
                    ),
                ],
            )
        ]

    def __init__(self):
        rvtypes.MinorMode.__init__(self)

        self._autoLoadDLUT = commands.readSettings(
            "CUSTOM_LUTS", "loadDefaultDisplayLut", self._autoLoadDLUT
        )
        for lut in self.lutNodes.keys():
            self._autoLoadCXfrm[lut] = commands.readSettings(
                "CUSTOM_LUTS", lut + "loadColorTransform", False
            )
            self._autoFormat[lut] = commands.readSettings(
                "CUSTOM_LUTS", lut + "autoColorTransform", ""
            )
            self._autoLevel[lut] = commands.readSettings(
                "CUSTOM_LUTS", lut + "autoFormatDirLevel", 0
            )
            self._autoLUT[lut] = commands.readSettings(
                "CUSTOM_LUTS", lut + "AutoLut", "NONE"
            )
        self._noLinConv = commands.readSettings(
            "CUSTOM_LUTS", "linearNoConversion", self._noLinConv
        )
        self._noDispCor = commands.readSettings(
            "CUSTOM_LUTS", "displayNoCorrection", self._noDispCor
        )
        self._preferEnv = commands.readSettings(
            "CUSTOM_LUTS", "preferEnvironment", self._preferEnv
        )
        self._onAllDisplays = commands.readSettings(
            "CUSTOM_LUTS", "onAllDisplays", self._onAllDisplays
        )

        for typeStr in self._dirs.keys():
            customEnvDir = os.environ.get("RV_CUSTOM_%s_DIR" % typeStr, "")
            customSettingDir = commands.readSettings(
                "CUSTOM_LUTS", "custom%sDir" % typeStr, customEnvDir
            )
            self._dirs[typeStr] = customSettingDir
            if self._preferEnv and customEnvDir != "":
                self._dirs[typeStr] = customEnvDir

        self.init(
            "custom-luts-mode",
            [
                ("source-group-complete", self.sourceColorSetup, "Source Color Setup"),
                (
                    "after-progressive-loading",
                    self.displayColorSetup,
                    "Display Color Setup",
                ),
                ("session-initialized", self.displayColorSetup, "Display Color Setup"),
            ],
            None,
            self.createMenu(),
            "zzz",
        )

        commands.bind("custom-luts-mode", "global", "key-down--/", self.toggleCXfrm, "")


def createMode():
    return CustomLUTMenuMode()
