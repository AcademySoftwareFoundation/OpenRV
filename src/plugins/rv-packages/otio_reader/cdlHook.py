#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import extra_commands, commands
from effectHook import set_rv_effect_props, add_otio_metadata


def modify_source(source, otio_cdl):
    rv_cdl = extra_commands.associatedNode("RVLinearize", source)

    node_component = "{}.CDL".format(rv_cdl)
    set_rv_effect_props(
        node_component,
        {
            "slope": otio_cdl.slope,
            "power": otio_cdl.power,
            "offset": otio_cdl.offset,
            "active": otio_cdl.visible,
            "saturation": otio_cdl.saturation,
        },
    )
    add_otio_metadata(rv_cdl, otio_cdl)


def add_cdl_node(otio_cdl):
    rv_cdl = commands.newNode("RVLinearize", otio_cdl.name or "linearize")

    node_component = "{}.CDL".format(rv_cdl)
    set_rv_effect_props(
        node_component,
        {
            "slope": otio_cdl.slope,
            "power": otio_cdl.power,
            "offset": otio_cdl.offset,
            "active": otio_cdl.visible,
            "saturation": otio_cdl.saturation,
        },
    )
    return rv_cdl


def hook_function(in_timeline, argument_map=None):
    return add_cdl_node(in_timeline)

    # The below code modifies the CDL properties of the RVColor node in the
    # source group as an alternative to creating a new RVCDL node
    #
    # return modify_source(
    #    extra_commands.associatedNode('RVColor', argument_map['source']),
    #    in_timeline
    # )
