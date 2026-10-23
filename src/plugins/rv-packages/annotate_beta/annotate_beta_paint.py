# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

"""Read and write annotation components stored as properties on RVPaint nodes."""

import os
import uuid
from collections.abc import Iterable

from rv import commands

import annotate_beta_constants as constants
from annotate_beta_types import PaintComponent, PaintFrame

BOX_PREFIXES = frozenset({"rect", "ellipse"})


def _ensure_property(property_name, property_type, width):
    if not commands.propertyExists(property_name):
        commands.newProperty(property_name, property_type, width)


def _ensure_visible(paint_node):
    show_property = f"{paint_node}.paint.show"
    _ensure_property(show_property, commands.IntType, 1)
    commands.setIntProperty(show_property, [1], True)


def _next_id(paint_node):
    next_id_property = f"{paint_node}.paint.nextId"
    if not commands.propertyExists(next_id_property):
        commands.newProperty(next_id_property, commands.IntType, 1)
        commands.setIntProperty(next_id_property, [0])
    next_id = commands.getIntProperty(next_id_property)[0] + 1
    commands.setIntProperty(next_id_property, [next_id])
    return next_id


def _new_component(paint_frame: PaintFrame, prefix: str) -> PaintComponent:
    node_id = _next_id(paint_frame.paint_node)
    host = commands.myNetworkHost().replace(".", "_")
    process_id = os.getpid()
    return PaintComponent(paint_frame, f"{prefix}:{node_id}:{paint_frame.frame}:{host}_{process_id}")


def _node_name(component: PaintComponent) -> str:
    """Return the property path prefix of a component, e.g. "RVPaint_1.rect:3:42:host_1234"."""
    return f"{component.paint_frame.paint_node}.{component.name}"


def frame_order_property(paint_frame: PaintFrame) -> str:
    return f"{paint_frame.paint_node}.frame:{paint_frame.frame}.order"


def _add_to_order(component: PaintComponent):
    """Insert the component into the frame draw order."""
    order_property = frame_order_property(component.paint_frame)
    _ensure_property(order_property, commands.StringType, 1)
    if component.name not in commands.getStringProperty(order_property):
        commands.insertStringProperty(order_property, [component.name])


def _remove_from_order(component: PaintComponent):
    """Remove a component from the frame draw-order list."""
    order_property = frame_order_property(component.paint_frame)
    if not commands.propertyExists(order_property):
        return
    current = list(commands.getStringProperty(order_property))
    if component.name in current:
        current.remove(component.name)
        commands.setStringProperty(order_property, current, True)


def _uuid_for(component: PaintComponent) -> str:
    """Return the UUID stored on a component, or empty string if unavailable."""
    uuid_property = f"{_node_name(component)}.uuid"
    if commands.propertyExists(uuid_property):
        return commands.getStringProperty(uuid_property)[0]
    return ""


def uuids_for(components: Iterable[PaintComponent]) -> list[str]:
    """Return the UUIDs stored on the components, skipping components without one."""
    uuids = (_uuid_for(component) for component in components)
    return [component_uuid for component_uuid in uuids if component_uuid]


def soft_delete(component: PaintComponent):
    _remove_from_order(component)
    commands.setIntProperty(f"{_node_name(component)}.softDeleted", [1], True)


def restore(component: PaintComponent):
    _add_to_order(component)
    commands.setIntProperty(f"{_node_name(component)}.softDeleted", [0], True)


def annotated_frames(paint_node: str) -> set[PaintFrame]:
    """Return the paint frames that have an order property on paint_node.

    Scans actual properties rather than iterating a frame range so that
    annotations stored at source-space frame numbers outside the current
    timeline range are still found.
    """
    frames = (order_property_frame(property_name) for property_name in commands.properties(paint_node))
    return {frame for frame in frames if frame is not None}


def all_annotated_frames() -> set[PaintFrame]:
    """Return the paint frames that have an order property on every paint node."""
    frames = set()
    for paint_node in commands.nodesOfType("RVPaint"):
        frames.update(annotated_frames(paint_node))
    return frames


def order_property_frame(property_name: str) -> PaintFrame | None:
    """Return the paint frame of a frame order property such as "RVPaint_1.frame:42.order"."""
    match property_name.split("."):
        case [paint_node, component, "order"]:
            kind, _, frame = component.partition(":")
            if kind == "frame":
                return PaintFrame(paint_node, int(frame))
    return None


def has_visible_components(paint_frame: PaintFrame) -> bool:
    order_property = frame_order_property(paint_frame)
    return commands.propertyExists(order_property) and bool(commands.getStringProperty(order_property))


