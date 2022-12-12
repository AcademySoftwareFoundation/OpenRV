# Copyright (c) 2020 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.

"""
This module defines an extremely simple API to the RV session file.

Basically, you make a Session object, then various top-level nodes
within the session (with the session's newNode() method), then
various properties within the nodes.

There are two levels of access to session properties here.  You can
use the utility functions defined below for the various node types,
or you can specify the path to the property explicitly and use the
general "setProperty" interface.

To discover the proper path for a given property, examine an RV
Session file written by RV that contains the stuff you want to set.

Supported top-level node types:

    Source
    Stack
    Switch
    Sequence
    Layout
    Retime
    Folder

CAVEATS:

* This is write-only, no session file reading yet.

* Only version 4 session files (RV v4.0.7 and above) are supported 

RELEASE NOTES:

0.5 Removed support for RV-3 version 2 session files in favor of RV-4
    version 4 session files. This includes some limited support for
    pipeline groups.

0.4 This version still support version 2 session files, but you can now write
    nodes of arbitrary type into the Session file (for example see the Transition
    nodes in the sample code).

0.3 The session file format has changed slightly.  Naming is
    different (but this change is backwards compatible) and the Retime
    node has become a "real" group node, so session files written with
    v0.2 that contain a Retime node will not read properly in RV v3.10.1
    and above.

    Also the sense of the video "scale" attribute has been reversed
    to match it's "length multiplier" meaning in the interface.  IE
    scale > 1 means the number of frames in the input will be
    increased.

    Improved pydoc output.

0.2 Initial public version.

"""

import sys
import six
import gto
import gtoContainer as gc

apiName = "rvSession.py"
apiVersion = "0.5"
nodeVersions = {"connection": 2, "RVColor": 2, "RVPaint": 3, "RVSession": 4}


class _Node:
    """
    Base class for all top-level nodes.
    """

    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.name = ""
        self.uiName = ""
        self.properties = {}
        self.inputs = []
        self.maxInputs = -1  # unlimited

    def setUIName(self, name):
        """
        UI names are for convenience only.  They do not need to be unique.
        """
        (objType, cons) = Session.lookupNodeType(self.typeName)
        self.setProperty(objType, "", "ui", "name", gto.STRING, name)
        self.uiName = name

    def addPipelineNode(self, nodeType, pipelineType, pipelineSuffix):
        """
        Append an instance of a nodeType node to the pipelineType pipeline and
        return the resulting name of the added node.
        """
        nodeType = six.ensure_binary(nodeType)
        try:
            pipeProp = self.getProperty(
                pipelineType, pipelineSuffix, "pipeline", "nodes"
            )
            pipenodes = pipeProp[1] + [nodeType]
        except KeyError:
            pipenodes = [nodeType]
        self.setProperty(
            pipelineType, pipelineSuffix, "pipeline", "nodes", gto.STRING, pipenodes
        )

        return "%s_%d" % (pipelineSuffix, len(pipenodes) - 1)

    def getPipelineNode(self, nodeType, pipelineType, pipelineSuffix, occurrence=1):
        """
        Find the n-th occurrence node of nodeType in the given pipeline group or
        return None if it cannot be located.
        """
        try:
            pipeProp = self.getProperty(
                pipelineType, pipelineSuffix, "pipeline", "nodes"
            )
        except KeyError:
            return None

        index = -1
        count = occurrence - 1
        for i, node in enumerate(pipeProp[1]):
            if node == nodeType:
                count -= 1
                if count == 0:
                    index = i
                    break
        if index == -1:
            return None

        return "%s_%d" % (pipelineSuffix, index)

    def getProperty(self, objType, objName, contName, propName):
        """
        Get the value of a property of the _Node object.
        """
        return self.properties[(objType, objName, contName, propName)]

    def setProperty(self, objType, objName, contName, propName, valueType, value):
        """
        If you know the path to a given property you can use
        setProperty directly.  Note that "objName" in this
        context is the part of the object name that follows the
        "<groupNodeName>_" portion of the object name you see in
        the session file.
        """
        if valueType == gto.STRING:
            if isinstance(value, (list, tuple)):
                for i, v in enumerate(value):
                    value[i] = six.ensure_binary(v)
            elif isinstance(value, six.string_types):
                value = six.ensure_binary(value)

        self.properties[(objType, objName, contName, propName)] = (valueType, value)

    def addInput(self, node):
        """
        Add an input node to this node.  Arg must be a node returned
        from Session.newNode().  Note that some node types (Sources
        are the only current example) do not take inputs, while
        others may only allow a certain number of inputs.
        """
        if self.maxInputs != -1 and len(self.inputs) + 1 > self.maxInputs:
            raise Exception(
                "ERROR: node '%s' (%s) only allowed %d inputs"
                % (self.name, self.typeName, self.maxInputs)
            )
        else:
            self.inputs.append(node.name)


