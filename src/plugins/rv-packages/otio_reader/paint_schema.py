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

```python
Example:
myObject = otio.schemadef.Paint.Paint(
    name, points, rgba, type, brush, layer_range, hold, ghost
)
"""

import opentimelineio as otio


@otio.core.register_type
class Paint(otio.core.SerializableObject):
    """A schema for the start of an annotation"""

    _serializable_label = "Paint.1"
    _name = "Paint"

    def __init__(
        self,
        name: str = "",
        points: list | None = None,
        rgba: list | None = None,
        type: str = "",
        brush: str = "",
        layer_range: dict | None = None,
        hold: bool = False,
        ghost: bool = False,
    ) -> None:
        super().__init__()
        self.name = name
        self.points = points
        self.rgba = rgba
        self.type = type
        self.brush = brush
        self.layer_range = layer_range
        self.hold = hold
        self.ghost = ghost

    name = otio.core.serializable_field(
        "name", required_type=str, doc=("Name: expects a string")
    )

    _points = otio.core.serializable_field(
        "points", required_type=list, doc=("Points: expects a list of point objects")
    )

    @property
    def points(self) -> list:
        return self._points

    @points.setter
    def points(self, val: list) -> None:
        self._points = val

    _rgba = otio.core.serializable_field(
        "rgba", required_type=list, doc=("RGBA: expects a list of four floats")
    )

    @property
    def rgba(self) -> list:
        return self._rgba

    @rgba.setter
    def rgba(self, val: list) -> list:
        self._rgba = val

    type = otio.core.serializable_field(
        "type", required_type=str, doc=("Type: expects a string")
    )

    brush = otio.core.serializable_field(
        "brush", required_type=str, doc=("Brush: expects a string")
    )

    _layer_range = otio.core.serializable_field(
        "layer_range",
        required_type=otio.opentime.TimeRange,
        doc=("Layer_range: expects a TimeRange object"),
    )

    @property
    def layer_range(self) -> otio.opentime.TimeRange:
        return self._layer_range

    @layer_range.setter
    def layer_range(self, val) -> otio.opentime.TimeRange:
        self._layer_range = val

    _hold = otio.core.serializable_field(
        "hold", required_type=bool, doc=("Hold: expects either true or false")
    )

    _ghost = otio.core.serializable_field(
        "ghost", required_type=bool, doc=("Ghost: expects either true or false")
    )

    def __str__(self) -> str:
        return (
            f"Paint({self.name}, {self.points}, {self.rgba}, {self.type}, "
            f"{self.brush}, {self.layer_range}, {self.hold}, {self.ghost})"
        )

    def __repr__(self) -> str:
        return (
            f"otio.schema.Paint(name={self.name!r}, points={self.points!r}, "
            f"rgba={self.rgba!r}, type={self.type!r}, brush={self.brush!r}, "
            f"layer_range={self.layer_range!r}, hold={self.hold!r}, "
            f"ghost={self.ghost!r})"
        )
