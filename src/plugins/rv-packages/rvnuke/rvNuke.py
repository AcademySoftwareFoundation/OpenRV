#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from __future__ import print_function

import nuke
import nukescripts
import os
import re
import platform
import subprocess
import rvNetwork
import time
import sys
import select
import threading

protocolVersion = 115

rvmon = None

rvmonProc = None

doDebug = False
if os.getenv("RV_NUKE_DEBUG") is not None:
    doDebug = True


def log(str):
    if doDebug:
        print("%s\n" % str, file=sys.stderr)


# log ("************ locals  %s" % str(locals()))
# log ("************ globals %s" % str(globals()))

log("************* protocolVersion %d" % protocolVersion)


def encodeNL(str):
    return str.replace("\n", "#NL#")


def initRvMon():
    global rvmon
    if not rvmon:
        rvmon = RvMonitor()

    return rvmon


def pythonHandler(eventContents):
    try:
        log("pythonHandler exec '%s'" % eventContents)
        exec(eventContents, globals(), locals())
        # nuke.executeInMainThread (nuke.selectPattern)
    except Exception:
        log("Remote python exec error: %s\n" % sys.exc_info()[1])


class RvMonitor:
    class LockedFifo:
        def __init__(self):
            self.first = ()
            self.lock = threading.Lock()

        def append(self, data):
            self.lock.acquire()
            node = [data, ()]
            if self.first:
                self.last[1] = node
            else:
                self.first = node
            self.last = node
            self.lock.release()

        def pop(self):
            #
            #  don't lock unless we have to
            #
            node = self.first
            if node == ():
                return ()

            self.lock.acquire()
            node = self.first
            if node != ():
                self.first = node[1]
                ret = node[0]
            else:
                ret = ()
            self.lock.release()
            return ret

        def clear(self):
            while self.pop() != ():
                pass

    class LockedFlag:
        def __init__(self):
            self.lock = threading.Lock()
            self.value = False

        def set(self):
            self.lock.acquire()
            self.value = True
            self.lock.release()

        def unset(self):
            self.lock.acquire()
            self.value = False
            self.lock.release()

        def isSet(self):
            self.lock.acquire()
            ret = self.value
            self.lock.release()

            return ret

    def __init__(self):
        self.rvc = rvNetwork.RvCommunicator("Nuke")
        self.port = 45128
        self.initialized = False
        self.running = False
        self.selectedNode = None

        self.commands = self.LockedFifo()
        self.crashFlag = self.LockedFlag()

        self.sessionDir = ""
        self.syncSelection = False
        self.syncFrameChange = False
        self.syncReadChanges = False
        self.rvExecPath = ""

        self.updateFromPrefs()

        self.portFile = self.sessionDir + "/rv" + str(os.getpid())

        self.zoomTargetNode = None

        log("adding callbacks")
        # nuke.addUpdateUI (self.updateUI)
        nuke.addKnobChanged(self.knobChanged)
        nuke.addOnCreate(self.onCreate)
        nuke.addOnDestroy(self.onDestroy)
        nuke.addOnScriptClose(self.onScriptClose)
        nuke.addOnScriptLoad(self.onScriptLoad)

    def __del__(self):
        #   Squirrel away a ref to the Popen object because it has a bug that
        #   will print an error message when we delete it if we do it at this
        #   point.  This prevents it from getting garbage collected until later.

        global rvmonProc
        rvmonProc = self.rvProc

    def restoreBegin(self):
        log("restoreBegin")

        nuke.removeKnobChanged(self.knobChanged)
        nuke.removeOnCreate(self.onCreate)
        nuke.removeOnDestroy(self.onDestroy)

    def restoreComplete(self):
        log("restoreComplete")

        nuke.addKnobChanged(self.knobChanged)
        nuke.addOnCreate(self.onCreate)
        nuke.addOnDestroy(self.onDestroy)

        self.allSyncUpdate()

    def updateFromPrefs(self):
        log("rvMonitor updateFromPrefs")

        rvPrefs = RvPreferences()

        self.rvExecPath = rvPrefs.prefs["rvExecPath"]
        self.extraArgs = rvPrefs.prefs["extraArgs"]

        rvSettings = RvSettings(rvPrefs.prefs["sessionDirBase"])

        self.sessionDir = rvSettings.settings["sessionDir"]
        log("   sessionDir '%s'" % self.sessionDir)
        self.syncSelection = rvSettings.settings["syncSelection"]
        self.syncFrameChange = rvSettings.settings["syncFrameChange"]
        self.syncReadChanges = rvSettings.settings["syncReadChanges"]
        self.outputFileFormat = rvSettings.settings["outputFileFormat"]

    def onScriptClose(self):
        log("onScriptClose")
        nuke.removeOnDestroy(self.onDestroy)
        nuke.removeKnobChanged(self.knobChanged)

        if self.rvc.connected:
            log("    disconnecting from RV")
            # self.rvc.disconnect()
            self.running = False
            log("    sleeping after disconnect")
            time.sleep(0.3)

    def onScriptLoad(self):
        log("onScriptLoad")
        """
        XXX
        On script load want to disconnect and shutdown rv,
        since incoming script may have different sessionDir, etc.
        """

    def onCreate(self):
        log("onCreate %s" % nuke.thisNode().name())

        node = nuke.thisNode()
        if not node or type(node).__name__ != "Node":
            return

        if self.syncReadChanges and (str(node.Class()) == "Read" or str(node.Class()) == "Write"):
            self.queueCommand('self.viewReadsInRv(["%s"])' % node.name())

    def onDestroy(self):
        log("onDestroy")
        node = nuke.thisNode()
        if node and type(node).__name__ == "Node":
            log("destroying node of class '%s'" % str(node.Class()))
            if self.syncReadChanges and (str(node.Class()) == "Read" or str(node.Class()) == "Write"):
                self.queueCommand("self.removeObsoleteReads()")

    def updateAndSyncSelection(self, force=False):
        nodes = nuke.selectedNodes()

        if len(nodes) == 1:
            node = nodes[0]

            if node.Class() == "Viewer":
                node = node.input(int(nuke.knob(node.name() + ".input_number")))

            if not node:
                return

            log("updateAndSyncSelection old %s new %s" % (self.selectedNode, node.name()))
            if force or node.name() != self.selectedNode:
                self.selectedNode = node.name()
                self.queueCommand("self.selectCurrentByNodeName()")
                self.queueCommand("self.changeFrame(%d)" % nuke.frame())
        else:
            self.selectedNode = None

    def knobChanged(self):
        nname = ""
        kname = ""
        if doDebug:
            if nuke.thisNode():
                try:
                    log("type '%s'" % type(nuke.thisNode()).__name__)
                    log("class '%s'" % nuke.thisNode().Class())
                    nname = nuke.thisNode().name()
                except Exception:
                    pass
            if nuke.thisKnob():
                kname = nuke.thisKnob().name()
            log("knobChanged node '%s' knob '%s' val '%s'\n" % (nname, kname, str(nuke.thisKnob().value())))

        node = nuke.thisNode()
        knob = nuke.thisKnob()
        if not node or not knob:
            return

        if type(node).__name__ != "Node" and type(node).__name__ != "Root" and type(node).__name__ != "Viewer":
            return

        #
        #  Track selection changes
        #
        if self.syncSelection and knob.name() == "selected" and self.running:
            log("selection changed")
            self.updateAndSyncSelection()

        #
        #   Track frame changes
        #
        elif self.syncFrameChange and str(node.Class()) == "Viewer" and knob.name() == "frame" and self.running:
            log("frame change")
            self.queueCommand("self.changeFrame(%d)" % knob.value())

        #
        #    Track read/write changes
        #
        elif (
            self.syncReadChanges
            and (str(node.Class()) == "Read" or str(node.Class()) == "Write")
            and self.running
            and (
                knob.name() == "colorspace"
                or knob.name() == "first"
                or knob.name() == "last"
                or knob.name() == "label"
                or knob.name() == "frame_mode"
                or knob.name() == "frame"
                or knob.name() == "file"
            )
        ):
            log("read/write knob changed")
            self.queueCommand('self.viewReadsInRv(["%s"])' % node.name())

        #
        #    Track read/write name changes
        #
        elif (
            self.syncReadChanges
            and (str(node.Class()) == "Read" or str(node.Class()) == "Write")
            and self.running
            and (knob.name() == "name")
        ):
            log("read/write name changed")
            self.queueCommand("self.removeObsoleteReads()")
            self.queueCommand('self.viewReadsInRv(["%s"])' % node.name())

        #
        #    Track settings changes
        #
        elif str(node.Class()) == "Root" and re.match("^rvSettings_.*", knob.name()):
            log("setting changed")
            self.updateFromPrefs()
            if self.running:
                if knob.name() == "rvSettings_sessionDir":
                    log("sessionDir setting changed: '%s'" % self.sessionDir)
                    log("restarting RV")
                    self.running = False
                    while self.thread.isAlive():
                        time.sleep(0.1)
                    rvmon.initializeRvSide()
                else:
                    self.allSyncUpdate()

        #
        #    Implement delayed "frame on node"
        #
        elif self.zoomTargetNode and knob.name() == "ypos":
            self.zoomToNode()

    def changeFrame(self, f):
        """ """
        self.ensureConnection()
        self.rvc.remoteEval(
            """
        require commands;
        commands.setFrame (%d); commands.redraw();
        """
            % f
        )

    def selectCurrentByNodeName(self):
        """ """
        self.ensureConnection()
        self.rvc.remoteEval(
            """
        require rvnuke_mode;
        rvnuke_mode.theMode().selectCurrentByNodeName ("%s");
        """
            % self.selectedNode
        )

    def spawn(self):
        log("spawn")

        self.ensureConnection()
        if not self.rvc.connected:
            self.running = False
            log("connect in spawn failed")
            self.initialized = False
            return

        while self.running:
            if not self.rvc.connected:
                log("not connected in spawn loop")
                self.running = False
                self.crashFlag.set()
                break

                #
                #   Send messages to RV
                #
                # log ("send messages to RV")
            try:
                while 1:
                    cmd = self.commands.pop()
                    if cmd == ():
                        break
                    log("running queued command '%s'" % cmd)
                    self.eval(cmd)
            except Exception:
                log("can't send messages to RV, shutting down.")
                try:
                    self.rvc.disconnect()
                except Exception:
                    pass
                self.running = False
                self.crashFlag.set()
                log("disconnected, crashed = True")

            # log ("done sending messages to RV")

            #
            #   Wait for messages from RV
            #
            try:
                # log ("selecting")
                select.select([self.rvc.sock], [], [], 0.1)
                # log ("selecting done")
            except Exception:
                log("rvc select/wait error: %s" % sys.exc_info()[1])

            #
            #   Handle messages from RV
            #
            try:
                # log ("process rv events")
                self.rvc.processEvents()
            except Exception:
                log("can't process messages from the RV side, shutting down.")
                try:
                    self.rvc.disconnect()
                except Exception:
                    pass
                self.running = False
                self.crashFlag.set()
                log("disconnected, crashed = True")
                break

            #
            #   Clear any queued commands
            #
        self.commands.clear()

        try:
            log("polling rv process")
            self.rvProc.poll()
            if not self.rvProc.returncode and self.rvc.connected:
                log("terminating rv process")
                self.rvc.remoteEval(
                    """
                    require rvnuke_mode;
                    rvnuke_mode.theMode().exit();
                """
                )
                self.rvc.connected = False
                # this stuff doesn't work in python 2.5.1 (Nuke 6.1)
                # self.rvProc.terminate()
                # self.rvProc.kill()
                # self.rvProc.wait()
                # log ("returncode %s" % self.rvProc.returncode))
        except Exception:
            log("error terminating RV: %s\n" % sys.exc_info()[1])

        self.initialized = False

    def ensureConnection(self, force=False):
        if self.rvc.connected:
            return

        log("ensureConnection crashed %d force %s" % (self.crashFlag.isSet(), force))

        """
	if (self.crashFlag.isSet() and not force) :
	    log ("prev crashed and not force, so returning")
	    return
	"""

        self.crashFlag.unset()

        args = [
            "-flags",
            "ModeManagerPreload=rvnuke_mode",
            "-network",
            "-networkPort",
            str(self.port),
            "-flags",
            "rvnukePortFile=%s" % self.portFile,
        ]

        usingBuild = False
        if self.rvExecPath != "":
            cmd = [self.rvExecPath]
        else:
            try:
                src = os.environ["SRC_ROOT"]
                usingBuild = True
            except Exception:
                pass

            cmd = [src + "/build/run", "--p", src + "/bin/nsapps/RV64"]

        if not usingBuild and not os.path.exists(self.rvExecPath):
            nuke.executeInMainThread(
                nuke.message,
                (
                    "Sorry '%s' does not exist, please use the 'RV Preferences' menu item or toolbar button to designate the RV executable."
                    % self.rvExecPath,
                ),
            )
            return

        fullcmd = cmd + self.extraArgs.split() + args
        log("starting cmd %s" % str(fullcmd))
        try:
            os.remove(self.portFile)
        except Exception:
            pass

        try:
            self.rvProc = subprocess.Popen(fullcmd)
            self.rvProc.poll()
            if self.rvProc.returncode:
                log("ERROR: failed to start RV '%s' return: %d" % (cmd[0], self.rvProc.returncode))
                print(
                    "ERROR: failed to start RV '%s' return: %d\n" % (cmd[0], self.rvProc.returncode),
                    file=sys.stderr,
                )
                return
        except Exception:
            log("ERROR: failed to start RV '%s'" % cmd[0])
            print("ERROR: failed to start RV '%s'\n" % cmd[0], file=sys.stderr)
            return

        self.port = 0
        for i in range(300):
            if os.path.exists(self.portFile):
                f = open(self.portFile)
                self.port = int(f.read())
                f.close()
                break
            else:
                time.sleep(0.1)

        log("got port: %d" % self.port)
        if self.port == 0:
            log("ERROR: failed to connect to running RV")
            print("ERROR: failed to connect to running RV\n", file=sys.stderr)
            return

        try:
            os.remove(self.portFile)
        except Exception:
            pass

        if self.port != 0:
            log("connecting")
            self.rvc.connect("127.0.0.1", self.port)
            if not self.rvc.connected:
                log("connect failed")

            self.rvc.handlers["remote-python-eval"] = pythonHandler

    def queueCommand(self, cmd):
        log("queueCommand '%s'" % cmd)
        self.commands.append(cmd)

    def setSessionDir(self):
        #    XXX check running ?
        self.rvc.remoteEval(
            """
            require rvnuke_mode;
            rvnuke_mode.theMode().initFromNuke ("%s", %d, "%s");
        """
            % (self.sessionDir, protocolVersion, nuke.env["ExecutablePath"])
        )

    def allSyncUpdate(self):
        if not self.running:
            return

        if self.syncReadChanges:
            readNodes = [
                n.name()
                for n in nuke.allNodes()
                if (type(n).__name__ == "Node" and (n.Class() == "Read" or n.Class() == "Write"))
            ]
            if readNodes:
                self.queueCommand("self.viewReadsInRv(%s)" % str(readNodes))

            self.queueCommand("self.removeObsoleteReads()")

        if self.syncSelection:
            self.updateAndSyncSelection(True)

        if self.syncFrameChange:
            self.queueCommand("self.changeFrame(%d)" % nuke.frame())

    def prepForRender(self, node, renderType):
        log("prepForRender %s %s" % (node.name(), renderType))

        baseDir = self.sessionDir + "/" + renderType

        dateString = time.strftime("%b %d %H:%M:%S")
        if renderType == "checkpoint":
            dateStringForName = "_" + re.sub(" ", "_", re.sub(":", "_", dateString))
        else:
            dateStringForName = ""

        nodeDir = baseDir + "/" + node.name() + dateStringForName

        if not os.path.exists(nodeDir):
            log("making directory '%s'" % nodeDir)
            os.makedirs(nodeDir)

        nuke.scriptSave(nodeDir + "/" + renderType + ".nk")

        return dateString

    def renderTmpFiles(self, node, seqDir, start, end, incr):
        log("renderTmpFiles %s %s %s %s %s" % (node.name(), seqDir, start, end, incr))

        """
        if (start == end) :
            targetPath = seqDir + "/" + node.name() + (".%d." % start) + self.outputFileFormat
        else :
            targetPath = seqDir + "/" + node.name() + ".#." + self.outputFileFormat
        """
        targetPath = seqDir + "/" + node.name() + ".#." + self.outputFileFormat

        log("targetPath %s" % targetPath)

        #  XXX handle proxy renders
        fieldname = "file"
        """
        if self.proxy:
            fieldname = "proxy"
        """
        nuke.Undo().disable()
        writeNode = nuke.nodes.Write(tile_color="0xff000000")
        writeNode[fieldname].setValue(targetPath)

        writeNode.setInput(0, node)

        ret = True

        try:
            nuke.executeMultiple((writeNode,), ([start, end, incr],))
        except Exception as msg:
            nuke.message("Render failed:\n%s" % (msg,))
            ret = False

        log("deleting writeNode")
        nuke.delete(writeNode)

        nuke.Undo().enable()

        return ret

    def raiseRv(self):
        self.ensureConnection()
        self.rvc.remoteEval(
            """
        require rvnuke_mode;
        rvnuke_mode.theMode().raiseMainWindow();
        """
        )

    def initializeRvSide(self):
        """
        Make sure rv is running, connecter is looping in separate thread, and send
        session dir to rv.
        """
        if self.running:
            self.queueCommand("self.raiseRv()")
            return

        self.updateFromPrefs()

        sessionDir = self.sessionDir

        if not os.path.exists(sessionDir):
            log("making directory '%s'" % sessionDir)
            try:
                os.makedirs(sessionDir)
            except Exception:
                pass

        self.queueCommand("self.setSessionDir()")

        if not self.running:
            self.running = True
            self.thread = threading.Thread(None, self.spawn)
            log("thread start")
            self.thread.start()
            log("sleep thread start")
            # time.sleep(2)
            # log ("done sleep")

        self.allSyncUpdate()

        self.initialized = True

    def initRenderRun(
        self,
        outputNode,
        startFrame,
        endFrame,
        incFrame,
        audioFile,
        audioOffset,
        cpFrame,
        label,
        date,
        renderType,
        stereo,
    ):
        """
        Notify RV that a flip render has started.  RV
        should start monitoring the target directory and loading new/changed files.
        If the corresponding source has not been created yet, or has been deleted, RV
        should wait until the first frame is created before creating the source.
        """
        self.ensureConnection()
        self.rvc.remoteEval(
            """
            require rvnuke_mode;
            rvnuke_mode.theMode().initRenderRun ("%s", %d, %d, %d, "%s", %g, %d, "%s", "%s", "%s", "%s", "%s");
        """
            % (
                outputNode,
                startFrame,
                endFrame,
                incFrame,
                audioFile,
                audioOffset,
                cpFrame,
                self.outputFileFormat,
                label,
                date,
                renderType,
                stereo,
            )
        )

    def removeObsoleteReads(self):
        """
        Remove any sources corresponding to Reads or Writes that
        have been deleted.
        """

        readNodes = [
            n.name()
            for n in nuke.allNodes()
            if (type(n).__name__ == "Node" and (n.Class() == "Read" or n.Class() == "Write"))
        ]
        if not readNodes:
            readsStr = "string[] {}"
        else:
            readsStr = 'string[] {"%s"' % readNodes[0]
            for i in range(1, len(readNodes)):
                readsStr += ', "%s"' % readNodes[i]
            readsStr += "}"

        log("    readsStr %s" % readsStr)

        remote = "require rvnuke_mode; rvnuke_mode.theMode().removeObsoleteReads (" + readsStr + ");"

        log("    remote %s" % remote)
        self.rvc.remoteEval(remote)

    def viewReadsInRv(self, readNodes):
        """
        Add a source to the session, a sequence held by a file input node.
        """

        names = []
        media = []
        firsts = []
        lasts = []
        spaces = []
        labels = []
        classes = []
        offsets = []
        for n in readNodes:
            node = nuke.toNode(n)
            if not node:
                continue
            names.append(node.name())
            media.append(nuke.filename(node))
            # firsts.append (node["first"].value())
            # lasts.append  (node["last"].value())
            firsts.append(node.firstFrame())
            lasts.append(node.lastFrame())
            spaces.append(re.sub("default \((.*)\)", "\g<1>", node["colorspace"].value()))
            labels.append(encodeNL(node["label"].value()))
            classes.append(node.Class())
            ro = "0"
            if node["frame_mode"].value() == "offset" and node["frame"].value() != "":
                ro = str(-int(node["frame"].value()))
            offsets.append(ro)

        log("    names %s" % names)
        log("    spaces %s" % spaces)

        nameStr = mediaStr = spaceStr = labelStr = classStr = "string[] {"
        firstStr = lastStr = offsetStr = "int[] {"
        first = True
        for i in range(len(names)):
            if first:
                comma = ""
                first = False
            else:
                comma = ","

            nameStr += comma + '"%s"' % names[i]
            mediaStr += comma + '"%s"' % media[i]
            spaceStr += comma + '"%s"' % spaces[i]
            firstStr += comma + "%d" % int(firsts[i])
            lastStr += comma + "%d" % int(lasts[i])
            labelStr += comma + '"%s"' % labels[i]
            classStr += comma + '"%s"' % classes[i]
            offsetStr += comma + "%s" % int(offsets[i])

        nameStr += "}"
        mediaStr += "}"
        spaceStr += "}"
        firstStr += "}"
        lastStr += "}"
        labelStr += "}"
        classStr += "}"
        offsetStr += "}"

        log("    nameStr %s" % nameStr)

        remote = "require rvnuke_mode; rvnuke_mode.theMode().viewReadNodes (%s, %s, %s, %s, %s, %s, %s, %s);" % (
            nameStr,
            mediaStr,
            spaceStr,
            firstStr,
            lastStr,
            labelStr,
            classStr,
            offsetStr,
        )

        log("    remote %s" % remote)
        self.rvc.remoteEval(remote)

    def eval(self, s):
        log("eval '%s'" % s)
        exec(s, globals(), locals())

    def zoomToNode(self):
        log("zoomToNode '%s'" % self.zoomTargetNode)

        if not self.zoomTargetNode:
            return

        node = nuke.toNode(self.zoomTargetNode)
        if not node:
            return

        x = node["xpos"].value()
        y = node["ypos"].value()
        log("    target x %g y %g" % (x, y))
        if abs(x) < 10000 and abs(y) < 10000:
            nuke.zoom(nuke.zoom(), [x, y])

        self.zoomTargetNode = None


