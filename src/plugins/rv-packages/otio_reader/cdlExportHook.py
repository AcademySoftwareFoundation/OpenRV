#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
import opentimelineio as otio
from rv import commands, extra_commands


def hook_function(in_timeline, argument_map):
    rv_node_name = argument_map.get("rv_node_name")
    if not rv_node_name:
        return

    node_base = f"{rv_node_name}.CDL"

    return otio.schemadef.CDL.CDL(
        name=extra_commands.uiName(rv_node_name),
        slope=commands.getFloatProperty(f"{node_base}.slope"),
        power=commands.getFloatProperty(f"{node_base}.power"),
        offset=commands.getFloatProperty(f"{node_base}.offset"),
        saturation=commands.getFloatProperty(f"{node_base}.saturation")[0],
        visible=commands.getIntProperty(f"{node_base}.active")[0] != 0,
    )
