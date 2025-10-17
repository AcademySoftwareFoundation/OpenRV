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
myObject = otio.schemadef.Annotation.Annotation(name, visible, layers, hold, ghost, ghost_before, ghost_after)
"""

import opentimelineio as otio


@otio.core.register_type
class Annotation(otio.schema.Effect):
    """A schema for annotations."""

    _serializable_label = "Annotation.1"
    _name = "Annotation"

    def __init__(
        self,
        name: str = "",
        visible: bool = True,
        layers: list | None = None,
        hold: bool = False,
        ghost: bool = False,
        ghost_before: int = 5,
        ghost_after: int = 5,
    ) -> None:
        super().__init__(name=name, effect_name="Annotation.1")
        self.visible = visible
        self.layers = layers
        self.hold = hold
        self.ghost = ghost
        self.ghost_before = ghost_before
        self.ghost_after = ghost_after

    _visible = otio.core.serializable_field(
        "visible", required_type=bool, doc=("visible: expects either true or false")
    )

    _layers = otio.core.serializable_field(
        "layers", required_type=list, doc=("layers: expects a list of annotation types")
    )

    @property
    def layers(self) -> list:
        return self._layers

    @layers.setter
    def layers(self, val: list):
        self._layers = val

    hold = otio.core.serializable_field("hold", required_type=bool, doc=("hold: expects either true or false"))

    ghost = otio.core.serializable_field("ghost", required_type=bool, doc=("ghost: expects either true or false"))

    ghost_before = otio.core.serializable_field(
        "ghost_before", required_type=int, doc=("ghost_before: expects an integer")
    )

    ghost_after = otio.core.serializable_field(
        "ghost_after", required_type=int, doc=("ghost_after: expects an integer")
    )

    def __str__(self) -> str:
        return (
            f"Annotation({self.name}, {self.effect_name}, {self.metadata}, "
            f"{self.layers}), {self.visible}, {self.hold}, {self.ghost}, "
            f"{self.ghost_before}, {self.ghost_after}"
        )

    def __repr__(self) -> str:
        return (
            f"otio.schema.Annotation(name={self.name!r}, "
            f"effect_name={self.effect_name!r}, "
            f"metadata={self.metadata!r}, "
            f"visible={self.visible!r}, layers={self.layers!r}), "
            f"hold={self.hold!r}, ghost={self.ghost!r}, "
            f"ghost_before={self.ghost_before!r}, ghost_after={self.ghost_after!r}"
        )