class RvSettings:
    def __init__(self, sessionDirBase):
        """
        RV Settings object
        """
        self.settings = {}

        scriptBase = ".".join(os.path.basename(nuke.root().name()).split(".")[:-1])

        baseDir = sessionDirBase

        if not baseDir:
            baseDir = os.getenv("NUKE_TEMP_DIR")
        if not baseDir:
            baseDir = nuke.value("preferences.DiskCachePath")

        self.settings["sessionDir"] = baseDir + "/RvDir/" + scriptBase

        self.settings["outputFileFormat"] = "rgb"
        self.settings["syncSelection"] = True
        self.settings["syncFrameChange"] = True
        self.settings["syncReadChanges"] = True

        #
        #  Override defaults with stored settings
        #

        self.updateFromRoot()

    def rebuildSettingsKnobs(self, r):
        log("rebuilding settings knobs")

        #
        #  Delete all rv settings knobs
        #

        knobList = ["rvSettings_" + k for k in self.settings.keys()]
        knobList += [
            "rvSettings_syncOptionsStart",
            "rvSettings_syncOptionsEnd",
            "rvSettings_RVstart",
        ]

        for k in knobList:
            try:
                r.removeKnob(r.knob(k))
            except Exception:
                pass

        #
        #  Add rv settings knobs
        #

        r.addKnob(nuke.Tab_Knob("rvSettings_RVstart", "RV"))
        # r.addKnob(nuke.Tab_Knob ("rvSettings_RVstart", "RV", nuke.TABBEGINGROUP))
        n = "rvSettings_"

        k = nuke.File_Knob(n + "sessionDir", "Session Directory")
        k.setTooltip("Root directory of RV session data.  Should be unique to this Nuke script")
        k.setValue(self.settings["sessionDir"])
        r.addKnob(k)

        k = nuke.Enumeration_Knob(n + "outputFileFormat", "Render File Format", ["rgb", "exr", "dpx", "jpg"])
        k.setTooltip("File format for renders, usual 'rgb' or 'exr'.")
        k.setValue(self.settings["outputFileFormat"])
        r.addKnob(k)

        # r.addKnob (nuke.BeginTabGroup_Knob ("rvSettings_syncOptionsBegin", "Sync Options"))
        r.addKnob(nuke.Tab_Knob("rvSettings_syncOptionsStart", "Sync Options", nuke.TABBEGINGROUP))

        k = nuke.Boolean_Knob(n + "syncSelection", "Nuke Node Selection  ->  RV Current View")
        k.setTooltip("Sync RV current view to Nuke node selection.")
        k.setValue(self.settings["syncSelection"])
        k.setFlag(nuke.STARTLINE)
        r.addKnob(k)

        k = nuke.Boolean_Knob(n + "syncFrameChange", "Nuke Frame  ->  RV Frame")
        k.setTooltip("Sync RV frame to Nuke's frame.")
        k.setValue(self.settings["syncFrameChange"])
        k.setFlag(nuke.STARTLINE)
        r.addKnob(k)

        k = nuke.Boolean_Knob(n + "syncReadChanges", "Nuke Read/Write Node Changes  ->  RV Sources")
        k.setTooltip("Sync creation/deletion/modification of Read and Write nodes to corresponding sources in RV.")
        k.setValue(self.settings["syncReadChanges"])
        k.setFlag(nuke.STARTLINE)
        r.addKnob(k)

        r.addKnob(nuke.Tab_Knob("rvSettings_syncOptionsEnd", "Sync Options", nuke.TABENDGROUP))
        # r.addKnob (nuke.EndTabGroup_Knob ("rvSettings_syncOptionsEnd", "Sync Options"))

    def updateFromRoot(self):
        r = nuke.root()
        if not r:
            return

        somethingMissing = False
        for k in self.settings.keys():
            try:
                self.settings[k] = r.knob("rvSettings_" + k).value()
            except Exception:
                log("value failed for '%s'" % k)
                somethingMissing = True

        if somethingMissing:
            self.rebuildSettingsKnobs(r)


