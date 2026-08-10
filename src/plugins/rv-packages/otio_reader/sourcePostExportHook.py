#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
import opentimelineio as otio
from effectHook import get_otio_metadata
from rv import commands, extra_commands


def hook_function(in_timeline, argument_map):
    source_group = argument_map.get("rv_node_name")
    clip = argument_map.get("post_node")

    if not source_group or not clip or not isinstance(clip, otio.schema.Clip):
        return

    source = extra_commands.nodesInGroupOfType(source_group, "RVSource")[0]
    linearize = extra_commands.associatedNode("RVLinearize", source)

    if commands.getIntProperty(f"{linearize}.CDL.active")[0] != 0:
        cdl = otio.schemadef.CDL.CDL(
            name=extra_commands.uiName(linearize),
            slope=commands.getFloatProperty(f"{linearize}.CDL.slope"),
            power=commands.getFloatProperty(f"{linearize}.CDL.power"),
            offset=commands.getFloatProperty(f"{linearize}.CDL.offset"),
            saturation=commands.getFloatProperty(f"{linearize}.CDL.saturation")[0],
            visible=True,
        )

        cdl.metadata.update(get_otio_metadata(f"{linearize}.CDL"))
        clip.effects.append(cdl)

    # for releases >= 0.15
    if hasattr(clip.media_reference, "available_image_bounds"):
        transform = extra_commands.associatedNode("RVTransform2D", source)
        if commands.getIntProperty(f"{transform}.transform.active")[0] != 0:
            global_translate_vec = otio.schema.V2d(0.0, 0.0)
            global_scale_vec = otio.schema.V2d(1.0, 1.0)

            if commands.propertyExists(f"{transform}.otio.global_translate") and commands.propertyExists(
                f"{transform}.otio.global_scale"
            ):
                global_translate = commands.getFloatProperty(f"{transform}.otio.global_translate")
                global_scale = commands.getFloatProperty(f"{transform}.otio.global_scale")
                global_translate_vec = otio.schema.V2d(global_translate[0], global_translate[1])
                global_scale_vec = otio.schema.V2d(global_scale[0], global_scale[1])

            translate = commands.getFloatProperty(
                f"{transform}.transform.translate",
            )
            scale = commands.getFloatProperty(
                f"{transform}.transform.scale",
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
