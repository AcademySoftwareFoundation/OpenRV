#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import commands, extra_commands
import opentimelineio as otio


def hook_function(in_timeline, argument_map):
    rv_node_name = argument_map.get("rv_node_name")
    if not rv_node_name:
        return

    node_base = "{}.CDL".format(rv_node_name)

    return otio.schemadef.CDL.CDL(
        name=extra_commands.uiName(rv_node_name),
        slope=commands.getFloatProperty("{}.slope".format(node_base)),
        power=commands.getFloatProperty("{}.power".format(node_base)),
        offset=commands.getFloatProperty("{}.offset".format(node_base)),
        saturation=commands.getFloatProperty("{}.saturation".format(node_base))[0],
        visible=commands.getIntProperty("{}.active".format(node_base))[0] != 0,
    )