class RvPreferences:
    def __init__(self):
        """
        RV Preferences object
        """
        self.prefs = {}

        _plat = platform.system()
        rvPath = ""
        try:
            rvPath = os.environ["RV_PATH"]
        except Exception:
            pass

        if rvPath == "":
            try:
                rvHome = os.environ["RV_HOME"]

                if nuke.env["MACOS"]:
                    rvPath = rvHome + "/Contents/MacOS/RV64"
                elif nuke.env["LINUX"]:
                    rvPath = rvHome + "/bin/rv"
                else:
                    rvPath = rvHome + "/bin/rv.exe"

            except Exception:
                if nuke.env["MACOS"]:
                    rvPath = "/Applications/RV64.app/Contents/MacOS/RV64"
                elif nuke.env["LINUX"]:
                    rvPath = "/bin/rv"
                else:
                    rvPath = "c:/Program Files/Tweak/RV-3.10/bin/rv.exe"

        if rvPath == "":
            if nuke.env["MACOS"]:
                rvPath = "/Applications/RV64.app/Contents/MacOS/RV64"
            elif nuke.env["LINUX"]:
                rvPath = "rv"
            else:
                rvPath = "C:\\Program Files\\Tweak\\RV-3.10\\bin\\rv.exe"

        self.prefs["rvExecPath"] = rvPath
        self.prefs["extraArgs"] = ""
        self.prefs["sessionDirBase"] = ""

        self.updateFromDisk()

    def prefsPath(self):
        home = os.path.expanduser("~")
        nukeDir = home + "/.nuke"

        if not os.path.exists(nukeDir):
            os.mkdir(nukeDir)

        return nukeDir + "/rvprefs"

    def updateFromDisk(self):
        log("RvPreferences.updateFromDisk")

        prefsFileName = self.prefsPath()
        if os.path.exists(prefsFileName):
            f = open(prefsFileName)
            newDict = eval(f.read())
            f.close()

            for k in newDict:
                if k in self.prefs:
                    self.prefs[k] = newDict[k]

        log("    prefs %s" % str(self.prefs))

    def saveToDisk(self):
        log("RvPreferences.saveToDisk")

        prefsFileName = self.prefsPath()
        log("   prefs '%s'" % prefsFileName)

        f = open(prefsFileName, "w")
        f.write(str(self.prefs))
        f.write("\n")
        f.close()


