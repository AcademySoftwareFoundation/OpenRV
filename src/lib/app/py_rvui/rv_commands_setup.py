#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import os
import sys
import rv.commands
import rv.extra_commands
import rv.runtime
from pymu import MuSymbol

all_mu_commands = [
    "insertByteProperty",
    "showConsole",
    "newImageSource",
    "getCurrentImageChannelNames",
    "fullScreenMode",
    "remoteSendDataEvent",
    "remoteNetworkStatus",
    "fps",
    "reload",
    "loadChangedFrames",
    "imagesAtPixel",
    "setSessionName",
    "remoteContacts",
    "frameEnd",
    "sequenceOfFile",
    "center",
    "close",
    "cacheDir",
    "audioTextureID",
    "audioTextureComplete",
    "viewNode",
    "getCurrentPixelValue",
    "popEventTable",
    "nodes",
    "audioCacheMode",
    "realFPS",
    "encodePassword",
    "bindings",
    "markFrame",
    "stopTimer",
    "setFrame",
    "stop",
    "undoHistory",
    "margins",
    "remoteSendEvent",
    "showNetworkDialog",
    "setFPS",
    "setOutPoint",
    "nodeConnections",
    "cacheMode",
    "eventToCameraSpace",
    "mbps",
    "resetMbps",
    "imageGeometryByIndex",
    "isMenuBarVisible",
    "newProperty",
    "newNDProperty",
    "setInPoint",
    "narrowToRange",
    "isTimerRunning",
    "setMargins",
    "addSources",
    "updateLUT",
    "narrowedFrameStart",
    "saveFileDialog",
    "setHardwareStereoMode",
    "setNodeInputs",
    "clearHistory",
    "deleteProperty",
    "isBuffering",
    "skipped",
    "frame",
    "sourceAttributes",
    "sourceDataAttributes",
    "renderedImages",
    "outPoint",
    "sessionFileName",
    "setSessionFileName",
    "sessionName",
    "openFileDialog",
    "setFiltering",
    "bgMethod",
    "remoteConnect",
    "setViewSize",
    "setStringProperty",
    "properties",
    "redo",
    "elapsedTime",
    "setFloatProperty",
    "readCDL",
    "readLUT",
    "sendInternalEvent",
    "endCompoundCommand",
    "activeEventTables",
    "testNodeInputs",
    "metaEvaluate",
    "commandLineFlag",
    "unbind",
    "inPoint",
    "setHalfProperty",
    "saveSession",
    "unbindRegex",
    "videoState",
    "setFrameStart",
    "redoHistory",
    "metaEvaluateClosestByType",
    "playMode",
    "exportCurrentFrame",
    "newNode",
    "startTimer",
    "decodePassword",
    "fileKind",
    "setInc",
    "sourceMediaInfo",
    "cacheInfo",
    "loadCount",
    "getCurrentNodesOfType",
    "prefTabWidget",
    "frameStart",
    "isMarked",
    "previousViewNode",
    "getHalfProperty",
    "setSourceMedia",
    "viewNodes",
    "eval",
    "setIntProperty",
    "isFullScreen",
    "nodesOfType",
    "sessionFromUrl",
    "nodeGroup",
    "nodeGroupRoot",
    "isRealtime",
    "isCurrentFrameIncomplete",
    "redoPathSwapVars",
    "sourceDisplayChannelNames",
    "sourcesRendered",
    "bindingDocumentation",
    "setDriverAttribute",
    "isCaching",
    "getCurrentImageSize",
    "putUrlOnClipboard",
    "eventToImageSpace",
    "loadTotal",
    "myNetworkPort",
    "presentationMode",
    "httpPost",
    "setSessionType",
    "insertFloatProperty",
    "inputAtPixel",
    "setRealtime",
    "deleteNode",
    "ioFormats",
    "getStringProperty",
    "nextViewNode",
    "setCursor",
    "setRendererType",
    "narrowedFrameEnd",
    "contentAspect",
    "sourcesAtFrame",
    "propertyExists",
    "getFiltering",
    "theTime",
    "getIntProperty",
    "insertStringProperty",
    "releaseAllCachedImages",
    "getFloatProperty",
    "sources",
    "toggleMenuBar",
    "sourceNameWithoutFrame",
    "queryDriverAttribute",
    "nodesInGroup",
    "ioParameters",
    "propertyInfo",
    "setPresentationMode",
    "optionsPlay",
    "addSource",
    "addSourceBegin",
    "addSourceEnd",
    "addSourceVerbose",
    "addSourcesVerbose",
    "setProgressiveSourceLoading",
    "progressiveSourceLoading",
    "setBGMethod",
    "activeModes",
    "optionsNoPackages",
    "isCurrentFrameError",
    "alertPanel",
    "stereoSupported",
    "nodeImageGeometry",
    "nodeRangeInfo",
    "resizeFit",
    "startupResize",
    "mapPropertyToGlobalFrames",
    "isModeActive",
    "setFrameEnd",
    "play",
    "newSession",
    "getSessionType",  # "javascriptMuExport",
    "sessionNames",
    "sourceMedia",
    "setAudioCacheMode",
    "exportCurrentSourceFrame",
    "setEventTableBBox",
    "optionsProgressiveLoading",
    "getRendererType",
    "pushEventTable",
    "networkAccessManager",
    "remoteNetwork",
    "isConsoleVisible",
    "undo",
    "openUrl",
    "openMediaFileDialog",
    "insertHalfProperty",
    "newImageSourcePixels",
    "remoteDisconnect",
    "undoPathSwapVars",
    "deactivateMode",
    "remoteSendMessage",
    "redraw",
    "insertIntProperty",
    "isPlaying",
    "imageToEventSpace",
    "scrubAudio",
    "defineMinorMode",
    "cacheSize",
    "addToSource",
    "activateMode",
    "remoteApplications",
    "markedFrames",
    "beginCompoundCommand",
    "setCacheMode",
    "sourcePixelValue",
    "framebufferPixelValue",   
    "setByteProperty",
    "inc",
    "getCurrentAttributes",
    "httpGet",
    "closestNodesOfType",
    "remoteConnections",
    "viewSize",
    "watchFile",
    "remoteLocalContactName",
    "setRemoteLocalContactName",
    "remoteDefaultPermission",
    "setRemoteDefaultPermission",
    "flushCachedNode",
    "nodeExists",
    "relocateSource",
    "contractSequences",
    "existingFilesInSequence",
    "existingFramesInSequence",
    "setViewNode",
    "imageGeometry",
    "clearAllButFrame",
    "setWindowTitle",
    "nodeType",
    "getByteProperty",
    "sourceAtPixel",
    "clearSession",
    "setPlayMode",
    "myNetworkHost",
    "sourceMediaInfoList",
    "packageListFromSetting",
    "shortAppName",
    "showTopViewToolbar",
    "showBottomViewToolbar",
    "isTopViewToolbarVisible",
    "isBottomViewToolbarVisible",
    "currentFrameStatus",
    "editNodeSource",
    "nodeTypes",
    "updateNodeDefinition_",
    "writeAllNodeDefinitions",
    "writeNodeDefinition",
    "videoDeviceIDString",
    "readProfile",
    "writeProfile",
    "editProfiles",
    "spoofConnectionStream",
    "sourceGeometry",
    "ocioUpdateConfig",
    "cacheOutsideRegion",
    "setCacheOutsideRegion",
    "licensingState",
    "releaseAllUnusedImages",
    "launchTLI",
    "validateShotgunToken",
    "rvioSetup",
    "imageGeometryByTag",
    "hopProfDynName",
    "logMetrics",
    "logMetricsWithProperties",
    "getVersion",
    "getReleaseVariant",
    "isDebug",
    "setFilterLiveReviewEvents",
    "filterLiveReviewEvents",
    "crash",
    "addSourceMediaRep",
    "setActiveSourceMediaRep",
    "sourceMediaRep",
    "sourceMediaReps",
    "sourceMediaRepSwitchNode",
    "sourceMediaRepSourceNode",
    "sourceMediaRepsAndNodes"
]


