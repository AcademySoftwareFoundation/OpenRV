# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
OTIO schemadef for a rectangle annotation shape.

Geometry uses min/max corners to avoid top-left vs centre ambiguity.
The border is centre-aligned on the shape edge (half inside, half outside).
All coordinates are in normalised image space (see position_schema.py).

Example:
    rect = otio.schemadef.Rectangle.Rectangle(
        min=otio.schemadef.Position.Position(x=-2.0, y=-1.0),
        max=otio.schemadef.Position.Position(x=2.0, y=1.0),
        inner_color=[1.0, 0.0, 0.0, 0.3],
        border_color=[1.0, 0.0, 0.0, 1.0],
        border_width=0.05,
        corner_radius=0.0,
        id="a1b2c3d4-e5f6-7890-abcd-ef1234567890",
        visible=True,
        layer_range=otio.opentime.TimeRange(...),
    )
"""

import opentimelineio as otio


@otio.core.register_type
class Rectangle(otio.core.SerializableObject):
    """An axis-aligned rectangle annotation."""

    _serializable_label = "Rectangle.1"
    _name = "Rectangle"

    def __init__(
        self,
        min=None,
        max=None,
        inner_color: list | None = None,
        border_color: list | None = None,
        border_width: float | None = None,
        corner_radius: float | None = None,
        id: str | None = None,
        visible: bool | None = None,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.min = min
        self.max = max
        self.inner_color = inner_color
        self.border_color = border_color
        self.border_width = border_width
        self.corner_radius = corner_radius
        self.id = id
        self.visible = visible
        self.layer_range = layer_range

    # Position.1 objects — required_type omitted to avoid load-order dependency
    min = otio.core.serializable_field("min", doc="Top-left corner (Position.1)")
    max = otio.core.serializable_field("max", doc="Bottom-right corner (Position.1)")

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
    corner_radius = otio.core.serializable_field(
        "corner_radius", required_type=float, doc="Corner radius in normalised units; 0.0 = sharp"
    )
    id = otio.core.serializable_field("id", required_type=str, doc="UUID for undo/redo tracking")
    visible = otio.core.serializable_field("visible", required_type=bool, doc="Show/hide")

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
        return f"Rectangle({self.id}, min={self.min}, max={self.max})"

    def __repr__(self) -> str:
        return (
            f"otio.schemadef.Rectangle.Rectangle("
            f"id={self.id!r}, min={self.min!r}, max={self.max!r}, "
            f"border_width={self.border_width!r}, corner_radius={self.corner_radius!r})"
        )