class RvPreferencesPanel(nukescripts.PythonPanel):
    def __init__(self):
        """
        RV Preferences UI
        """
        self._constructing = True

        nukescripts.PythonPanel.__init__(self, "Rv Preferences", "com.tweaksoftware.RvPreferencesPanel")

        self.rvPrefs = RvPreferences()

        self.rvExecPath = nuke.File_Knob("rvExecPath", "RV Executable Path")
        self.rvExecPath.setTooltip("Path to RV executable.")
        self.rvExecPath.setValue(self.rvPrefs.prefs["rvExecPath"])
        self.addKnob(self.rvExecPath)

        self.extraArgs = nuke.String_Knob("extraArgs", "Default Command Line Args")
        self.extraArgs.setTooltip("Additional arguments to be added to the command line when RV is run.")
        self.extraArgs.setValue(self.rvPrefs.prefs["extraArgs"])
        self.addKnob(self.extraArgs)

        self.sessionDirBase = nuke.File_Knob("sessionDirBase", "Default Session Dir Base")
        self.sessionDirBase.setTooltip(
            "Path to base directory from which to form sessionDirectories. $NUKE_TEMP_DIR will be used if this is not set."
        )
        self.sessionDirBase.setValue(self.rvPrefs.prefs["sessionDirBase"])
        self.addKnob(self.sessionDirBase)

        # self.knobs()['OK'].setLabel('Save')
        log("knobs %s" % self.knobs())

        self._constructing = False
        self.show()
        # log ("after show")
        # log ("knobs %s" % self.knobs())

    def knobChanged(self, knob):
        """no worky
        if (self.knobs().has_key('OK')) :
            self.hide()
            self.knobs()['OK'].setLabel ('Save')
            self.show()
            log ("new label '%s'" % self.knobs()['OK'].label())
        """

        # log ("knobs %s" % self.knobs())
        if self._constructing:
            return
        log("prefs knobChanged %s" % knob.name())
        if knob.name() in self.rvPrefs.prefs:
            self.rvPrefs.prefs[knob.name()] = knob.value()

    def show(self):
        log("prefs show")


