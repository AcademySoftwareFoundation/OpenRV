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
    name, points, rgba, type, brush, layer_range, hold, ghost, ghost_before, ghost_after, visible
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
        layer_range: otio.opentime.TimeRange | None = None,
        hold: bool = False,
        ghost: bool = False,
        ghost_before: int = 0,
        ghost_after: int = 0,
        visible: bool = True,
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
        self.ghost_before = ghost_before
        self.ghost_after = ghost_after
        self.visible = visible

    name = otio.core.serializable_field(
        "name", required_type=str, doc=("name: expects a string")
    )

    _points = otio.core.serializable_field(
        "points", required_type=list, doc=("points: expects a list of point objects")
    )

    @property
    def points(self) -> list:
        return self._points

    @points.setter
    def points(self, val: list):
        self._points = val

    _rgba = otio.core.serializable_field(
        "rgba", required_type=list, doc=("rgba: expects a list of four floats")
    )

    @property
    def rgba(self) -> list:
        return self._rgba

    @rgba.setter
    def rgba(self, val: list) -> None:
        self._rgba = val

    type = otio.core.serializable_field(
        "type", required_type=str, doc=("type: expects a string")
    )

    brush = otio.core.serializable_field(
        "brush", required_type=str, doc=("brush: expects a string")
    )

    _layer_range = otio.core.serializable_field(
        "layer_range",
        required_type=otio.opentime.TimeRange,
        doc=("layer_range: expects a TimeRange object"),
    )

    @property
    def layer_range(self) -> otio.opentime.TimeRange:
        return self._layer_range

    @layer_range.setter
    def layer_range(self, val):
        self._layer_range = val

    hold = otio.core.serializable_field(
        "hold", required_type=bool, doc=("hold: expects either true or false")
    )

    ghost = otio.core.serializable_field(
        "ghost", required_type=bool, doc=("ghost: expects either true or false")
    )

    _ghost_before = otio.core.serializable_field(
        "ghost_before", required_type=int, doc=("ghost_before: expects an integer")
    )

    _ghost_after = otio.core.serializable_field(
        "ghost_after", required_type=int, doc=("ghost_after: expects an integer")
    )

    visible = otio.core.serializable_field(
        "visible", required_type=bool, doc=("visible: expects either true or false")
    )

    def __str__(self) -> str:
        return (
            f"Paint({self.name}, {self.points}, {self.rgba}, {self.type}, "
            f"{self.brush}, {self.layer_range}, {self.hold}, {self.ghost}, "
            f"{self.ghost_before}, {self.ghost_after}, {self.visible})"
        )

    def __repr__(self) -> str:
        return (
            f"otio.schema.Paint(name={self.name!r}, points={self.points!r}, "
            f"rgba={self.rgba!r}, type={self.type!r}, brush={self.brush!r}, "
            f"layer_range={self.layer_range!r}, hold={self.hold!r}, "
            f"ghost={self.ghost!r}, ghost_before={self.ghost_before!r}, "
            f"ghost_after={self.ghost_after!r}, visible={self.visible!r})"
        )