all_extra_commands = [
    "popInputToTop",
    "stepBackward10",
    "reloadInOut",
    "loadCurrentSourcesChangedFrames",
    "nodesInEvalPath",
    "setUIName",
    "displayFeedback2",
    "inputNodeUserNameAtFrame",
    "sourceMetaInfoAtFrame",
    "sourceImageStructure",
    "stepBackward100",
    "setTranslation",
    "stepForward100",
    "isPlayingForwards",
    "setInactiveState",
    "stepBackward",
    "activatePackageModeEntry",
    "isViewNode",
    "toggleFilter",
    "topLevelGroup",
    "deactivatePackageModeEntry",
    "set",
    "associatedNode",
    "cycleNodeInputs",
    "toggleFullScreen",
    "currentImageAspect",
    "frameImage",
    "activateSync",
    "cacheUsage",
    "toggleRealtime",
    "toggleMotionScope",
    "findAnnotatedFrames",
    "isNarrowed",
    "isPlayable",
    "isPlayingBackwards",
    "scale",
    "toggleMotionScopeFromState",
    "setScale",
    "recordPixelInfo",
    "setActiveState",
    "displayFeedback",
    "togglePlay",
    "sequenceBoundaries",
    "togglePlayIfNoScrub",
    "isSessionEmpty",
    "togglePlayVerbose",
    "associatedNodes",
    "toggleForwardsBackwards",
    "translation",
    "stepForward1",
    "stepForward10",
    "sourceFrame",
    "nodesUnderPointer",
    "centerResizeFit",
    "numFrames",
    "toggleSync",
    "stepBackward1",
    "uiName",
    "stepForward",
    "cprop",
    "nodesInGroupOfType",
    "appendToProp",
    "removeFromProp",
    "existsInProp",
    "associatedVideoDevice",
    "updatePixelInfo",
    "setDisplayProfilesFromSettings",
    "minorModeIsLoaded",
]