def showPrefs():
    """
    Show preferences panel, update stored prefs with result.
    """
    log("showPrefs")
    panel = RvPreferencesPanel()
    panel.setMinimumSize(450, 100)
    if not panel.showModalDialog():
        return

    panel.rvPrefs.saveToDisk()

    # XXX panel.rvPrefs.saveToRoot()
    global rvmon
    if rvmon:
        rvmon.updateFromPrefs()


def showSettingsPanel():
    """
    Show Project Settings, rebuilding RV settings knobs if necessary
    """
    log("showSettings")
    try:
        _s = str(nuke.root())
    except Exception:
        return

    if not nuke.root():
        return

    rvPrefs = RvPreferences()

    _settings = RvSettings(rvPrefs.prefs["sessionDirBase"])

    nuke.showSettings()


def startRv():
    rvmon = initRvMon()
    rvmon.initializeRvSide()


def viewReadsInRv(all):
    if all:
        nodes = nuke.allNodes()
    else:
        nodes = nuke.selectedNodes()

    readNodes = [
        n.name() for n in nodes if (type(n).__name__ == "Node" and (n.Class() == "Read" or n.Class() == "Write"))
    ]

    log("    reads for viewing: %s\n" % str(readNodes))
    if not readNodes:
        nuke.message("Please select one or more Read or Write nodes to view in RV.")
        return

    rvmon = initRvMon()
    rvmon.initializeRvSide()
    rvmon.queueCommand("self.viewReadsInRv(%s)" % str(readNodes))


