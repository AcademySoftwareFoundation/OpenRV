#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import rvtypes
from rv import commands
from rv import extra_commands

import os, re, platform
from six.moves.urllib.parse import urlparse


def groupMemberOfType(node, memberType):
    for n in commands.nodesInGroup(node):
        if commands.nodeType(n) == memberType:
            return n
    return None


class SourceSetupMode(rvtypes.MinorMode):
    """
    This function is bound to the source-group-complete
    event. (Previously we used the new-source event, but there are
    limitations to using that.) Its called any time source media is
    added to RV. You can set up prefered color space conversions and
    pixel aspect, etc here. If the file name is meaningful you can
    even use that. You can replace it wholesale by doing this:

      bind("source-group-complete", myFunc);

    (E.g. from your ~/.rvrc.py file or a facility init if you
    don't want to edit this directly) You can get attrs and
    structure info by supplying two args to the
    sourceImageStructure() or sourceAttributes() functions.

    ORDERING: this mode uses a sort key of "source_setup" with an ordering
    value of 0. If you create a mode which piggybacks off of the
    results of this one for source-group-complete you should also use
    "source_setup" as the sort key but with an ordering number *after*
    0. If you want yours *before* this mode do the same, but use a
    negative ordering number.
    """

    def sourceSetup(self, event, noColorChanges=False):

        #
        #  event.reject() is done to allow other functions bound to
        #  this event to get a chance to modify the state as well. If
        #  its not rejected, the event will be eaten and no other call
        #  backs will occur.
        #

        event.reject()

        args = event.contents().split(";;")
        group = args[0]
        fileSource = groupMemberOfType(group, "RVFileSource")
        imageSource = groupMemberOfType(group, "RVImageSource")
        source = fileSource if imageSource == None else imageSource
        linPipeNode = groupMemberOfType(group, "RVLinearizePipelineGroup")
        linNode = groupMemberOfType(linPipeNode, "RVLinearize")
        ICCNode = groupMemberOfType(linPipeNode, "ICCLinearizeTransform")
        lensNode = groupMemberOfType(linPipeNode, "RVLensWarp")
        fmtNode = groupMemberOfType(group, "RVFormat")
        tformNode = groupMemberOfType(group, "RVTransform2D")
        lookPipeNode = groupMemberOfType(group, "RVLookPipelineGroup")
        lookNode = groupMemberOfType(lookPipeNode, "RVLookLUT")
        typeName = commands.nodeType(source)
        fileNames = commands.getStringProperty("%s.media.movie" % source, 0, 1000)
        fileName = fileNames[0]
        ext = fileName.split(".")[-1].upper()
        igPrim = self.checkIgnorePrimaries(ext)
        mInfo = commands.sourceMediaInfo(source, None)

        #
        #  Scarf the attrs we're intersted in up front so we don't have to
        #  keep looking for them
        #

        srcInfo = {
            "ColorSpace": "",
            "ColorSpacePrimaries": "",
            "ConversionMatrix": "",
            "TransferFunction": "",
            "JPEGDensity": "",
            "JPEGPixelAspect": "",
            "PixelAspectRatio": "",
            "TIFFImageDescription": "",
            "DPXCreator": "",
            "EXIFOrientation": "",
            "EXIFGamma": "",
            "DPX0Transfer": "",
            "ARRIDataSpace": "",
            "ICCProfileDesc": "",
            "ICCProfileVer": "",
            "ICCProfileData": [],
        }

        #
        #  This will throw if RV failed to load fileName.  In that case, don't
        #  configure the color on this Source.
        #

        try:
            srcAttrs = commands.sourceAttributes(source, fileName)
            srcData = commands.sourceDataAttributes(source, fileName)
        except:
            return

        if srcAttrs == None:
            print(
                (
                    "ERROR: SourceSetup: source %s/%s has no attributes"
                    % (source, fileName)
                )
            )
            return

        attrDict = dict(zip([i[0] for i in srcAttrs], [j[1] for j in srcAttrs]))
        attrMap = {
            "ColorSpace/ICC/Description": "ICCProfileDesc",
            "ColorSpace/ICC/Version": "ICCProfileVer",
            "ColorSpace": "ColorSpace",
            "ColorSpace/Transfer": "TransferFunction",
            "ColorSpace/Primaries": "ColorSpacePrimaries",
            "DPX-0/Transfer": "DPX0Transfer",
            "ColorSpace/Conversion": "ConversionMatrix",
            "JPEG/PixelAspect": "JPEGPixelAspect",
            "PixelAspectRatio": "PixelAspectRatio",
            "JPEG/Density": "JPEGDensity",
            "TIFF/ImageDescription": "TIFFImageDescription",
            "DPX/Creator": "DPXCreator",
            "EXIF/Orientation": "EXIFOrientation",
            "EXIF/Gamma": "EXIFGamma",
            "ARRI-Image/DataSpace": "ARRIDataSpace",
        }
        for key in attrMap.keys():
            try:
                srcInfo[attrMap[key]] = attrDict[key]
            except KeyError:
                pass

        if len(srcData) > 0:
            dataDict = dict(zip([i[0] for i in srcData], [j[1] for j in srcData]))
            dataMap = {"ColorSpace/ICC/Data": "ICCProfileData"}
            for key in dataMap.keys():
                try:
                    srcInfo[dataMap[key]] = dataDict[key]
                except KeyError:
                    pass

        #
        #  Rules based on the extention
        #

        if ext == "DPX":
            if (
                srcInfo["DPXCreator"] == "AppleComputers.libcineon"
                or srcInfo["DPXCreator"] == "AUTODESK"
            ):
                #
                #  Final Cut's "Color" and Maya write out bogus DPX
                #  header info with the aspect ratio fields set
                #  improperly (to 0s usually). Properly undefined DPX
                #  headers do not have the value 0.
                #

                if float(srcInfo["PixelAspectRatio"]) == 0.0:
                    self.setPixelAspect(lensNode, 1.0)
            elif (
                srcInfo["ColorSpace"] == "" or srcInfo["ColorSpace"] == "Other (0)"
            ) and (
                srcInfo["DPX0Transfer"] == "" or srcInfo["DPX0Transfer"] == "Other (0)"
            ):
                #
                #  Nuke produces identical (uninformative) dpx headers for
                #  both Linear and Cineon files.  But we expect Cineon to be
                #  much more common, so go with that.
                #

                srcInfo["TransferFunction"] = "Cineon Log"
        elif ext == "TIF" and srcInfo["TransferFunction"] == "":
            #
            #  Assume 8bit tif files are sRGB if there's no other indication;
            #  fall back to linear.
            #

            if mInfo["bitsPerChannel"] == 8:
                srcInfo["TransferFunction"] = "sRGB"
            else:
                srcInfo["TransferFunction"] = "Linear"
        elif ext == "ARI" and (
            srcInfo["TransferFunction"] == ""
            or srcInfo["TransferFunction"] == "ARRI LogC"
        ):
            #
            #  Assume tif files are linear if there's no other indication
            #

            srcInfo["TransferFunction"] = "ALEXA LogC"
        elif ext == "ARI" and srcInfo["TransferFunction"] == "ARRI LogC Film":
            #
            #  Assume tif files are linear if there's no other indication
            #

            srcInfo["TransferFunction"] = "ALEXA LogC Film"
        elif (
            ext in ["JPEG", "JPG", "MOV", "AVI", "MP4"]
            and srcInfo["TransferFunction"] == ""
        ):
            #
            #  Assume jpeg/mov is in sRGB space if none is specified
            #

            srcInfo["TransferFunction"] = "sRGB"
        elif (
            ext in ["J2C", "J2K", "JPT", "JP2"]
            and srcInfo["ColorSpacePrimaries"] == "UNSPECIFIED"
        ):
            #
            #  If we're assuming XYZ primaries, but ignoring primaries just set
            #  transfer to sRGB.
            #

            if igPrim:
                srcInfo["TransferFunction"] = "sRGB"

        if igPrim:
            commands.setIntProperty(linNode + ".color.ignoreChromaticities", [1], True)

        #
        #  Check for any ICC information
        #

        if ICCNode:
            commands.setIntProperty(ICCNode + ".node.active", [0], True)
        if srcInfo["ICCProfileDesc"] != "":
            if ICCNode and len(srcInfo["ICCProfileData"]) > 0:
                commands.setStringProperty(
                    ICCNode + ".inProfile.description",
                    [srcInfo["ICCProfileDesc"]],
                    True,
                )
                commands.setFloatProperty(
                    ICCNode + ".inProfile.version",
                    [float(srcInfo["ICCProfileVer"])],
                    True,
                )
                commands.setByteProperty(
                    ICCNode + ".inProfile.data", srcInfo["ICCProfileData"], True
                )
                commands.setIntProperty(ICCNode + ".node.active", [1], True)
                srcInfo["TransferFunction"] = "Linear"
            else:
                #
                #  Hack -- if you see sRGB in a color profile name just use the
                #  built-in sRGB conversion.
                #

                if "sRGB" in srcInfo["ICCProfileDesc"]:
                    srcInfo["TransferFunction"] = "sRGB"
                else:
                    srcInfo["TransferFunction"] = ""

        #
        #  Handle aspect ratio changes
        #

        if srcInfo["TIFFImageDescription"] == "Image converted using ifftoany":
            #
            #  Get around maya bugs
            #

            print(
                (
                    "WARNING: Assuming %s was created by Maya with a bad pixel aspect ratio\n"
                    % fileName
                )
            )
            self.setPixelAspect(lensNode, 1.0)

        if srcInfo["JPEGPixelAspect"] != "" and srcInfo["JPEGDensity"] != "":
            info = commands.sourceMediaInfo(source, fileName)
            attrPA = float(srcInfo["JPEGPixelAspect"])
            imagePA = float(info["width"]) / float(info["height"])
            testDiff = attrPA - 1.0 / imagePA

            if (testDiff < 0.0001) and (testDiff > -0.0001):
                #
                #  Maya JPEG -- fix pixel aspect
                #

                print(
                    (
                        "WARNING: Assuming %s was created by Maya with a bad pixel aspect ratio\n"
                        % fileName
                    )
                )
                self.setPixelAspect(lensNode, 1.0)

        #
        #  Handle rotation
        #

        if srcInfo["EXIFOrientation"] != "":
            #
            #  Some of these tags are beyond the internal image
            #  orienation choices so we need to possibly rotate, etc
            #

            if not self.doNotConfigure(tformNode):
                rprop = tformNode + ".transform.rotate"
                if srcInfo["EXIFOrientation"] == "right - top":
                    commands.setFloatProperty(rprop, [90.0], True)
                elif srcInfo["EXIFOrientation"] == "right - bottom":
                    commands.setFloatProperty(rprop, [-90.0], True)
                elif srcInfo["EXIFOrientation"] == "left - top":
                    commands.setFloatProperty(rprop, [90.0], True)
                elif srcInfo["EXIFOrientation"] == "left - bottom":
                    commands.setFloatProperty(rprop, [-90.0], True)

        if not noColorChanges:
            #
            #  Assume (in the absence of info to the contrary) any 8bit file will be in sRGB space.
            #
            if srcInfo["TransferFunction"] == "" and mInfo["bitsPerChannel"] == 8:
                srcInfo["TransferFunction"] = "sRGB"

            #
            #  Allow user to override with environment variables
            #
            srcInfo["TransferFunction"] = self.checkEnvVar(
                ext, mInfo["bitsPerChannel"], srcInfo["TransferFunction"]
            )

            if self.setFileColorSpace(
                linNode, srcInfo["TransferFunction"], srcInfo["ColorSpace"]
            ):

                #
                #  The default display correction is sRGB if the
                #  pixels can be converted to (or are already in)
                #  linear space
                #
                #  For gamma instead do this:
                #
                #      setFloatProperty("#RVDisplayColor.color.gamma", [2.2])
                #
                #  For a linear -> screen LUT do this:
                #
                #      readLUT(lutfile, "#RVDisplayColor", True)
                #

                self.setDisplayFromProfile()

    def setDisplayFromProfile(self):
        #
        #  We only configure display nodes once (since otherwise we may
        #  overwrite stuff the user has done).  And DisplayGroups are created
        #  only when the graph is created.
        #
        #  Later: it would be better to only create DisplayGroups once per run,
        #  which would mean setting them aside when the session is destroyed
        #  and re-connecting them if it is re-built.
        #
        if not self._haveNewDisplayGroups:
            return

        #
        #  The new default behavior in RV 4 is to try and
        #  assign display profiles to the devices if one is
        #  available. If it does find a profile it will use it
        #  otherwise it does what RV 3 used to do. Since this
        #  is using readSettings the actual code is in
        #  extra_commands.mu (so this python file doesn't
        #  require Qt)
        #
        dnodesWithProfile = extra_commands.setDisplayProfilesFromSettings()

        #
        #   See if there are profiles assigned to each of the
        #   devices. If so set them
        #

        for dnode in commands.nodesOfType("RVDisplayColor"):

            #
            #  If we managed to recognize a transform to linear, default
            #  to a display to a linear -> sRGB
            #

            dpipe = commands.nodeGroup(dnode)
            dgroup = commands.nodeGroup(dpipe)

            dICCNode = groupMemberOfType(dpipe, "ICCDisplayTransform")
            if dICCNode:
                commands.setIntProperty(dICCNode + ".node.active", [0], True)

            if (
                dgroup != None
                and commands.nodeType(dgroup) == "RVDisplayGroup"
                and not dgroup in dnodesWithProfile
            ):

                dICC = commands.getStringProperty(dgroup + ".device.systemProfileURL")
                if dICCNode and len(dICC) == 1 and not dICC[0] == "":
                    dICCPath = urlparse(dICC[0].replace("%20", " ")).path
                    if "windows" in platform.platform().lower() and re.match(
                        "^/.:.*$", dICCPath
                    ):
                        dICCPath = dICCPath[1:]
                    commands.setStringProperty(
                        dICCNode + ".outProfile.url", [dICCPath], True
                    )
                    commands.setIntProperty(dICCNode + ".node.active", [1], True)
                else:
                    sRGBDisplay = (
                        commands.getIntProperty(dnode + ".color.sRGB", 0, 20000)[0] != 0
                    )
                    rec709Display = (
                        commands.getIntProperty(dnode + ".color.Rec709", 0, 20000)[0]
                        != 0
                    )
                    gammaDisplay = (
                        commands.getFloatProperty(dnode + ".color.gamma", 0, 20000)[0]
                        != 1.0
                    )
                    lutDisplay = (
                        commands.getIntProperty(dnode + ".lut.active", 0, 20000)[0] != 0
                    )
                    anyDisplay = (
                        sRGBDisplay or rec709Display or gammaDisplay or lutDisplay
                    )

                    if not anyDisplay:
                        commands.setIntProperty(dnode + ".color.sRGB", [1], True)

        self._haveNewDisplayGroups = False

    def setFileColorSpace(self, node, tf, cs):
        if self.doNotConfigure(node):
            return True
        sRGB = 0
        logT = 0
        r709 = 0
        tolinear = True
        fileGamma = 1.0
        gamma = tf.split(" ")

        #
        #  If we know the color space, do the right thing, otherwise indicate
        #  that the file pixels are not known to be linear
        #

        if tf == "sRGB":
            sRGB = 1
        elif tf == "Cineon Log" or tf == "Kodak Log":
            logT = 1  # Cineon log
        elif tf == "ALEXA LogC":
            logT = 3  # ARRI Log-C
        elif tf == "ALEXA LogC Film":
            logT = 4  # ARRI Log-C Film
        elif tf == "Viper Log":
            logT = 2
        elif tf == "Red Log":
            logT = 6
        elif tf == "Linear":
            pass
        elif tf == "Rec709":
            r709 = 1
        elif len(gamma) == 2 and gamma[0] == "Gamma":
            fileGamma = float(gamma[1])
        elif tf == "" and cs != "Linear":
            tolinear = False
        else:
            print(("WARNING: Unknown TransferFunction '%s'" % tf))

        commands.setIntProperty(node + ".color.sRGB2linear", [sRGB], True)
        commands.setIntProperty(node + ".color.logtype", [logT], True)
        commands.setIntProperty(node + ".color.Rec709ToLinear", [r709], True)
        commands.setFloatProperty(node + ".color.fileGamma", [fileGamma], True)

        return tolinear

    def doNotConfigure(self, node):
        """
        If the node does not exist (probably because we're using an alternate
        color pipeline), don't configure.

        Also, if we are reading a session file and this node is _defined_ in the
        session file, then we should not configure it.  On the other hand this
        may be a "stub" session file, with perhaps only FileSource nodes in it,
        in which case we _should_ configure the node, since it was created with
        default values.

        And Note, we _should_ configure a node even if it was read from a
        session file if we're not reading a session file _now_, since otherwise
        source_setup could not operate on sources read from a session file
        whose media changes later.
        """
        return (not commands.nodeExists(node)) or (
            self._readingSession
            and commands.propertyExists(node + ".internal.creationContext")
            and commands.getIntProperty(node + ".internal.creationContext")[0] == 1
        )

    def setPixelAspect(self, node, ratio):
        if self.doNotConfigure(node):
            return
        commands.setFloatProperty(node + ".warp.pixelAspectRatio", [ratio], True)

    def flip(self, node):
        if self.doNotConfigure(node):
            return
        commands.setIntProperty(node + ".transform.flip", [1], True)

    def flop(self, node):
        if self.doNotConfigure(node):
            return
        commands.setIntProperty(node + ".transform.flop", [1], True)

    def checkIgnorePrimaries(self, ext):
        var = "RV_OVERRIDE_IGNORE_PRIMARIES_" + ext

        val = commands.commandLineFlag(var, None)

        if val is None:
            val = os.getenv(var, "false")

        return val != "false"

    def checkEnvVar(self, ext, bits, transfer):
        varGeneral = "RV_OVERRIDE_TRANSFER_" + ext
        varSpecific = varGeneral + "_" + str(bits)

        val = commands.commandLineFlag(varSpecific, None)

        if val == None:
            val = commands.commandLineFlag(varGeneral, None)
        if val == None:
            val = os.getenv(varSpecific, None)
        if val == None:
            val = os.getenv(varGeneral, transfer)

        return val

    def beforeSessionRead(self, event):
        event.reject()
        self._readingSession = True

    def afterSessionRead(self, event):
        event.reject()
        self._readingSession = False

    def checkForDisplayGroup(self, event):
        event.reject()
        if commands.nodeType(event.contents()) == "RVDisplayGroup":
            self._haveNewDisplayGroups = True

    def __init__(self):
        self._readingSession = False
        self._haveNewDisplayGroups = True

        rvtypes.MinorMode.__init__(self)

        self.init(
            "Source Setup",
            None,
            [
                ("after-session-read", self.afterSessionRead, ""),
                ("before-session-read", self.beforeSessionRead, ""),
                (
                    "source-group-complete",
                    self.sourceSetup,
                    "Color and Geometry Management",
                ),
                ("graph-new-node", self.checkForDisplayGroup, ""),
            ],
            None,
            "source_setup",
            0,
        )
        # "source_setup" key used by this and ocio modes


def createMode():
    return SourceSetupMode()
