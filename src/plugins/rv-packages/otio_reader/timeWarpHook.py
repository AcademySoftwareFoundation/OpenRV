#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import sys

from rv import commands
from effectHook import *


def hook_function(in_timeline, argument_map=None):
    rv_retime = commands.newNode("RVRetime")
    otio_retime = in_timeline
    scalar = otio_retime.time_scalar
    if scalar <= 0.0:
        scalar = sys.float_info.epsilon

    kind = "audio" if argument_map.get("kind", "") == "Audio" else "visual"

    node_component = "{}.{}".format(rv_retime, kind)
    set_rv_effect_props(node_component, {"scale": 1 / scalar})
    return rv_retime