def createCheckpoint():
    nodes = nuke.selectedNodes()

    if len(nodes) != 1:
        nuke.message("Please select a node to checkpoint.")
        return

    node = nodes[0]

    if type(node).__name__ != "Node" and type(node).__name__ != "Viewer":
        nuke.message("Node '%s' is not renderable, pleases select a node that can be rendered." % node.name())
        return

    if node.Class() == "Read" or node.Class() == "Write":
        nuke.message("There's no need to checkpoint Read or Write nodes, since they can be viewed directly in RV.")
        return

    log("createCheckpoint %s" % node.name())
    rvmon = initRvMon()
    rvmon.initializeRvSide()

    if node.Class() == "Viewer":
        node = node.input(int(nuke.knob(node.name() + ".input_number")))

    dateStr = rvmon.prepForRender(node, "checkpoint")

    f = nuke.frame()

    if nuke.views() == ["left", "right"]:
        stereo = "stereo"
    else:
        stereo = "mono"

    rvmon.queueCommand(
        "self.initRenderRun('%s', %d, %d, %d, \"%s\", %g, %d, '%s', '%s', '%s', '%s')"
        % (
            node.name(),
            f,
            f,
            1,
            "",
            0.0,
            f,
            encodeNL(node["label"].value()),
            dateStr,
            "checkpoint",
            stereo,
        )
    )

    """
    viewedInput = None
    viewer = None
    log ("activeViewer %s input %s" % (nuke.activeViewer(), nuke.activeViewer().activeInput()))
    if (nuke.activeViewer() and nuke.activeViewer().activeInput() is not None) :
        log("viewer and activeInput")
        n = nuke.activeViewer().node().input(nuke.activeViewer().activeInput())
        log ("n %s node %s" % (n.name(), node.name()))
        if (node == n) :
            log("caching viewed node %s" % node.name())
            viewedInput = nuke.activeViewer().activeInput()
            viewer = nuke.activeViewer().node()
            viewer.setInput(viewedInput, None)
            time.sleep(1)

    if (viewedInput is not None) :
        log("restoring viewer")
        viewer.setInput(viewedInput, node)
    """


