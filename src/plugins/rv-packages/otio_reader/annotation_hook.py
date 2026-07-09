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

            if commands.propertyExists(f"{transform_node}.otio.global_translate") and commands.propertyExists(
                f"{transform_node}.otio.global_scale"
            ):
                global_translate = commands.getFloatProperty(f"{transform_node}.otio.global_translate")
                global_scale = commands.getFloatProperty(f"{transform_node}.otio.global_scale")
                global_translate_vec = otio.schema.V2d(global_translate[0], global_translate[1])
                global_scale_vec = otio.schema.V2d(global_scale[0], global_scale[1])
            else:
                logging.debug(
                    "OTIO global scale and translate properties not found, using aspect ratio from the source"
                )
                try:
                    media_info = commands.sourceMediaInfo(first_source_node)
                    height = media_info["height"]
                    aspect_ratio = media_info["width"] / height if height != 0 else 1920 / 1080
                except Exception:
                    logging.warning("Unable to determine aspect ratio, using default value of 16:9")
                    aspect_ratio = 1920 / 1080

                scale = aspect_ratio / 16
                global_scale_vec = otio.schema.V2d(scale, scale)

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


def _transform_pos(x, y):
    """Transform a 2D OTIO position to RV WCS. Returns (wcs_x, wcs_y) or None."""
    result = _transform_otio_to_world_coordinate(otio.schemadef.Point.Point(x=x, y=y, width=0.0))
    if result is None:
        return None
    return result[0], result[1]


def _transform_scalar(w):
    """Scale a scalar OTIO distance to RV WCS via the x-axis scale, or return the raw value."""
    result = _transform_otio_to_world_coordinate(otio.schemadef.Point.Point(x=0.0, y=0.0, width=float(w)))
    return result[2] if result is not None else float(w)


def _add_shape_to_frame_order(paint_node, stroke_id, frame, prefix):
    """Insert a shape into the frame draw-order list."""
    frame_component = f"{paint_node}.frame:{frame}"
    if not commands.propertyExists(f"{frame_component}.order"):
        commands.newProperty(f"{frame_component}.order", commands.StringType, 1)
    commands.insertStringProperty(f"{frame_component}.order", [f"{prefix}:{stroke_id}:{frame}:annotation"])


def _create_bbox_shape(layer, paint_node, stroke_id, frame, shape_type):
    """Create an RVPaint rect: or ellipse: node from a shape layer."""
    prefix = "rect" if shape_type.startswith("Rectangle.") else "ellipse"
    shape_component = f"{paint_node}.{prefix}:{stroke_id}:{frame}:annotation"

    min_pos = _transform_pos(layer.min.x, layer.min.y)
    max_pos = _transform_pos(layer.max.x, layer.max.y)
    if min_pos is None or max_pos is None:
        logging.warning(f"annotation_hook: could not transform bbox coords for {shape_component} — skipping")
        return

    border_width = float(layer.border_width) if layer.border_width is not None else 0.0
    wcs_border_width = _transform_scalar(border_width)

    corner_radius = getattr(layer, "corner_radius", None)
    wcs_corner_radius = float(corner_radius) if corner_radius is not None else 0.0

    effectHook.add_rv_effect_props(
        shape_component,
        {
            "min": list(min_pos),
            "max": list(max_pos),
            "innerColor": [float(c) for c in layer.inner_color],
            "borderColor": [float(c) for c in layer.border_color],
            "borderWidth": [wcs_border_width],
            "cornerRadius": [wcs_corner_radius],
            "startFrame": frame,
            "duration": 1,
            "eye": 2,
            "softDeleted": layer.visible is False,
            "uuid": [layer.id],
        },
    )

    if layer.visible is not False:
        _add_shape_to_frame_order(paint_node, stroke_id, frame, prefix)


def _create_point_pair_shape(layer, paint_node, stroke_id, frame, shape_type):
    """Create an RVPaint arrow: or line: node from a shape layer."""
    prefix = "arrow" if shape_type.startswith("Arrow.") else "line"
    shape_component = f"{paint_node}.{prefix}:{stroke_id}:{frame}:annotation"

    start_pos = _transform_pos(layer.start_position.x, layer.start_position.y)
    end_pos = _transform_pos(layer.end_position.x, layer.end_position.y)
    if start_pos is None or end_pos is None:
        logging.warning(f"annotation_hook: could not transform point-pair coords for {shape_component} — skipping")
        return

    raw_bw = float(layer.width if shape_type.startswith("Line.") else layer.border_width)
    wcs_border_width = _transform_scalar(raw_bw)

    props = {
        "startPos": list(start_pos),
        "endPos": list(end_pos),
        "borderColor": [float(c) for c in layer.border_color],
        "borderWidth": [wcs_border_width],
        "startFrame": frame,
        "duration": 1,
        "eye": 2,
        "softDeleted": layer.visible is False,
        "uuid": [layer.id],
    }

    if shape_type.startswith("Arrow."):
        wcs_thickness = _transform_scalar(float(layer.width))
        props["innerColor"] = [float(c) for c in layer.inner_color]
        props["thickness"] = [wcs_thickness]

    effectHook.add_rv_effect_props(shape_component, props)

    if layer.visible is not False:
        _add_shape_to_frame_order(paint_node, stroke_id, frame, prefix)


