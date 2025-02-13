#! /usr/bin/python
#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

import rvNetwork
import sys

if len(sys.argv) != 3:
    print("usage: %s <port> <file>" % sys.argv[0])
    exit(1)

port = int(sys.argv[1])
file = sys.argv[2]

rvc = rvNetwork.RvCommunicator("AddSource")
rvc.connect("127.0.0.1", port)
if not rvc.connected:
    print("Failed to connect to RV")
    sys.exit(2)

cmd = (
    """
{
    int _ret = 0;
    try 
    {
        int before = sources().size(); 
        addSource(\"%s\"); 
        play();
        _ret = sources().size() - before;
    }
    catch (object obj)
    {
        _ret = 0;
    }
    _ret;
}
"""
    % file
)

ret = rvc.remoteEvalAndReturn(cmd)
if ret == "1":
    print("added '%s'" % file)
else:
    print("failed to add '%s'" % file)

rvc.disconnect()
