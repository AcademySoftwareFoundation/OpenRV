#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import commands, extra_commands
import opentimelineio as otio


def hook_function(in_timeline, argument_map):
    switch_node = argument_map.get("rv_node_name")

    # in case there are any post source hooks, run them on the active source
    for src_group in commands.nodeConnections(switch_node)[0]:
        source = extra_commands.nodesInGroupOfType(src_group, "RVSource")[0]
        if commands.getIntProperty("{}.media.active".format(source))[0] == 1:
            argument_map["rv_node_name"] = src_group
            otio.hooks.run("post_export_hook_RVSourceGroup", in_timeline, argument_map)

            argument_map["rv_node_name"] = switch_node
