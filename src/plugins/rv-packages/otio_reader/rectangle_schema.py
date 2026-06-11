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
        innerColor=[1.0, 0.0, 0.0, 0.3],
        borderColor=[1.0, 0.0, 0.0, 1.0],
        borderWidth=0.05,
        cornerRadius=0.0,
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
        innerColor: list | None = None,
        borderColor: list | None = None,
        borderWidth: float | None = None,
        cornerRadius: float | None = None,
        id: str | None = None,
        visible: bool | None = None,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.min = min
        self.max = max
        self.innerColor = innerColor
        self.borderColor = borderColor
        self.borderWidth = borderWidth
        self.cornerRadius = cornerRadius
        self.id = id
        self.visible = visible
        self.layer_range = layer_range

    # Position.1 objects — required_type omitted to avoid load-order dependency
    min = otio.core.serializable_field("min", doc="Top-left corner (Position.1)")
    max = otio.core.serializable_field("max", doc="Bottom-right corner (Position.1)")

    _innerColor = otio.core.serializable_field("innerColor", required_type=list, doc="Fill colour [r, g, b, a]")

    @property
    def innerColor(self) -> list:
        return self._innerColor

    @innerColor.setter
    def innerColor(self, val: list) -> None:
        self._innerColor = val

    _borderColor = otio.core.serializable_field("borderColor", required_type=list, doc="Outline colour [r, g, b, a]")

    @property
    def borderColor(self) -> list:
        return self._borderColor

    @borderColor.setter
    def borderColor(self, val: list) -> None:
        self._borderColor = val

    borderWidth = otio.core.serializable_field(
        "borderWidth", required_type=float, doc="Outline width in normalised units"
    )
    cornerRadius = otio.core.serializable_field(
        "cornerRadius", required_type=float, doc="Corner radius in normalised units; 0.0 = sharp"
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
            f"borderWidth={self.borderWidth!r}, cornerRadius={self.cornerRadius!r})"
        )