##
##  This is a stop-gap, since I can't figure out a sensible way to find
##  these values programmatically.
##
##  There is no sensible way!
##

commands_int_constants = [
    ("OneFileName", 3),
    ("EDLFileKind", 7),
    ("UncheckedMenuState", 1),
    ("StackSession", 1),
    ("CursorArrow", 0),
    ("ShortType", 7),
    ("WarningAlert", 1),
    ("ImageFileKind", 1),
    ("OneDirectory", 4),
    ("PlayPingPong", 2),
    ("NeutralMenuState", 0),
    ("ManyExistingFilesAndDirectories", 2),
    ("CacheBuffer", 1),
    ("ManyExistingFiles", 1),
    ("HalfType", 5),
    ("RGB709", 0),
    ("StringType", 8),
    ("MixedStateMenuState", 3),
    ("NetworkStatusOn", 1),
    ("FloatType", 1),
    ("NetworkStatusOff", 0),
    ("CacheOff", 0),
    ("ErrorAlert", 2),
    ("IntType", 2),
    ("PlayLoop", 0),
    ("CursorDefault", 0),
    ("CDLFileKind", 3),
    ("LUTFileKind", 4),
    ("MovieFileKind", 2),
    ("PlayOnce", 1),
    ("RVFileKind", 6),
    ("CursorNone", 10),
    ("InfoAlert", 0),
    ("SequenceSession", 0),
    ("DisabledMenuState", -1),
    ("CacheGreedy", 2),
    ("DirectoryFileKind", 5),
    ("UnknownFileKind", 0),
    ("ByteType", 6),
    ("CIEXYZ", 1),
    ("CheckedMenuState", 2),
    ("OneExistingFile", 0),
    ("IndependentDisplayMode", 0),
    ("MirrorDisplayMode", 1),
    ("NotADisplayMode", 2),
    ("VideoAndDataFormatID", 1),
    ("DeviceNameID", 4),
    ("ModuleNameID", 5),
    ("NetworkPermissionAsk", 0),
    ("NetworkPermissionAllow", 1),
    ("NetworkPermissionDeny", 2),
]


def bind_symbols(symbol_list, module_name, mod):
    for sym in symbol_list:
        try:
            s = MuSymbol(module_name + "." + sym)
            setattr(mod, sym, s)
        except:
            print("Bind to python failed:", sym)


def bind_constants(constant_list, mod):
    for const_pair in constant_list:
        try:
            setattr(mod, const_pair[0], const_pair[1])
        except:
            print("Bind to python failed:", sym)


bind_symbols(all_mu_commands, "commands", rv.commands)
bind_symbols(all_extra_commands, "extra_commands", rv.extra_commands)
bind_symbols(["eval"], "runtime", rv.runtime)
bind_constants(commands_int_constants, rv.commands)

rv.extra_commands.displayFeedback = rv.extra_commands.displayFeedback2

if "RV_NO_CONSOLE_REDIRECT" not in os.environ:

    class RVStdOut:
        def __init__(self):
            self.write = MuSymbol("extra_commands._print")

        def flush(self):
            pass

    sys.stdout = RVStdOut()
    sys.stderr = sys.stdout
