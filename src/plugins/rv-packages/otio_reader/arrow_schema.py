# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
OTIO schemadef for an arrow annotation shape.

The arrowhead is at end_position and scales proportionally with width.
All coordinates are in normalised image space (see position_schema.py).

Example:
    arrow = otio.schemadef.Arrow.Arrow(
        start_position=otio.schemadef.Position.Position(x=-4.0, y=0.0),
        end_position=otio.schemadef.Position.Position(x=4.0, y=0.0),
        inner_color=[1.0, 1.0, 0.0, 1.0],
        border_color=[0.8, 0.8, 0.0, 1.0],
        width=0.15,
        border_width=0.02,
        id="c3d4e5f6-a7b8-9012-cdef-123456789012",
        visible=True,
        layer_range=otio.opentime.TimeRange(...),
    )
"""

import opentimelineio as otio


@otio.core.register_type
class Arrow(otio.core.SerializableObject):
    """An arrow annotation with a filled shaft and arrowhead at end_position."""

    _serializable_label = "Arrow.1"
    _name = "Arrow"

    def __init__(
        self,
        start_position=None,
        end_position=None,
        inner_color: list | None = None,
        border_color: list | None = None,
        width: float | None = None,
        border_width: float | None = None,
        id: str | None = None,
        visible: bool | None = None,
        soft_deleted: bool = False,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.start_position = start_position
        self.end_position = end_position
        self.inner_color = inner_color
        self.border_color = border_color
        self.width = width
        self.border_width = border_width
        self.id = id
        self.visible = visible
        self.soft_deleted = soft_deleted
        self.layer_range = layer_range

    start_position = otio.core.serializable_field("start_position", doc="Tail of the arrow (Position.1)")
    end_position = otio.core.serializable_field(
        "end_position", doc="Tip of the arrow — arrowhead points here (Position.1)"
    )

    _inner_color = otio.core.serializable_field("inner_color", required_type=list, doc="Shaft fill colour [r, g, b, a]")

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

    width = otio.core.serializable_field(
        "width", required_type=float, doc="Shaft width in normalised units; arrowhead scales proportionally"
    )
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
        return f"Arrow({self.id}, {self.start_position} → {self.end_position})"

    def __repr__(self) -> str:
        return (
            f"otio.schemadef.Arrow.Arrow("
            f"id={self.id!r}, start_position={self.start_position!r}, "
            f"end_position={self.end_position!r}, width={self.width!r})"
        )