def soft_delete_frame(paint_frame: PaintFrame) -> list[PaintComponent]:
    """Soft-delete every visible component on a frame and return them."""
    order_property = frame_order_property(paint_frame)
    if not commands.propertyExists(order_property):
        return []
    names = commands.getStringProperty(order_property)
    cleared = [PaintComponent(paint_frame, name) for name in names if name]
    for component in cleared:
        _ensure_property(f"{_node_name(component)}.softDeleted", commands.IntType, 1)
        commands.setIntProperty(f"{_node_name(component)}.softDeleted", [1], True)
    if names:
        commands.setStringProperty(order_property, [], True)
    return cleared


def set_tags():
    if commands.viewNode():
        for node in commands.closestNodesOfType("RVPaint"):
            annotate_tag = f"{node}.tag.annotate"
            _ensure_property(annotate_tag, commands.StringType, 1)
            commands.setStringProperty(annotate_tag, [""], True)


def remove_tags():
    if commands.viewNode():
        for node in commands.closestNodesOfType("RVPaint"):
            annotate_tag = f"{node}.tag.annotate"
            if commands.propertyExists(annotate_tag):
                commands.deleteProperty(annotate_tag)


# ----------------------------------------------------------------------
# Shapes
# ----------------------------------------------------------------------


def new_shape(paint_frame, prefix, anchor, current, style, border_width):
    """Create a rect/ellipse/arrow/line component and return it."""
    _ensure_visible(paint_frame.paint_node)
    component = _new_component(paint_frame, prefix)
    node_name = _node_name(component)
    color = style.color
    alpha = style.opacity / 100.0
    filled = prefix == "arrow" or (style.filled and prefix in BOX_PREFIXES)
    border_color = [color.redF(), color.greenF(), color.blueF(), alpha]
    inner_color = [color.redF(), color.greenF(), color.blueF(), alpha if filled else 0.0]
    shape_uuid = str(uuid.uuid4())

    _ensure_property(f"{node_name}.startFrame", commands.IntType, 1)
    _ensure_property(f"{node_name}.duration", commands.IntType, 1)
    _ensure_property(f"{node_name}.eye", commands.IntType, 1)
    commands.setIntProperty(f"{node_name}.startFrame", [paint_frame.frame], True)
    commands.setIntProperty(f"{node_name}.duration", [1], True)
    commands.setIntProperty(f"{node_name}.eye", [2], True)

    if prefix in BOX_PREFIXES:
        for suffix, width in ((".min", 2), (".max", 2), (".innerColor", 4), (".borderColor", 4), (".borderWidth", 1)):
            _ensure_property(f"{node_name}{suffix}", commands.FloatType, width)
        min_x = min(anchor.x, current.x)
        min_y = min(anchor.y, current.y)
        max_x = max(anchor.x, current.x)
        max_y = max(anchor.y, current.y)
        commands.setFloatProperty(f"{node_name}.min", [min_x, min_y], True)
        commands.setFloatProperty(f"{node_name}.max", [max_x, max_y], True)
        commands.setFloatProperty(f"{node_name}.innerColor", inner_color, True)
        commands.setFloatProperty(f"{node_name}.borderColor", border_color, True)
        commands.setFloatProperty(f"{node_name}.borderWidth", [border_width], True)
    else:
        for suffix, width in ((".startPos", 2), (".endPos", 2), (".borderColor", 4), (".borderWidth", 1)):
            _ensure_property(f"{node_name}{suffix}", commands.FloatType, width)
        commands.setFloatProperty(f"{node_name}.startPos", [anchor.x, anchor.y], True)
        commands.setFloatProperty(f"{node_name}.endPos", [current.x, current.y], True)
        commands.setFloatProperty(f"{node_name}.borderColor", border_color, True)
        commands.setFloatProperty(f"{node_name}.borderWidth", [border_width], True)
        if prefix == "arrow":
            _ensure_property(f"{node_name}.innerColor", commands.FloatType, 4)
            _ensure_property(f"{node_name}.thickness", commands.FloatType, 1)
            commands.setFloatProperty(f"{node_name}.innerColor", inner_color, True)
            commands.setFloatProperty(f"{node_name}.thickness", [border_width], True)

    _ensure_property(f"{node_name}.uuid", commands.StringType, 1)
    _ensure_property(f"{node_name}.softDeleted", commands.IntType, 1)
    commands.setStringProperty(f"{node_name}.uuid", [shape_uuid], True)
    commands.setIntProperty(f"{node_name}.softDeleted", [0], True)

    _add_to_order(component)
    commands.redraw()
    return component


