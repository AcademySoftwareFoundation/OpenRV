#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import commands, extra_commands
from effectHook import get_otio_metadata
import opentimelineio as otio


def hook_function(in_timeline, argument_map):
    source_group = argument_map.get("rv_node_name")
    clip = argument_map.get("post_node")

    if not source_group or not clip or not isinstance(clip, otio.schema.Clip):
        return

    source = extra_commands.nodesInGroupOfType(source_group, "RVSource")[0]
    for nodeType in ("RVLinearize", "RVColor"):
        node = extra_commands.associatedNode(nodeType, source)
        if not commands.propertyExists(f"{node}.CDL.active"):
            continue

        active = commands.getIntProperty(f"{node}.CDL.active")
        if not active or active[0] == 0:
            continue

        cdl = otio.schemadef.CDL.CDL(
            name=extra_commands.uiName(node),
            slope=commands.getFloatProperty(f"{node}.CDL.slope"),
            power=commands.getFloatProperty(f"{node}.CDL.power"),
            offset=commands.getFloatProperty(f"{node}.CDL.offset"),
            saturation=commands.getFloatProperty(f"{node}.CDL.saturation")[0],
            visible=True,
        )

        cdl.metadata.update(get_otio_metadata(f"{node}.CDL"))
        clip.effects.append(cdl)

    # for releases >= 0.15
    if hasattr(clip.media_reference, "available_image_bounds"):
        transform = extra_commands.associatedNode("RVTransform2D", source)
        if commands.getIntProperty("{}.transform.active".format(transform))[0] != 0:
            global_translate_vec = otio.schema.V2d(0.0, 0.0)
            global_scale_vec = otio.schema.V2d(1.0, 1.0)

            if commands.propertyExists("{}.otio.global_translate".format(transform)) and commands.propertyExists(
                "{}.otio.global_scale".format(transform)
            ):
                global_translate = commands.getFloatProperty("{}.otio.global_translate".format(transform))
                global_scale = commands.getFloatProperty("{}.otio.global_scale".format(transform))
                global_translate_vec = otio.schema.V2d(global_translate[0], global_translate[1])
                global_scale_vec = otio.schema.V2d(global_scale[0], global_scale[1])

            translate = commands.getFloatProperty(
                "{}.transform.translate".format(transform),
            )
            scale = commands.getFloatProperty(
                "{}.transform.scale".format(transform),
            )

            translate_vec = otio.schema.V2d(translate[0], translate[1])
            scale_vec = otio.schema.V2d(scale[0], scale[1])

            media_info = commands.sourceMediaInfo(source)
            height = media_info["height"]
            aspect_ratio = 1.0 if height == 0 else media_info["width"] / height

            bounds_size = scale_vec / global_scale_vec
            bounds_center = (translate_vec + global_translate_vec) / global_scale_vec

            clip.media_reference.available_image_bounds = otio.schema.Box2d(
                otio.schema.V2d(
                    bounds_center.x - bounds_size.x * aspect_ratio / 2.0,
                    bounds_center.y - bounds_size.y / 2.0,
                ),
                otio.schema.V2d(
                    bounds_center.x + bounds_size.x * aspect_ratio / 2.0,
                    bounds_center.y + bounds_size.y / 2.0,
                ),
            )
