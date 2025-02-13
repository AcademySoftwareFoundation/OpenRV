#! /usr/bin/python
#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

import rvNetwork
import sys
import time


class MuConsoleException(Exception):
    pass


class MuConsole:
    def __init__(self, host="127.0.0.1", port=45124):
        self.rvc = rvNetwork.RvCommunicator("myMuConsole")
        self.rvc.connect(host, port)
        if not self.rvc.connected:
            raise MuConsoleException("Cannot connect to '%s:%d'" % (host, port))

    def run(self):
        print("Return on blank line to send to rv.")
        buf = ""
        while 1:
            time.sleep(0.1)

            l = sys.stdin.readline()
            if l[0] == "\n":
                print("RETURN:", self.rvc.remoteEvalAndReturn(buf))
                buf = ""
            else:
                buf += l

            self.rvc.processEvents()


if __name__ == "__main__":
    try:
        mu = MuConsole()
        mu.run()
    except MuConsoleException as inst:
        print((str(inst)))
        sys.exit(-1)
