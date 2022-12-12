#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import commands, extra_commands
import opentimelineio as otio

import six
import math
import sys


def hook_function(in_timeline, argument_map):
    def is_close(v1, v2, rel_tol=1e-05, abs_tol=0.0):
        if six.PY2:
            return abs(v1 - v2) <= max(rel_tol * max(abs(v1), abs(v2)), abs_tol)
        return math.isclose(v1, v2, rel_tol=rel_tol, abs_tol=abs_tol)

    rv_node_name = argument_map.get("rv_node_name")
    if not rv_node_name:
        return

    scalar = 1 / commands.getFloatProperty("{}.audio.scale".format(rv_node_name))[0]

    if is_close(scalar, 1.0):
        scalar = (
            1 / commands.getFloatProperty("{}.visual.scale".format(rv_node_name))[0]
        )

    key_frames = commands.getIntProperty("{}.warp.keyFrames".format(rv_node_name))

    # only linear warps are supported
    if len(key_frames) > 1:
        return

    if is_close(scalar, 1.0):
        return
    elif is_close(scalar, sys.float_info.epsilon):
        return otio.schema.FreezeFrame(
            name=extra_commands.uiName(rv_node_name),
        )
    return otio.schema.LinearTimeWarp(
        name=extra_commands.uiName(rv_node_name), time_scalar=scalar
    )
