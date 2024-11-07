# *****************************************************************************
# Copyright 2024 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
For our OTIO output to effectively interface with other programs
using the OpenTimelineIO Python API, our custom schema need to be
specified and registered with the API.

As per OTIO documentation, a class such as this one must be created,
the schema must be registered with a PluginManifest, and the path to that
manifest must be added to $OTIO_PLUGIN_MANIFEST_PATH; then the schema
is ready to be used.

Example:
myObject = otio.schemadef.Point.Point(
    name, points, rgba, type, brush, layer_range
)
"""

import opentimelineio as otio


@otio.core.register_type
class Point(otio.core.SerializableObject):
    """A schema for points forming an annotation"""

    _serializable_label = "Point.1"
    _name = "Point"

    def __init__(
        self, 
        width: float | None = None,
        x: float | None = None,
        y: float | None = None
    ) -> None:
        super().__init__()
        self.width = width
        self.x = x
        self.y = y

    width = otio.core.serializable_field(
        "width", required_type=float, doc=("Width: expect a float")
    )

    x = otio.core.serializable_field(
        "x", required_type=float, doc=("x: expects a float")
    )

    y = otio.core.serializable_field(
        "y", required_type=float, doc=("y: expects a float")
    )

    def __str__(self) -> str:
        return f"Point{self.width}, {self.x}, {self.y}"

    def __repr__(self) -> str:
        return (
            f"otio.schema.Point(width={self.width!r}, x={self.x!r}, "
            f"y={self.y!r})"
        )