def heartbeatFromRv():
    # log ("heartbeat from RV");
    pass


class RvRenderPanel(nukescripts.PythonPanel):
    def __init__(self):
        """RV Render UI"""
        log("RvRenderPanel init")
        nukescripts.PythonPanel.__init__(self, "RvRenderPanel", "com.tweaksoftware.RenderPanel")

        self.outputNode = nuke.String_Knob("outputNode", "Output Node")
        self.outputNode.setValue("")
        self.addKnob(self.outputNode)

        self.useSelected = nuke.Boolean_Knob("useSelected", "Use Selected")
        self.useSelected.setValue(True)
        self.addKnob(self.useSelected)

        minFrame = nuke.root().knob("first_frame").value()
        maxFrame = nuke.root().knob("last_frame").value()
        log("    minf %d maxf %d" % (minFrame, maxFrame))

        n = nuke.toNode(self.outputNode.value())
        log("    n %s" % n)
        if n:
            minFrame = n.firstFrame()
            maxFrame = n.lastFrame()

        log("    minf %d maxf %d" % (minFrame, maxFrame))
        # self.startFrame = nuke.Int_Knob ('firstFrame', '<img src=":qrc/images/FrameRangeLock.png">', self.__getRange() )
        self.firstFrame = nuke.Int_Knob("firstFrame", "First Frame")
        self.firstFrame.setTooltip("First frame of range to render")
        self.firstFrame.setValue(int(minFrame))
        self.addKnob(self.firstFrame)

        self.lastFrame = nuke.Int_Knob("lastFrame", "Last Frame")
        self.lastFrame.setTooltip("Last frame of range to render")
        self.lastFrame.setValue(int(maxFrame))
        self.lastFrame.clearFlag(nuke.STARTLINE)
        self.addKnob(self.lastFrame)

        #    Don't understand this, but nuke on windows complains if this knob is not present
        #
        self.frameRange = nuke.String_Knob("range", "range")
        self.frameRange.setVisible(False)
        self.addKnob(self.frameRange)

        self.updateFromRoot()
        self.updateFromSelection()

    def updateFromSelection(self):
        log("updateFromSelection")
        if self.useSelected.value():
            log("    useSelected True")
            nodes = nuke.selectedNodes()
            log("    %d selected nodes" % len(nodes))
            if nodes:
                log("    first selected node '%s'" % nodes[0].name())
                self.outputNode.setValue(nodes[0].name())
            else:
                self.outputNode.setValue("")

    def updateFromRoot(self):
        r = nuke.root()
        kd = r.knobs()
        if "RvRenderPrefs" not in kd:
            return
        s = kd["RvRenderPrefs"].value()
        # log ("reading knobs from Root '%s'" % s)
        try:
            self.readKnobs(s)
        except Exception:
            pass

    def saveToRoot(self):
        r = nuke.root()
        kd = r.knobs()
        k = None
        if "RvRenderPrefs" not in kd:
            k = nuke.Text_Knob("RvRenderPrefs", "Rv Render Panel Preferences")
            r.addKnob(k)
        else:
            k = kd["RvRenderPrefs"]

        s = self.writeKnobs(nuke.TO_SCRIPT | nuke.TO_VALUE | nuke.WRITE_ALL | nuke.WRITE_NON_DEFAULT_ONLY)
        # log ("saving knobs to Root '%s'" % s)
        k.setValue(s)
        k.setVisible(False)

    def seqInfoKnobs(self):
        pass

    def __getRange(self):
        """get the maximum frame range from all ranges requested"""
        allRanges = nuke.FrameRanges()
        n = nuke.toNode(self.outputNode.value())
        if n:
            allRanges.add(n.frameRange())

        return "%s-%sx1" % (allRanges.minFrame(), allRanges.maxFrame())

    def knobChanged(self, knob):
        log("knobChanged %s" % knob.name())
        if knob.name() == "useSelected":
            self.updateFromSelection()
            self.saveToRoot()
        """
        if knob.name() == 'cropMode':
            self.crop.setVisible (knob.value() == 'custom')
            self.cropChan.setVisible (knob.value() == 'auto')
        """