def update_shape(component: PaintComponent, prefix, anchor, current):
    node_name = _node_name(component)
    if prefix in BOX_PREFIXES:
        commands.setFloatProperty(f"{node_name}.min", [min(anchor.x, current.x), min(anchor.y, current.y)], True)
        commands.setFloatProperty(f"{node_name}.max", [max(anchor.x, current.x), max(anchor.y, current.y)], True)
    else:
        commands.setFloatProperty(f"{node_name}.endPos", [current.x, current.y], True)
    commands.redraw()


# ----------------------------------------------------------------------
# Text
# ----------------------------------------------------------------------


def new_text_node(paint_frame: PaintFrame, position, settings, font_size) -> PaintComponent:
    """Create an empty text component at position and return it."""
    _ensure_visible(paint_frame.paint_node)
    component = _new_component(paint_frame, "text")
    node_name = _node_name(component)
    shape_uuid = str(uuid.uuid4())

    color = settings.tool_state.color
    rgba = [color.redF(), color.greenF(), color.blueF(), 1.0]
    font_weight = "bold" if settings.font_bold else "normal"
    font_style = "italic" if settings.font_italic else "normal"
    text_decoration = "underline" if settings.font_underline else "none"

    for suffix, property_type, width in (
        (".position", commands.FloatType, 2),
        (".color", commands.FloatType, 4),
        (".size", commands.FloatType, 1),
        (".scale", commands.FloatType, 1),
        (".rotation", commands.FloatType, 1),
        (".spacing", commands.FloatType, 1),
        (".font", commands.StringType, 1),
        (".text", commands.StringType, 1),
        (".origin", commands.StringType, 1),
        (".debug", commands.IntType, 1),
        (".startFrame", commands.IntType, 1),
        (".duration", commands.IntType, 1),
        (".mode", commands.IntType, 1),
    ):
        _ensure_property(f"{node_name}{suffix}", property_type, width)

    commands.setFloatProperty(f"{node_name}.position", [position.x, position.y], True)
    commands.setFloatProperty(f"{node_name}.color", rgba, True)
    commands.setFloatProperty(f"{node_name}.size", [0.01], True)
    commands.setFloatProperty(f"{node_name}.scale", [1.0], True)
    commands.setFloatProperty(f"{node_name}.rotation", [0.0], True)
    commands.setFloatProperty(f"{node_name}.spacing", [0.8], True)
    commands.setStringProperty(f"{node_name}.font", [""], True)
    commands.setStringProperty(f"{node_name}.text", ["|"], True)
    commands.setStringProperty(f"{node_name}.origin", [""], True)
    commands.setIntProperty(f"{node_name}.debug", [0], True)
    commands.setIntProperty(f"{node_name}.startFrame", [paint_frame.frame], True)
    commands.setIntProperty(f"{node_name}.duration", [1], True)
    commands.setIntProperty(f"{node_name}.mode", [0], True)

    for suffix, property_type, width in (
        (".fontFamily", commands.StringType, 1),
        (".fontSize", commands.FloatType, 1),
        (".fontWeight", commands.StringType, 1),
        (".fontStyle", commands.StringType, 1),
        (".textDecoration", commands.StringType, 1),
        (".textAlign", commands.StringType, 1),
    ):
        _ensure_property(f"{node_name}{suffix}", property_type, width)

    commands.setStringProperty(f"{node_name}.fontFamily", [settings.font_family], True)
    commands.setFloatProperty(f"{node_name}.fontSize", [font_size], True)
    commands.setStringProperty(f"{node_name}.fontWeight", [font_weight], True)
    commands.setStringProperty(f"{node_name}.fontStyle", [font_style], True)
    commands.setStringProperty(f"{node_name}.textDecoration", [text_decoration], True)
    commands.setStringProperty(f"{node_name}.textAlign", ["left"], True)

    _ensure_property(f"{node_name}.uuid", commands.StringType, 1)
    _ensure_property(f"{node_name}.softDeleted", commands.IntType, 1)
    commands.setStringProperty(f"{node_name}.uuid", [shape_uuid], True)
    commands.setIntProperty(f"{node_name}.softDeleted", [0], True)

    _add_to_order(component)
    commands.redraw()
    return component


def set_text(component: PaintComponent, text: str):
    commands.setStringProperty(f"{_node_name(component)}.text", [text], True)
    commands.redraw()


# ----------------------------------------------------------------------
# Pen / eraser strokes
# ----------------------------------------------------------------------


