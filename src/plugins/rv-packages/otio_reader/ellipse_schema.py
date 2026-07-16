# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
OTIO schemadef for an ellipse annotation shape.

Geometry uses min/max bounding-box corners.
All coordinates are in normalised image space (see position_schema.py).

Example:
    ellipse = otio.schemadef.Ellipse.Ellipse(
        min=otio.schemadef.Position.Position(x=-2.0, y=-1.0),
        max=otio.schemadef.Position.Position(x=2.0, y=1.0),
        inner_color=[0.0, 0.5, 1.0, 0.2],
        border_color=[0.0, 0.5, 1.0, 1.0],
        border_width=0.05,
        id="b2c3d4e5-f6a7-8901-bcde-f12345678901",
        visible=True,
        layer_range=otio.opentime.TimeRange(...),
    )
"""

import opentimelineio as otio


@otio.core.register_type
class Ellipse(otio.core.SerializableObject):
    """An axis-aligned ellipse annotation."""

    _serializable_label = "Ellipse.1"
    _name = "Ellipse"

    def __init__(
        self,
        min=None,
        max=None,
        inner_color: list | None = None,
        border_color: list | None = None,
        border_width: float | None = None,
        id: str | None = None,
        visible: bool | None = None,
        soft_deleted: bool = False,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.min = min
        self.max = max
        self.inner_color = inner_color
        self.border_color = border_color
        self.border_width = border_width
        self.id = id
        self.visible = visible
        self.soft_deleted = soft_deleted
        self.layer_range = layer_range

    min = otio.core.serializable_field("min", doc="Top-left corner of bounding box (Position.1)")
    max = otio.core.serializable_field("max", doc="Bottom-right corner of bounding box (Position.1)")

    _inner_color = otio.core.serializable_field("inner_color", required_type=list, doc="Fill colour [r, g, b, a]")

    @property
    def inner_color(self) -> list:
        return self._inner_color

    @inner_color.setter
    def inner_color(self, val: list) -> None:
        self._inner_color = val

    _border_color = otio.core.serializable_field("border_color", required_type=list, doc="Outline colour [r, g, b, a]")

    @property
    def border_color(self) -> list:
        return self._border_color

    @border_color.setter
    def border_color(self, val: list) -> None:
        self._border_color = val

    border_width = otio.core.serializable_field(
        "border_width", required_type=float, doc="Outline width in normalised units"
    )
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
        return f"Ellipse({self.id}, min={self.min}, max={self.max})"

    def __repr__(self) -> str:
        return (
            f"otio.schemadef.Ellipse.Ellipse("
            f"id={self.id!r}, min={self.min!r}, max={self.max!r}, border_width={self.border_width!r})"
        )