class Render:
    def __init__(self):
        """ """
        rvmon = initRvMon()

        panel = RvRenderPanel()
        panel.setMinimumSize(450, 100)
        if not panel.showModalDialog():
            return

        panel.saveToRoot()

        node = nuke.toNode(panel.outputNode.value())

        if not node:
            nuke.message("No such node as '%s'." % panel.outputNode.value())
            return

        if node.Class() == "Viewer":
            node = node.input(int(nuke.knob(node.name() + ".input_number")))

        """
        self.views = panel.views.value().split()
        self.output = panel.output.value()
        self.proxy = panel.proxy.value()
        """

        dateStr = rvmon.prepForRender(node, "current")

        start = panel.firstFrame.value()
        end = panel.lastFrame.value()
        log("    start %d end %d" % (start, end))
        """
	log ("    has_key %s" % str(node.knobs().has_key("use_limit")))
        log ("    firstFrame %d lastFrame %d" % (node.firstFrame(), node.lastFrame()))
	if ((not node.knobs().has_key("use_limit")) or node["use_limit"].value()) :
	    start = max (start, node.firstFrame())
	    end   = min (end,   node.lastFrame())
        log ("    start %d end %d" % (start, end))
	"""

        incr = 1

        audioFile = ""
        audioOffset = 0.0
        checkpointFrame = int((start + end) / 2)
        rvmon.initializeRvSide()

        if nuke.views() == ["left", "right"]:
            stereo = "stereo"
        else:
            stereo = "mono"

        rvmon.queueCommand(
            "self.initRenderRun('%s', %d, %d, %d, \"%s\", %g, %d, '%s', '%s', '%s', '%s')"
            % (
                node.name(),
                start,
                end,
                incr,
                audioFile,
                audioOffset,
                checkpointFrame,
                encodeNL(node["label"].value()),
                dateStr,
                "current",
                stereo,
            )
        )


def createReadNode(infos):
    log("createReadNode '%s'" % infos)
    global zoomTargetNode, zoomTimer

    if infos:
        nuke.Undo().begin("Create Read Nodes From RV")
        try:
            nukescripts.misc.clear_selection_recursive()

            if rvmon:
                rvmon.restoreBegin()

            last = None
            for i in infos:
                last = nuke.nodes.Read(file=i[0], first=i[1], last=i[2], name=i[3])

            last["selected"].setValue(True)
            # zoomTargetNode = last
            # nuke.show(zoomTargetNode)
            # zoomTimer = threading.Timer (0.1, zoomToNode)
            # zoomTimer.start()

            if rvmon:
                rvmon.zoomTargetNode = last.name()
                rvmon.restoreComplete()

        except Exception as msg:
            nuke.message("Read node creation failed:\n%s" % (msg,))
        nuke.Undo().end()

    # nuke.zoom (0, n["xpos"].value(), n["ypos"].value())


def restoreCheckpoint(nukeScript, nodeName, date):
    log("restoreCheckpoint %s %s %s" % (nukeScript, nodeName, date))

    #  We ask on rv side now, since otherwise dialog can come up behind rv.
    #
    # ans = nuke.ask ("Restore checkpoint: %s, %s ?" % (nodeName, date))
    # log ("    ans %s" % ans)

    log("    reading checkpoint script")
    nuke.Undo().begin("Restore %s, %s" % (nodeName, date))
    try:
        nukescripts.misc.clear_selection_recursive()

        try:
            v = nuke.activeViewer().node().name()
        except Exception:
            v = None

        if rvmon:
            rvmon.restoreBegin()

        nodes = nuke.root().nodes()
        for n in nodes:
            # log ("    deleting %s %s %s" % (n.name(), type(n).__name__, n.Class()))
            nuke.delete(n)
        nuke.scriptReadFile(nukeScript)

        if v:
            n = nuke.toNode(v)
            if n:
                nuke.show(n)

        if rvmon:
            rvmon.restoreComplete()

        log("    checkpoint restore complete")

    except Exception as msg:
        nuke.message("Checkpoint restore failed:\n%s" % (msg,))

    nuke.Undo().end()
    log("    done")


def killRV():
    if rvmon:
        # if (rvmon.rvc.connected) :
        #     rvmon.rvc.disconnect()
        rvmon.running = False


def testMenu():
    log("node %s" % nuke.thisNode().name())
    log("knob %s" % nuke.thisKnob().name())
