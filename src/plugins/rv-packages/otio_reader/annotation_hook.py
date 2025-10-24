# *****************************************************************************
# Copyright 2024 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************


import logging
import effectHook
import opentimelineio as otio
from rv import commands, extra_commands


def _get_transform_properties():
    """Retrieve RV transform properties for coordinate transformations."""
    try:
        media_switch = commands.nodeConnections("MediaTrack")[0][0]
        media_source_group = commands.nodeConnections(media_switch)[0][0]
        first_source_node = extra_commands.nodesInGroupOfType(media_source_group, "RVFileSource")[0]

    except Exception as e:
        logging.warning(f"Unable to get source node: {e}")
        return None

    if first_source_node:
        transform_node = extra_commands.associatedNode("RVTransform2D", first_source_node)
        if commands.getIntProperty(f"{transform_node}.transform.active")[0] != 0:
            global_translate_vec = otio.schema.V2d(0.0, 0.0)
            global_scale_vec = otio.schema.V2d(1.0, 1.0)

            global_translate = commands.getFloatProperty(f"{transform_node}.otio.global_translate")
            global_scale = commands.getFloatProperty(f"{transform_node}.otio.global_scale")
            global_translate_vec = otio.schema.V2d(global_translate[0], global_translate[1])
            global_scale_vec = otio.schema.V2d(global_scale[0], global_scale[1])

            translate = commands.getFloatProperty(f"{transform_node}.transform.translate")
            scale = commands.getFloatProperty(f"{transform_node}.transform.scale")

            translate_vec = otio.schema.V2d(translate[0], translate[1])
            scale_vec = otio.schema.V2d(scale[0], scale[1])

            bounds_size = scale_vec / global_scale_vec
            bounds_center = (translate_vec + global_translate_vec) / global_scale_vec

            return (bounds_size, bounds_center)
        else:
            return None
    else:
        return None


def _transform_otio_to_world_coordinate(point):
    """Transform coordinates from OTIO space to RV world coordinate space (WCS)."""
    transform_properties = _get_transform_properties()
    if transform_properties is None:
        return None

    bounds_size, bounds_center = transform_properties

    world_coordinate_x = (point.x - bounds_center.x) / bounds_size.x
    world_coordinate_y = (point.y - bounds_center.y) / bounds_size.y
    world_coordinate_width = point.width / bounds_size.x

    return (world_coordinate_x, world_coordinate_y, world_coordinate_width)


def hook_function(in_timeline: otio.schemadef.Annotation.Annotation, argument_map: dict | None = None) -> None:
    """A hook for the annotation schema"""

    try:
        if argument_map["effect_metadata"]:
            effect_metadata = argument_map["effect_metadata"]
            commands.setIntProperty("#Session.paintEffects.hold", [effect_metadata.get("hold", 0)])
            commands.setIntProperty("#Session.paintEffects.ghost", [effect_metadata.get("ghost", 0)])
        else:
            commands.setIntProperty("#Session.paintEffects.hold", [in_timeline.hold])
            commands.setIntProperty("#Session.paintEffects.ghost", [in_timeline.ghost])

        commands.setIntProperty("#Session.paintEffects.ghostBefore", [in_timeline.ghost_before])
        commands.setIntProperty("#Session.paintEffects.ghostAfter", [in_timeline.ghost_after])
    except Exception:
        logging.exception("Unable to set Hold and Ghost properties")

    for layer in in_timeline.layers:
        if layer.name == "Paint":
            if isinstance(layer.layer_range, otio.opentime.TimeRange):
                time_range = layer.layer_range
            else:
                time_range = otio.opentime.TimeRange(layer.layer_range["start_time"], layer.layer_range["duration"])

            relative_time = time_range.end_time_inclusive()
            frame = relative_time.to_frames()

            source_node = argument_map.get("source_group")
            paint_node = extra_commands.nodesInGroupOfType(source_node, "RVPaint")[0]
            paint_component = f"{paint_node}.paint"
            stroke_id = commands.getIntProperty(f"{paint_component}.nextId")[0] + 1
            pen_component = f"{paint_node}.pen:{stroke_id}:{frame}:annotation"
            frame_component = f"{paint_node}.frame:{frame}"

            # Set properties on the paint component of the RVPaint node
            effectHook.set_rv_effect_props(paint_component, {"nextId": stroke_id})

            start_time = int(time_range.start_time.value)
            end_time = int(start_time + time_range.duration.value)

            duration = end_time - start_time

            # Add and set properties on the pen component of the RVPaint node
            effectHook.add_rv_effect_props(
                pen_component,
                {
                    "color": list(map(float, layer.rgba)),
                    "brush": layer.brush,
                    "debug": 1,
                    "join": 3,
                    "cap": 1,
                    "splat": 0,
                    "mode": 0 if layer.type == "COLOR" else 1,
                    "startFrame": start_time,
                    "duration": duration,
                },
            )

            if not commands.propertyExists(f"{frame_component}.order"):
                commands.newProperty(f"{frame_component}.order", commands.StringType, 1)

            commands.insertStringProperty(f"{frame_component}.order", [f"pen:{stroke_id}:{frame}:annotation"])

            points_property = f"{pen_component}.points"
            width_property = f"{pen_component}.width"
            if not commands.propertyExists(points_property):
                commands.newProperty(points_property, commands.FloatType, 2)
            if not commands.propertyExists(width_property):
                commands.newProperty(width_property, commands.FloatType, 1)
            for point in layer.points:
                world_coordinate_x, world_coordinate_y, world_coordinate_width = _transform_otio_to_world_coordinate(
                    point
                )

                commands.insertFloatProperty(
                    points_property,
                    [world_coordinate_x, world_coordinate_y],
                )
                commands.insertFloatProperty(width_property, [world_coordinate_width])