class _GroupNode(_Node):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        _Node.__init__(self)


class Source(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Source"
        _Node.__init__(self)
        self.maxInputs = 0
        self.txt = Text()

    #
    #   Utility Functions
    #
    #   TODO:
    #   pre-cache, file, look lut
    #   stereo offsets
    #   gamma, srgb, color transforms
    #   contrast, hsv

    ## Media and timing
    def setFPS(self, fps):
        """
        RV will detect FPS where possible (especially from
        movie files, but also from image headers in file
        sequences).  Otherwise you can set it here.
        """
        self.setProperty("RVFileSource", "source", "group", "fps", gto.FLOAT, fps)

    def setMedia(self, media):
        """
        Input media can be either a single path or python list
        of paths to specify both left and right eyes of stereo
        media, a separate audio file, etc.
        """
        self.setProperty("RVFileSource", "source", "media", "movie", gto.STRING, media)

    def setAudioOffset(self, offset):
        """
        Offset must be in _seconds_.
        """
        self.setProperty(
            "RVFileSource", "source", "group", "audioOffset", gto.FLOAT, offset
        )

    def setRangeOffset(self, offset):
        """
        Offset in frames for source frame numbers.  For example
        to have the first frame of foo.mov set to 101, set range
        offset to 100.
        """
        self.setProperty(
            "RVFileSource", "source", "group", "rangeOffset", gto.INT, offset
        )

    def setCutIn(self, frame):
        """
        Set the "in point" for this source.  Views that pay
        attention to in/out points will show you this source
        starting from this frame.  Note that the frame number is
        interpreted _after_ any range offset is applied.
        """
        self.setProperty("RVFileSource", "source", "cut", "in", gto.INT, frame)

    def setCutOut(self, frame):
        """
        Set the "out point" for this source.  Views that pay
        attention to in/out points will show you this source
        ending with this frame.  Note that the frame number is
        interpreted _after_ any range offset is applied.
        """
        self.setProperty("RVFileSource", "source", "cut", "out", gto.INT, frame)

    ## Metadata
    def setMetaData(self, metadata):
        """
        Metadata string should be in the form "key=value". For multiple bits of metadata,
        metadata should equal a list of [ k1=v1, k2=v2, k3=v3 ... ].
        Items at the end of the list will appear topmost as the metadata in the viewer
        As a helper, a dictionary can be supplied that will print metadata sorted by the keys
        """
        if isinstance(metadata, dict):
            metadata = [
                "{0}={1}".format(k, v)
                for k, v in sorted(metadata.items(), reverse=True)
            ]

        self.setProperty(
            "RVFileSource",
            "source",
            "site_annotation",
            "metadata",
            gto.STRING,
            metadata,
        )

    ## Linearize
    def setIgnoreChromaticities(self, ignore):
        """
        Turn on/off ignoreChromaticities
        """
        args = ["RVLinearize", "RVLinearizePipelineGroup", "tolinPipeline"]
        pipeNode = self.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.addPipelineNode(*args)
        self.setProperty(
            "RVLinearize",
            pipeNode,
            "color",
            "ignoreChromaticities",
            gto.INT,
            int(ignore),
        )
        self.setProperty("RVLinearize", pipeNode, "color", "active", gto.INT, 1)

    ## LensWarp
    def setAspectRatio(self, pa):
        """
        Set the pixel aspect ratio for the source.
        """
        args = ["RVLensWarp", "RVLinearizePipelineGroup", "tolinPipeline"]
        pipeNode = self.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.addPipelineNode(*args)
        self.setProperty(
            "RVLensWarp", pipeNode, "warp", "pixelAspectRatio", gto.FLOAT, pa
        )
        self.setProperty("RVLensWarp", pipeNode, "node", "active", gto.INT, 1)

    ## Color
    def setExposure(self, exp):
        """
        Exposure is a power function; default exposure is 0.0.  To
        increase exposure by 1 "stop", set to 1.0.  The argument
        should be a 3-tuple of floats corresponding to R,G,B
        exposure.
        """
        args = ["RVColor", "RVColorPipelineGroup", "colorPipeline"]
        pipeNode = self.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.addPipelineNode(*args)
        self.setProperty("RVColor", pipeNode, "color", "exposure", gto.FLOAT, exp)
        self.setProperty("RVColor", pipeNode, "color", "active", gto.INT, 1)
        self.setProperty(
            "RVFormat", "format", "color", "allowFloatingPoint", gto.INT, 1
        )
        self.setProperty("RVFormat", "format", "color", "maxBitDepth", gto.INT, 0)

    def setColorScale(self, scale):
        """
        Scale Color values; default scale is 0.0. The argument
        should be a 3-tuple of floats corresponding to R,G,B
        exposure.
        """
        args = ["RVColor", "RVColorPipelineGroup", "colorPipeline"]
        pipeNode = self.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.addPipelineNode(*args)
        self.setProperty("RVColor", pipeNode, "color", "scale", gto.FLOAT, scale)
        self.setProperty("RVColor", pipeNode, "color", "active", gto.INT, 1)
        self.setProperty(
            "RVFormat", "format", "color", "allowFloatingPoint", gto.INT, 1
        )
        self.setProperty("RVFormat", "format", "color", "maxBitDepth", gto.INT, 0)

    ## Channels and Layers
    def setChannelOrder(self, channelOrder):
        """
        channelOrder is a re-ordering of channels to the RGBA for display. channelOrder
        happens in hardware. This string is in the form "Z" or "ZZZA", etc.
        Not working...
        """
        args = ["RVColor", "RVColorPipelineGroup", "colorPipeline"]
        pipeNode = self.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.addPipelineNode(*args)
        self.setProperty(
            "RVColor", pipeNode, "color", "channelOrder", gto.STRING, channelOrder
        )
        self.setProperty("RVColor", pipeNode, "color", "active", gto.INT, 1)
        self.setProperty(
            "RVFormat", "format", "color", "allowFloatingPoint", gto.INT, 1
        )
        self.setProperty("RVFormat", "format", "color", "maxBitDepth", gto.INT, 0)
        self.setProperty(
            "RVFileSource", "source", "request", "readAllChannels", gto.INT, 1
        )

    def setImageLayerSelection(self, layer):
        """
        Defines the exr layer to view.
        """
        self.setProperty(
            "RVFileSource",
            "source",
            "request",
            "imageLayerSelection",
            gto.STRING,
            layer,
        )

    def setChannelMap(self, channelMap):
        """
        channelMap can be a single channel which represents RGB, or a tuple ("Z","A"). Mapping
        happens in software
        """
        self.setProperty(
            "RVChannelMap", "channelMap", "format", "channels", gto.STRING, channelMap
        )
        self.setProperty(
            "RVFormat", "format", "color", "allowFloatingPoint", gto.INT, 1
        )
        self.setProperty("RVFormat", "format", "color", "maxBitDepth", gto.INT, 0)
        self.setProperty(
            "RVFileSource", "source", "request", "readAllChannels", gto.INT, 1
        )

    ## Text
    def setTextPosition(self, x, y):
        """
        modify position of text. Positions are x, y values:
        lower left ==  0,1
        upper left ==  0,0
        lower right == 1,1
        upper right == 1,0
        """
        self.setProperty(
            "RVPaint", "paint", self.txt.name, "position", gto.FLOAT, (x, y)
        )

    def setTextColor(self, r=1, g=1, b=1, a=1):
        """
        modify color to text # note: this should probably use a tuple like other "set" functions
        """
        self.setProperty(
            "RVPaint", "paint", self.txt.name, "color", gto.FLOAT, (r, g, b, a)
        )

    def setTextSize(self, size):
        """
        modify size of text. Size is a percentage.
        """
        self.setProperty("RVPaint", "paint", self.txt.name, "size", gto.FLOAT, size)

    def setText(self, text, count=0, frame=1):
        """
        Add text to layout mode
        """

        self.txt.newText(text, count=count, frame=frame)

        for k, v in self.txt.__dict__.items():
            valueType = gto.FLOAT
            if k not in ["order", "sourceCount", "name"]:
                if isinstance(v, str):
                    valueType = gto.STRING
                self.setProperty(
                    "RVPaint", "paint", self.txt.name, "{0}".format(k), valueType, v
                )
        # For layout, but not source
        # self.txt.order.append(self.txt.name)
        return self.txt.name

    def setFrameNumberForText(self, frame, setTextName):
        """
        Set the text at this frame number
        """
        self.setProperty(
            "RVPaint",
            "paint",
            "frame:{0:d}".format(int(frame)),
            "order",
            gto.STRING,
            setTextName,
        )


class Retime(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Retime"
        _Node.__init__(self)
        self.maxInputs = 1

    def setVScale(self, scale):
        """
        Set video scale (unit-less scale), scales > 1 make the video
        longer (increase the number of frames).
        """
        self.setProperty("RVRetime", "retime", "visual", "scale", gto.FLOAT, scale)

    def setVOffset(self, offset):
        """
        Set video offset (frames)
        """
        self.setProperty("RVRetime", "retime", "visual", "offset", gto.FLOAT, offset)

    def setAScale(self, scale):
        """
        Set audio scale (unit-less scale), scales > 1 make the
        audio faster (shorter).
        """
        self.setProperty("RVRetime", "retime", "audio", "scale", gto.FLOAT, scale)

    def setAOffset(self, offset):
        """
        Set audio offset (seconds)
        """
        self.setProperty("RVRetime", "retime", "audio", "offset", gto.FLOAT, offset)

    def setTargetFps(self, fps):
        """
        Set target output fps.  inputFPS/outputFPS then provides an
        additional implicit scale factor.
        """
        self.setProperty("RVRetime", "retime", "output", "fps", gto.FLOAT, fps)


class Stack(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Stack"
        _GroupNode.__init__(self)

    #
    #   Utility Functions
    #

    def setWipes(self, on):
        """
        Turn on (or off, according to the boolean argument) Wipes
        mode for this Stack.
        """
        self.setProperty("RVStackGroup", "", "ui", "wipes", gto.INT, on)

    def setCompOp(self, op):
        """
        Set the "compositing operator" used to combine the images in
        this stack for rendering.  Current possible operators are:

        "over"        Traditional compositing "over". Note that all inputs are
                      assumed to be premulted.
        "add"         Just add pixel values
        "difference"  A - B
        "-difference" B - A
        "replace"     Just replace lower stuff in stack with stuff
                      higher up.
        """
        self.setProperty("RVStack", "stack", "composite", "type", gto.STRING, op)


class Switch(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Switch"
        _GroupNode.__init__(self)


class Sequence(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Sequence"
        _GroupNode.__init__(self)

    #
    #   Utility Functions
    #
    #   TODO:
    #   hand-edit edl
    #   use custom in/out rather than those from source


class Text:
    def __init__(self):
        self.sourceCount = 0
        self.position = (0, 0)
        self.color = (1, 1, 1, 1)
        self.size = 0.01
        self.text = ""
        self.order = []

    def newText(self, text="", count=0, frame=1):
        """
        Add a new text subnode to paint. the frame arg is used with Source, but not needed with Layout
        """
        if not count:
            count = self.sourceCount
        self.name = "text:%d:%d:" % (count, frame)
        self.sourceCount += 1
        self.text = text


class Layout(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Layout"
        _GroupNode.__init__(self)
        self.txt = Text()

    #
    #   Utility Functions
    #
    #   TODO:
    #   direct access to 2D transform

    def setLayoutMode(self, mode):
        """
        Current possible layout modes are:

        "packed" Dynamically arrange in a grid
        "row"    Dynamically arrange in a row
        "column" Dynamically arrange in a column
        "manual" Provide the user with interactive manipulators
        "static" Leave the current arrangement alone
        """
        self.setProperty("RVLayoutGroup", "", "layout", "mode", gto.STRING, mode)

    def setTextPosition(self, x, y):
        """
        modify position of text. Positions are x, y values from lower left == 0, upper right == 1
        """
        self.setProperty(
            "RVPaint", "paint", self.txt.name, "position", gto.FLOAT, (x, y)
        )

    def setTextColor(self, r=1, g=1, b=1, a=1):
        """
        modify color to text
        """
        self.setProperty(
            "RVPaint", "paint", self.txt.name, "color", gto.FLOAT, (r, g, b, a)
        )

    def setTextSize(self, size):
        """
        modify size of text. Size is a percentage.
        """
        self.setProperty("RVPaint", "paint", self.txt.name, "size", gto.FLOAT, size)

    def setText(self, text, count=0, frame=1):
        """
        Add text to layout mode
        """

        self.txt.newText(text, count=count, frame=frame)

        for k, v in self.txt.__dict__.items():
            valueType = gto.FLOAT
            if k not in ["order", "sourceCount", "name"]:
                if isinstance(v, str):
                    valueType = gto.STRING
                self.setProperty(
                    "RVPaint", "paint", self.txt.name, "{0}".format(k), valueType, v
                )
        self.txt.order.append(self.txt.name)

    def setFrameNumberForText(self, frame):
        """
        Set the text at this frame number
        """
        self.setProperty(
            "RVPaint",
            "paint",
            "frame:{0:d}".format(int(frame)),
            "order",
            gto.STRING,
            self.txt.order,
        )

    def createTileList(self, numTiles):
        """createTileList(i) -> (scale, (x,y))
        construct optimal tiling for the given number of tiles. The return value
        is a tuple with the following information:
        (scale, [(x1, y1), (x2, y2), ...])
        scale - the scale of the tile relative to the entire image
        x/y - location of the tile relative to the width/height of the image

        optimal tiling is calculated as follows...
        Since every image is the same size and is the same aspect ratio as the
        resulting tiled image, we can simplify the optimal tiling quite a bit.
        Since the tiles are the same aspect ratio as the tiled image, the tiled
        image will be square (same number of rows and columns). To find out how
        many tiles on each side, just take the square root. We have to take
        ceiling of this since we need at least the number of tiles given.
        """
        import math

        nside = int(math.ceil(math.sqrt(numTiles)))

        scale = 1.0 / nside

        pos = []
        total = 0
        for row in reversed(range(0, nside)):
            y = scale * row - (0.5 * scale)
            for column in range(0, nside):
                x = scale * column
                pos.append((x * 2, y))

        return (scale, pos[:numTiles])

    def tile(self, images):
        """tile(listOfImages) -> dict(image, (x,y))
        builds a dictionary that holds the x, y position for text of an image
        Attention: RV does something fancier with it's tiling for the bottom row,
        so it's necessary for the length of the image list to be an even number
        to ensure a rectangular ordering.
        """
        tileDict = {}
        tileList = self.createTileList(len(images))
        scale = tileList[0]
        pos = tileList[1]
        for i in range(0, len(images)):
            img = images[i]
            x, y = pos[i]
            tileDict[img] = (x, y)

        return scale, tileDict


class Folder(_GroupNode):
    def __init__(self):
        """
        Private Constructor: must not be called by other than the Session.
        """
        self.typeName = "Folder"
        _GroupNode.__init__(self)


class Session:
    """
    A Session is contains a bunch of nodes (top-level nodes in RV's
    IP graph), properties of those nodes, connections between those
    nodes, properties for the RVSession object, and properties for
    the several objects that make up the output properties for RVIO.
    """

    class Output(_Node):
        """
        There is only one node of type "Output", managed by the
        session.  Its props can be modified via Session utility
        functions or the setOutputProperty method.
        """

        def __init__(self):
            self.typeName = "Output"
            _Node.__init__(self)

    nodeTypes = {
        "Source": ("RVSourceGroup", Source),
        "Stack": ("RVStackGroup", Stack),
        "Switch": ("RVSwitchGroup", Switch),
        "Sequence": ("RVSequenceGroup", Sequence),
        "Layout": ("RVLayoutGroup", Layout),
        "Retime": ("RVRetimeGroup", Retime),
        "Folder": ("RVFolderGroup", Folder),
    }

    def __init__(self):
        self.sourceCount = 0
        self.properties = {}
        self.nodes = {}

        self.outputGroup = self.Output()
        self.outputGroup.name = "defaultOutputGroup"

        self.setProperty("RVSession", "", "writer", "name", gto.STRING, apiName)
        self.setProperty("RVSession", "", "writer", "version", gto.STRING, apiVersion)

    @staticmethod
    def lookupNodeType(key):

        if key in Session.nodeTypes.keys():
            return Session.nodeTypes[key]

        class foo(_Node):
            def __init__(self):
                self.typeName = key
                _Node.__init__(self)

        return (key, foo)

    def getProperty(self, objType, objName, contName, propName):
        """
        Get the value of a property of the RVsession object.
        """
        return self.properties[(objType, objName, contName, propName)]

    def setProperty(self, objType, objName, contName, propName, valueType, value):
        """
        Set the value of a property of the RVsession object.
        """
        if valueType == gto.STRING:
            if isinstance(value, (list, tuple)):
                for i, v in enumerate(value):
                    value[i] = six.ensure_binary(v)
            elif isinstance(value, six.string_types):
                value = six.ensure_binary(value)

        self.properties[(objType, objName, contName, propName)] = (valueType, value)

    def setOutputProperty(self, objType, objName, contName, propName, valueType, value):
        """
        An RV session has a single instance of the "display stack",
        IP nodes that are involved in the final process of putting
        the pixels on the screen, like final color corrections,
        LUTs, or stereo modes.
        """

        self.outputGroup.setProperty(
            objType, objName, contName, propName, valueType, value
        )

    def newNode(self, nodeType, uiName=None):
        """
        Add a new top-level node to the RV ip graph.  nodeType can be one of
        the nodeTypes in self.nodeTypes, some of which have additional utility
        methods.  If nodeType is not recognised, we assume it is the exact name
        of a objec type as it would appear in a session file. uiName can be any
        string, but usually you'd want it to be unique in this session.  If no
        uiName is provided, it'll be set to the node's internal name.

        Current legal Node Types are: Source, Sequence, Stack,
        Layout, Retime.
        """

        try:
            (objType, cons) = Session.lookupNodeType(nodeType)
            node = cons()
        except:
            raise Exception(
                "Can't construct node of type '%s':" % nodeType, sys.exc_info()[0]
            )

        if nodeType == "Source":
            node.name = "sourceGroup%06d" % self.sourceCount
            self.sourceCount += 1
        else:
            i = 0
            while 1:
                name = "%s%06d" % (nodeType, i)
                if name not in self.nodes.keys():
                    node.name = name
                    break
                i += 1

        if uiName:
            node.setUIName(uiName)
        else:
            node.setUIName(node.name)

        self.nodes[node.name] = node

        return node

    #
    #   Output to session file
    #

    def _getVersionedObj(self, objName, objType):
        """
        Check for a particular node protocol version from the mapping at the
        top of the file. If there is none listed default to version 1.
        """
        return gc.Object(objName, objType, nodeVersions.get(objType, 1))

    def _writeProperties(self, top, obj, objName, properties):
        """
        A property is designated by a GTO object-container-property
        path.  All these properties are associated with the given
        object, but some are not contained in that (group) object.
        IE if subObject is not '', a new GTO object is created to
        hold the property.
        """

        subObjects = {}
        if obj != None:
            subObjects[""] = (obj, {})

        containers = {}

        for (path, info) in sorted(properties.items()):

            (subObjType, subObject, container, prop) = path
            (typeName, value) = info

            if subObject not in subObjects.keys():
                #
                #   This property belongs in a subObject we haven't
                #   created yet, so do it now.
                #
                subName = "%s_%s" % (objName, subObject)
                o = self._getVersionedObj(subName, subObjType)
                top.append(o)
                subObjects[subObject] = (o, {})

            (object, containers) = subObjects[subObject]

            if container not in containers.keys():
                #
                #   This property belongs in a container we haven't
                #   created yet, so do it now.
                #
                c = gc.Component(container, "compinterp")
                object.append(c)
                containers[container] = c

            gtoContainer = containers[container]

            w = 1
            element = value
            if type(element) == type([]):
                element = element[0]
            else:
                value = [value]

            if type(element) == type((0,)):
                w = len(element)

            gtoContainer.append(
                gc.Property(prop, typeName, size=len(value), width=w, data=value)
            )

    def _writeSessionObject(self, top):

        top.rv = self._getVersionedObj("rv", "RVSession")
        self._writeProperties(top, top.rv, "rv", self.properties)

    def _writeDefaultOutput(self, top):

        top.defaultOutputGroup = self._getVersionedObj(
            "defaultOutputGroup", "RVOutputGroup"
        )
        self._writeProperties(
            top,
            top.defaultOutputGroup,
            "defaultOutputGroup",
            self.outputGroup.properties,
        )

    def _writeConnections(self, top):

        #
        #   Input order is significant for stack/sequence/layout
        #   nodes, so preserve it here.
        #

        cons = []
        for node in self.nodes.values():
            for input in node.inputs:
                cons.append([six.ensure_binary(input), six.ensure_binary(node.name)])

        if len(cons) == 0:
            return

        top.connections = self._getVersionedObj("connections", "connection")

        top.connections.evaluation = gc.Component("evaluation", "compinterp")
        top.connections.evaluation.append(
            gc.Property("connections", gto.STRING, size=len(cons), width=2, data=cons)
        )

    def _writeNodes(self, top):
        """
        Write "nodes" (top-level nodes in rv's ip graph).  Sub-nodes
        are created automatically as needed (if they are needed to
        hold properties.

        Source names are significant and must be written into the
        file in lexigraphic order.
        """

        nodeNames = self.nodes.keys()
        nodeNames = sorted(nodeNames)
        for name in nodeNames:
            node = self.nodes[name]
            objType = Session.lookupNodeType(node.typeName)[0]
            top[node.name] = self._getVersionedObj(node.name, objType)

            #   write properties of this node.
            self._writeProperties(top, top[node.name], node.name, node.properties)

    def write(self, filename):
        """
        Write all nodes and connections to a file.  Filename must
        end in ".rv"
        """

        f = gc.gtoContainer()

        self._writeSessionObject(f)
        self._writeConnections(f)
        self._writeDefaultOutput(f)
        self._writeNodes(f)

        f.write(filename, 2)  # 0: Binary 1: Compressed 2: Text

    #
    #   Output Property Utility Functions
    #

    def setOutputStereoType(self, mode):
        """
        Set stereo output mode.  Current allowed modes are:

        "off"      No stereo
        "anaglyph" Magenta/Cyan Anaglyph
        "hardware" Sequential frame display (shutter glasses)
        "checker"  Left/right media in alternating pixels
        "scanline" Left/right media in alternating scanlines
        "left"     Left eye only
        "right"    Right eye only
        "pair"     Left eye on left, Right eye on right
        "mirror"   Like "pair" but with right eye mirrored
        """
        self.setOutputProperty(
            "RVOutputDisplayStereo", "stereo", "stereo", "type", gto.STRING, mode
        )

    def setOutputGamma(self, gamma):
        """
        Set Output Gamma
        """
        args = ["RVDisplayColor", "RVDisplayPipelineGroup", "colorPipeline"]
        pipeNode = self.outputGroup.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.outputGroup.addPipelineNode(*args)
        self.setOutputProperty(
            "RVDisplayColor", pipeNode, "color", "gamma", gto.FLOAT, gamma
        )
        self.setOutputProperty(
            "RVDisplayColor", pipeNode, "color", "active", gto.INT, 1
        )

        return pipeNode

    def setOutputLutName(self, name):
        """
        Set Output LUT filename, and activate.
        """
        args = ["RVDisplayColor", "RVDisplayPipelineGroup", "colorPipeline"]
        pipeNode = self.outputGroup.getPipelineNode(*args)
        if pipeNode == None:
            pipeNode = self.outputGroup.addPipelineNode(*args)
        self.setOutputProperty(
            "RVDisplayColor", pipeNode, "lut", "file", gto.STRING, name
        )
        self.setOutputProperty("RVDIsplayColor", pipeNode, "lut", "active", gto.INT, 1)

        return pipeNode

    #
    #   Session Property Utility Functions
    #

    def setFPS(self, fps):
        """
        Set FPS for the session.  If not set, default FPS comes from
        the preferences.
        """
        self.setProperty("RVSession", "", "session", "fps", gto.FLOAT, fps)

    def setViewNode(self, node):
        """
        The "view node" is the node that RV will be "looking at"
        immediately after the session file is loaded.
        """
        self.setProperty("RVSession", "", "session", "viewNode", gto.STRING, node.name)


def SampleCode():
    """

    #! /usr/bin/python

    import rvSession
    import gto

    def buildAnamorphicStuff (session) :

        dpx = "/tweaklib/media/sequences/lions/vfx018_020_plate_v001.#.dpx"

        src = session.newNode ("Source", "DPX Anamorphic Sequence")
        src.setMedia (dpx)

        src.setIgnoreChromaticities(True)
        src.setAspectRatio(2.0)

        return src

    def buildColorStuff (session) :

        exr = "/tweaklib/media/images/exr/Balls.exr"

        src = session.newNode ("Source", "Exr Image")
        src.setMedia (exr)

        src.setExposure ((3.0,4.0,5.0))

        return src

    def buildRetimeStuff (session) :

        video = "/tweaklib/media/movies/photojpg/upDialog.mov"

        # normal speed
        normal = session.newNode ("Source", "Up Dialog")
        normal.setMedia (video)

        # retime to increase speed by 50%
        retimeUp = session.newNode ("Retime", "Faster")
        retimeUp.addInput (normal)
        retimeUp.setVScale (0.75)

        # retime to decrease speed by 50%
        retimeDown = session.newNode ("Retime", "Slower")
        retimeDown.addInput (normal)
        retimeDown.setVScale (1.5)

        # layout of all three
        layout = session.newNode ("Layout", "Layout of Retimes")
        layout.setLayoutMode ("row")
        layout.addInput (retimeUp)
        layout.addInput (normal)
        layout.addInput (retimeDown)

        return layout

    def buildStereo (session) :

        left  = "/tweaklib/media/movies/photojpg/ct01.left.bs.elem.1k.gamma.mov"
        right = "/tweaklib/media/movies/photojpg/ct01.right.bs.elem.1k.gamma.mov"

        s1 = s.newNode ("Source")
        s1.setUIName ("stereoSource")
        s1.setMedia ([left, right])

        return s1

    def buildStackOfSeq (session) :

        #    Make stack, turn on wipes
        stack = session.newNode ("Stack", "Stack of Sequences")
        stack.setWipes (1)
        stack.setCompOp ("replace")

        #    Make layout
        layout = session.newNode ("Layout", "Layout of Sequences")
        layout.setLayoutMode ("packed")

        #
        #    Make two sequences and connenct them as inputs to the stack
        #    and layout
        #

        seq1 = session.newNode ("Sequence", "Sequence 1")
        stack.addInput (seq1)
        layout.addInput (seq1)

        seq2 = session.newNode ("Sequence", "Sequence 2")
        stack.addInput (seq2)
        layout.addInput (seq2)

        #
        #   Media for the sources
        #

        audio = "/tweaklib/media/audio/woman_16-bit_44KHz_master.wav"
        video1 = "/tweaklib/media/movies/photojpg/hippoNumbered.mov"
        video2 = "/tweaklib/media/movies/photojpg/hippo.mov"

        #
        #   Make two sources, add them to the first sequence
        #   The order of the inputs is the order of the clips in the
        #   sequence view.
        #

        s1 = session.newNode ("Source")
        s1.setUIName ("201-248")
        s1.setMedia ([video1, audio])
        s1.setCutIn (201)
        s1.setCutOut (248)
        s1.setAudioOffset (8.3333)
        seq1.addInput (s1)

        s2 = session.newNode ("Source")
        s2.setUIName ("249-298")
        s2.setMedia ([video1, audio])
        s2.setCutIn (249)
        s2.setCutOut (298)
        s2.setAudioOffset (8.3333)
        seq1.addInput (s2)

        #
        #   Make two more sources, with more delayed audio, and add them
        #   to the second sequence.
        #

        s3 = session.newNode ("Source")
        s3.setUIName ("201-248 late audio")
        s3.setMedia ([video2, audio])
        s3.setCutIn (201)
        s3.setCutOut (248)
        s3.setAudioOffset (8.5333)
        seq2.addInput (s3)

        s4 = session.newNode ("Source")
        s4.setUIName ("249-298 late audio")
        s4.setMedia ([video2, audio])
        s4.setCutIn (249)
        s4.setCutOut (298)
        s4.setAudioOffset (8.5333)
        seq2.addInput (s4)

        return (stack, layout)


    def addTextToSequence(session, arbitraryText='foo'):

        framerange = (1,100)
        movie = "/tweaklib/media/movies/photojpg/hippoNumbered.mov"

        #
        #   Make Sequence
        #
        sequence = session.newNode("Sequence", "Text Sequence")

        s1 = session.newNode ("Source")
        s1.setMedia (movie)

        #
        #   Also adding metadata
        #
        s1.setMetaData ('here is the same text as metadata={0}'.format(arbitraryText))

        #
        #   to make text last longer than one frame, you need to loop it over the desired range.
        #
        for i in range(int(framerange[0]),int(framerange[1])+1):
            textName = s1.setText(arbitraryText, 1, i)
            s1.setFrameNumberForText(i, textName)
            s1.setTextPosition(-0.65,-0.45) # normalized-coordinates 0,0 is the center
            # Turn text red
            #s1.setTextColor (1,0,0)
            s1.setTextSize(0.01)
        sequence.addInput (s1)

        return sequence


    def buildTransition (session, one, two) :

        #
        #   Add transition node.
        #
        #   Wipe between "one" and "two" starting on frame 41, lasting for 20 frames
        #
        t = session.newNode ("Wipe", "Wipe 1")
        t.setProperty ("Wipe", "", "parameters", "startFrame", gto.FLOAT, 41.0)
        t.setProperty ("Wipe", "", "parameters", "numFrames",  gto.FLOAT, 20.0)

        t.addInput (one)
        t.addInput (two)

        return t

    def buildOutputColor (session) :

        #
        #   Add outputGroup color node.
        #
        o = session.setOutputGamma(2.2)

        return o

    #######################################################################

    #
    #   Set this to whatever you want to view from the possibilities
    #   below.  Note that all this stuff is in the file no matter what,
    #   these options just setup the inital view (when the session file is
    #   loaded).
    #
    view = "text"

    s = rvSession.Session()
    s.setFPS (24.0)

    #
    #   Anamorphic sample
    #
    anamorphicView = buildAnamorphicStuff (s)

    #
    #   Color settings sample
    #
    colorView = buildColorStuff (s)

    #
    #   Stereo sample
    #
    stereoView = buildStereo (s)

    #
    #   Stack of sequences and Layout of Sequences samples
    #
    (SoSview, LoSview) = buildStackOfSeq (s)

    #
    #   Some retime nodes
    #
    retimeView = buildRetimeStuff (s)

    #
    #   Some text addition nodes
    #
    sequenceTextView = addTextToSequence (s)

    #
    #   A Editorial Transition (wipe)
    #
    buildTransition(s, SoSview, LoSview)

    #
    #   A Output Color node for use by RVIO
    #
    buildOutputColor(s)

    if   (view == "color") :
        s.setDisplayLutName ("/tweaklib/luts/csp/up1stop.csp")
        s.setViewNode (colorView)

    if   (view == "stereo") :
        s.setStereoType ("anaglyph")
        s.setViewNode (stereoView)

    elif (view == "SoS") :
        s.setViewNode (SoSview)

    elif (view == "LoS") :
        s.setViewNode (LoSview)

    elif (view == "retime") :
        s.setViewNode (retimeView)

    elif (view == "text") :
        s.setViewNode (sequenceTextView)


    s.write ("sample.rv")
    """
    pass
