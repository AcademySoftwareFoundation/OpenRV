#! /usr/bin/python
#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

import rvNetwork
import sys
import time
import select
import tty


class PlayControl:
    def __init__(self, host, port=45124):
        self.frameChanged = False
        self.myFrame = None
        self.rvc = rvNetwork.RvCommunicator("myPlayControl")
        self.rvc.connect("127.0.0.1", port)
        if not self.rvc.connected:
            sys.exit(-1)
        self.rvc.bindToEvent("frame-changed", self.frameChangeHandler)
        tty.setraw(sys.stdin.fileno())

    def frameChangeHandler(self, contents):
        self.frameChanged = True

    def run(self):

        sys.stdout.write("Type: p=play, s=stop, r=reverse play, q=quit\r\n")

        while 1:
            time.sleep(0.01)

            self.rvc.processEvents()

            if not self.rvc.connected:
                sys.exit(-1)

            rfds, wfds, efds = select.select([sys.stdin], [], [], 0)
            if len(rfds) > 0:
                c = sys.stdin.read(1)

                if c == "p":
                    self.rvc.remoteEval("setInc(1); play();")
                elif c == "s":
                    self.rvc.remoteEval("stop();")
                elif c == "r":
                    self.rvc.remoteEval("setInc(-1); play();")
                elif c == "q":
                    self.rvc.disconnect()
                    sys.exit(0)

            self.rvc.processEvents()

            if self.frameChanged:
                self.frameChanged = False
                frame = self.rvc.remoteEvalAndReturn("frame();")
                if frame != self.myFrame:
                    myFrame = frame
                    sys.stdout.write("frame: %s\r\n" % myFrame)


pc = PlayControl("127.0.0.1")
pc.run()
