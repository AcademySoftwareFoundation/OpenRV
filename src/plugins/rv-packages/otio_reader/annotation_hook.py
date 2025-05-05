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


def calculate_next_available_start_time(in_timeline, frame):
    annotations = []

    for layer in in_timeline.layers:
        if layer.name == "Paint":
            time_range = layer.layer_range
            start_time = int(time_range.start_time.value)
            annotations.append(start_time)

    annotations.sort()

    for start_time in annotations:
        if start_time > frame:
            return start_time

    return None


def hook_function(
    in_timeline: otio.schemadef.Annotation.Annotation, argument_map: dict | None = None
) -> None:
    """A hook for the annotation schema"""
    for layer in in_timeline.layers:
        if layer.name == "Paint":
            if isinstance(layer.layer_range, otio.opentime.TimeRange):
                time_range = layer.layer_range
            else:
                time_range = otio.opentime.TimeRange(
                    layer.layer_range["start_time"], layer.layer_range["duration"]
                )

            relative_time = time_range.end_time_inclusive()
            frame = relative_time.to_frames()

            source_node = argument_map.get("source_group")
            paint_node = extra_commands.nodesInGroupOfType(source_node, "RVPaint")[0]
            paint_component = f"{paint_node}.paint"
            stroke_id = commands.getIntProperty(f"{paint_component}.nextId")[0]
            pen_component = f"{paint_node}.pen:{stroke_id}:{frame}:annotation"
            frame_component = f"{paint_node}.frame:{frame}"

            # Set properties on the paint component of the RVPaint node
            effectHook.set_rv_effect_props(paint_component, {"nextId": stroke_id + 1})

            is_hold = argument_map["effect_metadata"]["hold"]
            is_ghost = argument_map["effect_metadata"]["ghost"]

            start_time = int(time_range.start_time.value)
            end_time = int(start_time + time_range.duration.value)

            if is_hold:
                next_available_start_time = calculate_next_available_start_time(
                    in_timeline, frame
                )
                if next_available_start_time is not None:
                    duration = next_available_start_time - start_time
                else:
                    source = argument_map.get("src")
                    media_info = commands.sourceMediaInfo(source)
                    end_frame = int(media_info["endFrame"])
                    duration = end_frame - start_time
            else:
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
                    "hold": is_hold,
                    "ghost": is_ghost,
                    "ghostBefore": 3,
                    "ghostAfter": 3,
                },
            )

            if not commands.propertyExists(f"{frame_component}.order"):
                commands.newProperty(f"{frame_component}.order", commands.StringType, 1)

            commands.insertStringProperty(
                f"{frame_component}.order", [f"pen:{stroke_id}:{frame}:annotation"]
            )

            global_scale = argument_map.get("global_scale")
            if global_scale is None:
                logging.warning(
                    "Unable to get the global scale, using the aspect ratio of the first media file"
                )
                try:
                    first_source_node = commands.sourcesAtFrame(0)[0]
                    media_info = commands.sourceMediaInfo(first_source_node)
                    height = media_info["height"]
                    aspect_ratio = media_info["width"] / height
                except Exception:
                    logging.exception(
                        "Unable to determine aspect ratio, using default value of 16:9"
                    )
                    aspect_ratio = 1920 / 1080
                finally:
                    scale = aspect_ratio / 16
                    global_scale = otio.schema.V2d(scale, scale)

            points_property = f"{pen_component}.points"
            width_property = f"{pen_component}.width"

            if not commands.propertyExists(points_property):
                commands.newProperty(points_property, commands.FloatType, 2)
            if not commands.propertyExists(width_property):
                commands.newProperty(width_property, commands.FloatType, 1)

            global_width = 2 / 15  # 0.133333...

            for point in layer.points:
                commands.insertFloatProperty(
                    points_property,
                    [point.x * global_scale.x, point.y * global_scale.y],
                )
                commands.insertFloatProperty(
                    width_property, [point.width * global_width]
                )
