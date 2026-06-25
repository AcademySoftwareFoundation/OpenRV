# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
OTIO schemadef for a straight line annotation shape.

Lines have no fill — borderColor is the line colour. Ends are rounded (capsule).
All coordinates are in normalised image space (see position_schema.py).

Example:
    line = otio.schemadef.Line.Line(
        start_position=otio.schemadef.Position.Position(x=-4.0, y=2.0),
        end_position=otio.schemadef.Position.Position(x=4.0, y=-2.0),
        borderColor=[0.0, 1.0, 0.5, 1.0],
        width=0.04,
        id="d4e5f6a7-b8c9-0123-defa-234567890123",
        visible=True,
        layer_range=otio.opentime.TimeRange(...),
    )
"""

import opentimelineio as otio


@otio.core.register_type
class Line(otio.core.SerializableObject):
    """A straight line annotation. First-class type, not a degenerate Arrow."""

    _serializable_label = "Line.1"
    _name = "Line"

    def __init__(
        self,
        start_position=None,
        end_position=None,
        borderColor: list | None = None,
        width: float | None = None,
        id: str | None = None,
        visible: bool | None = None,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.start_position = start_position
        self.end_position = end_position
        self.borderColor = borderColor
        self.width = width
        self.id = id
        self.visible = visible
        self.layer_range = layer_range

    start_position = otio.core.serializable_field("start_position", doc="Start point (Position.1)")
    end_position = otio.core.serializable_field("end_position", doc="End point (Position.1)")

    _borderColor = otio.core.serializable_field("borderColor", required_type=list, doc="Line colour [r, g, b, a]")

    @property
    def borderColor(self) -> list:
        return self._borderColor

    @borderColor.setter
    def borderColor(self, val: list) -> None:
        self._borderColor = val

    width = otio.core.serializable_field("width", required_type=float, doc="Line width in normalised units")
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
        return f"Line({self.id}, {self.start_position} → {self.end_position})"

    def __repr__(self) -> str:
        return (
            f"otio.schemadef.Line.Line("
            f"id={self.id!r}, start_position={self.start_position!r}, "
            f"end_position={self.end_position!r}, width={self.width!r})"
        )