def _create_text_shape(layer, paint_node, stroke_id, frame, source_node=None):
    """Create an RVPaint text: node from a shape layer."""
    shape_component = f"{paint_node}.text:{stroke_id}:{frame}:annotation"

    anchor = layer.anchor
    anchor_pos = _transform_pos(anchor.x, anchor.y)
    if anchor_pos is None:
        logging.warning(f"annotation_hook: could not transform text anchor for {shape_component} — skipping")
        return

    # font_size in OTIO is in OTIO-space units (same coordinate space as
    # border_width). RVPaint's fontSize is a WCS fraction (image-height-
    # normalised) — the renderer multiplies by framebuffer height at draw
    # time (see PaintCommand.cpp) — so no pixel conversion happens here.
    font_size_wcs = _transform_scalar(float(layer.font_size))

    effectHook.add_rv_effect_props(
        shape_component,
        {
            "text": [layer.text],
            "fontFamily": [layer.font_family],
            "fontWeight": [layer.font_weight],
            "fontStyle": [layer.font_style],
            "textDecoration": [layer.text_decoration],
            "textAlign": [layer.text_align],
            "position": list(anchor_pos),
            "fontSize": [font_size_wcs],
            "color": [float(c) for c in layer.inner_color],
            "startFrame": frame,
            "duration": 1,
            "softDeleted": layer.visible is False,
            "uuid": [layer.id],
        },
    )

    if layer.visible is not False:
        _add_shape_to_frame_order(paint_node, stroke_id, frame, "text")


def hook_function(in_timeline: otio.schemadef.Annotation.Annotation, argument_map: dict | None = None) -> None:
    """A hook for the annotation schema"""
    try:
        commands.setIntProperty("#Session.paintEffects.hold", [in_timeline.hold])
        commands.setIntProperty("#Session.paintEffects.ghost", [in_timeline.ghost])

        commands.setIntProperty("#Session.paintEffects.ghostBefore", [in_timeline.ghost_before])
        commands.setIntProperty("#Session.paintEffects.ghostAfter", [in_timeline.ghost_after])
    except Exception:
        logging.warning("Unable to set Hold and Ghost properties")

    for layer in in_timeline.layers:
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
        frame_component = f"{paint_node}.frame:{frame}"

        # Set properties on the paint component of the RVPaint node
        effectHook.set_rv_effect_props(paint_component, {"nextId": stroke_id})

        shape_label = getattr(layer, "_serializable_label", "")

        if isinstance(layer, otio.schemadef.Paint.Paint):
            pen_component = f"{paint_node}.pen:{stroke_id}:{frame}:annotation"

            start_time = int(time_range.start_time.value)
            end_time = int(start_time + time_range.duration.value)
            duration = end_time - start_time

            # Add and set properties on the pen component of the RVPaint node
            effectHook.add_rv_effect_props(
                pen_component,
                {
                    "color": list(map(float, layer.rgba)),
                    "brush": [layer.brush],
                    "debug": 1,
                    "join": 3,
                    "cap": 1,
                    "splat": 0,
                    "mode": 0 if layer.type == "COLOR" else 1,
                    "startFrame": start_time,
                    "duration": duration,
                    "softDeleted": layer.soft_deleted,
                    "uuid": [layer.id],
                },
            )

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

            if not layer.soft_deleted:
                if not commands.propertyExists(f"{frame_component}.order"):
                    commands.newProperty(f"{frame_component}.order", commands.StringType, 1)

                commands.insertStringProperty(f"{frame_component}.order", [f"pen:{stroke_id}:{frame}:annotation"])

        elif shape_label.startswith(("Rectangle.", "Ellipse.")):
            _create_bbox_shape(layer, paint_node, stroke_id, frame, shape_label)

        elif shape_label.startswith(("Arrow.", "Line.")):
            _create_point_pair_shape(layer, paint_node, stroke_id, frame, shape_label)

        elif shape_label.startswith("Text."):
            _create_text_shape(layer, paint_node, stroke_id, frame, source_node)

        else:
            logging.warning(f"annotation_hook: unrecognised layer type {shape_label!r} — skipping")
