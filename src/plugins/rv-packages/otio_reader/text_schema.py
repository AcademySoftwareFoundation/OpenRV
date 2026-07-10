# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
OTIO schemadef for a text annotation shape.

Font properties are discrete fields so each platform constructs its own
representation (CSS for web clients, QFont for RV) without string parsing.
anchor is the typographic baseline origin (bottom-left of the first character,
excluding descenders). This matches the CSS/web convention. Renderers that
use a top-left origin (e.g. RVPaint) must offset upward by one em (font_size).

font_size is in OTIO normalised coordinate units (same space as border_width
and all other scalar dimensions). For standard 16:9 content the OTIO y-range
is 9 units, so small ≈ 0.15, medium ≈ 0.25, large ≈ 0.5.
RVPaint's fontSize is a WCS fraction, exactly like border_width: divide by
bounds_size to reach WCS and store that directly — no pixel conversion.
The renderer multiplies by framebuffer height at draw time (PaintCommand.cpp).

All coordinates are in normalised image space (see position_schema.py).

Example:
    text = otio.schemadef.Text.Text(
        anchor=otio.schemadef.Position.Position(x=-3.0, y=2.0),
        text="Frame needs colour correction",
        font_family="Inter",
        font_size=0.22,
        font_weight="NORMAL",
        font_style="NORMAL",
        text_decoration="NONE",
        text_align="LEFT",
        inner_color=[1.0, 1.0, 1.0, 1.0],
        id="e5f6a7b8-c9d0-1234-efab-345678901234",
        visible=True,
        layer_range=otio.opentime.TimeRange(...),
    )
"""

import opentimelineio as otio


@otio.core.register_type
class Text(otio.core.SerializableObject):
    """A text annotation."""

    _serializable_label = "Text.1"
    _name = "Text"

    def __init__(
        self,
        anchor=None,
        text: str | None = None,
        font_family: str | None = None,
        font_size: float | None = None,
        font_weight: str | None = None,
        font_style: str | None = None,
        text_decoration: str | None = None,
        text_align: str | None = None,
        inner_color: list | None = None,
        id: str | None = None,
        visible: bool | None = None,
        soft_deleted: bool | None = None,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.anchor = anchor
        self.text = text
        self.font_family = font_family
        self.font_size = font_size
        self.font_weight = font_weight
        self.font_style = font_style
        self.text_decoration = text_decoration
        self.text_align = text_align
        self.inner_color = inner_color
        self.id = id
        self.visible = visible
        self.soft_deleted = soft_deleted
        self.layer_range = layer_range

    anchor = otio.core.serializable_field("anchor", doc="Baseline start point (Position.1)")
    text = otio.core.serializable_field("text", required_type=str, doc="Text content")
    font_family = otio.core.serializable_field("font_family", required_type=str, doc="Font family, e.g. 'Inter'")
    font_size = otio.core.serializable_field(
        "font_size", required_type=float, doc="Font size in normalised units (relative to image height)"
    )

    # "NORMAL" | "BOLD"
    font_weight = otio.core.serializable_field("font_weight", required_type=str, doc="Font weight: 'NORMAL' or 'BOLD'")

    # "NORMAL" | "ITALIC"
    font_style = otio.core.serializable_field("font_style", required_type=str, doc="Font style: 'NORMAL' or 'ITALIC'")

    # "NONE" | "UNDERLINE"
    text_decoration = otio.core.serializable_field(
        "text_decoration", required_type=str, doc="Text decoration: 'NONE' or 'UNDERLINE'"
    )

    # "LEFT" | "CENTER" | "RIGHT"
    text_align = otio.core.serializable_field(
        "text_align", required_type=str, doc="Text alignment relative to anchor: 'LEFT', 'CENTER', or 'RIGHT'"
    )

    _inner_color = otio.core.serializable_field("inner_color", required_type=list, doc="Text fill colour [r, g, b, a]")

    @property
    def inner_color(self) -> list:
        return self._inner_color

    @inner_color.setter
    def inner_color(self, val: list) -> None:
        self._inner_color = val

    id = otio.core.serializable_field("id", required_type=str, doc="UUID for undo/redo tracking")
    visible = otio.core.serializable_field("visible", required_type=bool, doc="Show/hide")
    soft_deleted = otio.core.serializable_field(
        "soft_deleted", required_type=bool, doc="Undo/redo soft-delete flag, consistent with Paint.2"
    )

    _layer_range = otio.core.serializable_field(
        "layer_range",
        required_type=otio.opentime.TimeRange,
        doc="Time range this annotation is visible on",
    )

    @property
    def layer_range(self) -> otio.opentime.TimeRange:
        return self._layer_range

    @layer_range.setter
    def layer_range(self, val) -> None:
        self._layer_range = val

    def __str__(self) -> str:
        return f"Text({self.id}, {self.text!r})"

    def __repr__(self) -> str:
        return (
            f"otio.schemadef.Text.Text("
            f"id={self.id!r}, text={self.text!r}, font_family={self.font_family!r}, "
            f"font_size={self.font_size!r}, anchor={self.anchor!r})"
        )
