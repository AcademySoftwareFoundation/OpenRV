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
    linearize = extra_commands.associatedNode("RVLinearize", source)

    if commands.getIntProperty("{}.CDL.active".format(linearize))[0] != 0:
        cdl = otio.schemadef.CDL.CDL(
            name=extra_commands.uiName(linearize),
            slope=commands.getFloatProperty("{}.CDL.slope".format(linearize)),
            power=commands.getFloatProperty("{}.CDL.power".format(linearize)),
            offset=commands.getFloatProperty("{}.CDL.offset".format(linearize)),
            saturation=commands.getFloatProperty("{}.CDL.saturation".format(linearize))[0],
            visible=True,
        )

        cdl.metadata.update(get_otio_metadata("{}.CDL".format(linearize)))
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
