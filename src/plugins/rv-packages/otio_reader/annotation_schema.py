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
myObject = otio.schemadef.Annotation.Annotation(name, visible, layers)
"""

import opentimelineio as otio


@otio.core.register_type
class Annotation(otio.schema.Effect):
    """A schema for annotations."""

    _serializable_label = "Annotation.1"
    _name = "Annotation"

    def __init__(
        self, name: str = "", visible: bool = True, layers: list | None = None
    ) -> None:
        super().__init__(name=name, effect_name="Annotation.1")
        self.visible = visible
        self.layers = layers

    _visible = otio.core.serializable_field(
        "visible", required_type=bool, doc=("Visible: expects either true or false")
    )

    _layers = otio.core.serializable_field(
        "layers", required_type=list, doc=("Layers: expects a list of annotation types")
    )

    @property
    def layers(self) -> list:
        return self._layers

    @layers.setter
    def layers(self, val: list):
        self._layers = val

    def __str__(self) -> str:
        return f"Annotation({self.name}, {self.effect_name}, {self.visible}, {self.layers})"

    def __repr__(self) -> str:
        return f"otio.schema.Annotation(name={self.name!r}, effect_name={self.effect_name!r}, visible={self.visible!r}, layers={self.layers!r})"
