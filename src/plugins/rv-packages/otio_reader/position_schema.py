# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
Schemadef for a 2D point in normalised image coordinate space.

Coordinates are bounds-relative, centred at the image origin.
For 16:9 content: x ∈ [-8, 8], y ∈ [-4.5, 4.5].

Example:
    pos = otio.schemadef.Position.Position(x=1.5, y=-0.5)
"""

import opentimelineio as otio


@otio.core.register_type
class Position(otio.core.SerializableObject):
    """A 2D point in normalised image coordinate space."""

    _serializable_label = "Position.1"
    _name = "Position"

    def __init__(self, x: float | None = None, y: float | None = None) -> None:
        super().__init__()
        self.x = x
        self.y = y

    x = otio.core.serializable_field("x", required_type=float, doc="x coordinate")
    y = otio.core.serializable_field("y", required_type=float, doc="y coordinate")

    def __str__(self) -> str:
        return f"Position({self.x}, {self.y})"

    def __repr__(self) -> str:
        return f"otio.schemadef.Position.Position(x={self.x!r}, y={self.y!r})"