def new_stroke(paint_frame, first_point, first_point_width, brush, erase_mode, style):
    """Create a new pen/eraser stroke component and return it."""
    _ensure_visible(paint_frame.paint_node)
    component = _new_component(paint_frame, "pen")
    node_name = _node_name(component)
    stroke_uuid = str(uuid.uuid4())

    color = style.color
    alpha = style.opacity / 100.0
    blend_mode = style.color_modifier

    if blend_mode == constants.ColorModifier.ADDITIVE:
        scale = 1.0 + alpha
        rgba = [color.redF() * scale, color.greenF() * scale, color.blueF() * scale, alpha * alpha]
    elif blend_mode == constants.ColorModifier.DARKEN:
        scale = (1.0 - alpha) * 0.75 + 0.25
        rgba = [color.redF() * scale, color.greenF() * scale, color.blueF() * scale, alpha * alpha]
    else:
        rgba = [color.redF(), color.greenF(), color.blueF(), alpha]

    for suffix, property_type, width in (
        (".color", commands.FloatType, 4),
        (".width", commands.FloatType, 1),
        (".brush", commands.StringType, 1),
        (".uuid", commands.StringType, 1),
        (".points", commands.FloatType, 2),
        (".join", commands.IntType, 1),
        (".cap", commands.IntType, 1),
        (".splat", commands.IntType, 1),
        (".mode", commands.IntType, 1),
        (".debug", commands.IntType, 1),
        (".smoothingWidth", commands.FloatType, 1),
        (".startFrame", commands.IntType, 1),
        (".duration", commands.IntType, 1),
        # Stamp-brush properties (only take effect for brush names other than
        # "circle"/"gauss" — see PaintIPNode::compilePenComponent). Not yet
        # exposed in this UI; written here so future sliders/pickers have a
        # ready-made property to set.
        (".hardness", commands.FloatType, 1),
        (".tipTexture", commands.StringType, 1),
        (".blendMode", commands.IntType, 1),
    ):
        _ensure_property(f"{node_name}{suffix}", property_type, width)

    # mode: 0=OverMode, 1=EraseMode, 2=ScaleMode (burn/dodge use ScaleMode)
    if erase_mode:
        stroke_mode = 1
    elif blend_mode in (constants.ColorModifier.ADDITIVE, constants.ColorModifier.DARKEN):
        stroke_mode = 2
    else:
        stroke_mode = 0

    # Write every other property before .points: writing .points fires
    # graph-state-change, and listeners may read the whole stroke at that point.
    #
    # .width and .points are inserted, not set. sync.mu filters out sets on
    # RVPaint .points/.width to save bandwidth (MatchNoStateChange) and only
    # relays inserts, so a set first point never reaches remote sync peers.
    # Both properties are new and empty here, so the local result is the same.
    # Matches newStroke/addToStroke in annotate_mode.mu.
    _ensure_property(f"{node_name}.softDeleted", commands.IntType, 1)
    commands.setFloatProperty(f"{node_name}.color", rgba, True)
    commands.insertFloatProperty(f"{node_name}.width", [first_point_width])  # insert, see above
    commands.setStringProperty(f"{node_name}.brush", [brush], True)
    commands.setIntProperty(f"{node_name}.mode", [stroke_mode], True)
    commands.setIntProperty(f"{node_name}.startFrame", [paint_frame.frame], True)
    commands.setIntProperty(f"{node_name}.duration", [1], True)
    commands.setStringProperty(f"{node_name}.uuid", [stroke_uuid], True)
    commands.setIntProperty(f"{node_name}.softDeleted", [0], True)
    commands.insertFloatProperty(f"{node_name}.points", [first_point.x, first_point.y])  # insert, see above
    commands.setIntProperty(f"{node_name}.join", [3], True)  # RoundJoin
    commands.setIntProperty(f"{node_name}.cap", [1], True)  # SquareCap, rendered round by the radial shader
    commands.setIntProperty(f"{node_name}.splat", [1 if brush == constants.Brush.GAUSS else 0], True)
    commands.setIntProperty(f"{node_name}.debug", [0], True)
    commands.setFloatProperty(f"{node_name}.smoothingWidth", [1.0], True)
    commands.setFloatProperty(f"{node_name}.hardness", [100.0], True)
    commands.setStringProperty(f"{node_name}.tipTexture", [""], True)
    commands.setIntProperty(
        f"{node_name}.blendMode", [2 if blend_mode == constants.ColorModifier.ADDITIVE else 0], True
    )

    _add_to_order(component)
    commands.redraw()
    return component


def append_stroke_point(component: PaintComponent, point, width):
    node_name = _node_name(component)
    commands.insertFloatProperty(f"{node_name}.points", [point.x, point.y])
    commands.insertFloatProperty(f"{node_name}.width", [width])
