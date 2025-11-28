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
myObject = otio.schemadef.Compare.Compare(name, visible, mode, side_by_side, difference, over_with_opacity, angular_mask, secondaryElementId)
"""

import opentimelineio as otio


@otio.core.register_type
class Compare(otio.schema.Effect):
    """A schema for comparing two clips."""

    _serializable_label = "Compare.1"
    _name = "Compare"

    def __init__(
        self,
        name: str = "",
        visible: bool = True,
        mode: str = "",
        side_by_side: dict | None = None,
        difference: dict | None = None,
        over_with_opacity: dict | None = None,
        angular_mask: dict | None = None,
        secondary_element_id: str | None = None,
        is_swapped: bool = False,
    ) -> None:
        super().__init__(name=name, effect_name="Compare.1")
        self.visible = visible
        self.mode = mode
        self.side_by_side = side_by_side
        self.difference = difference
        self.over_with_opacity = over_with_opacity
        self.angular_mask = angular_mask
        self.secondary_element_id = secondary_element_id
        self.is_swapped = is_swapped

    visible = otio.core.serializable_field("visible", required_type=bool, doc="visible: expects either true or false")

    mode = otio.core.serializable_field("mode", required_type=str, doc="mode: expects a string")

    _side_by_side = otio.core.serializable_field(
        "side_by_side",
        required_type=dict,
        doc="mode: expects a dict containing a gap and an orientation",
    )

    @property
    def side_by_side(self) -> dict | None:
        return self._side_by_side

    @side_by_side.setter
    def side_by_side(self, val: dict | None) -> None:
        self._side_by_side = val

    _difference = otio.core.serializable_field(
        "difference",
        required_type=dict,
        doc="difference: expects a dict containing an offset and a slope",
    )

    @property
    def difference(self) -> dict | None:
        return self._difference

    @difference.setter
    def difference(self, val: dict | None) -> None:
        self._difference = val

    _over_with_opacity = otio.core.serializable_field(
        "over_with_opacity",
        required_type=dict,
        doc="over_with_opacity: expects a dict containing an opacity",
    )

    @property
    def over_with_opacity(self) -> dict | None:
        return self._over_with_opacity

    @over_with_opacity.setter
    def over_with_opacity(self, val: dict | None) -> None:
        self._over_with_opacity = val

    _angular_mask = otio.core.serializable_field(
        "angular_mask",
        required_type=dict,
        doc="angular_mask: expects a dict containing a pivot and an angle in radians",
    )

    @property
    def angular_mask(self) -> dict | None:
        return self._angular_mask

    @angular_mask.setter
    def angular_mask(self, val: dict | None) -> None:
        self._angular_mask = val

    _secondary_element_id = otio.core.serializable_field(
        "secondaryElementId",
        required_type=str,
        doc="SecondaryElementId: expects a string in the format /<type>/<id>",
    )

    @property
    def secondary_element_id(self) -> str | None:
        return self._secondary_element_id

    @secondary_element_id.setter
    def secondary_element_id(self, val: str | None) -> None:
        self._secondary_element_id = val

    is_swapped = otio.core.serializable_field(
        "is_swapped", required_type=bool, doc="is_swapped: expects either true or false"
    )

    def __str__(self) -> str:
        return (
            f"Compare({self.name}, {self.effect_name}, {self.visible}, {self.mode},"
            f" {self.side_by_side}, {self.difference}, {self.over_with_opacity},"
            f" {self.angular_mask}, {self.secondary_element_id}, {self.is_swapped}"
        )

    def __repr__(self) -> str:
        return (
            f"otio.schema.Compare(name={self.name!r}, effect_name={self.effect_name!r},"
            f" visible={self.visible!r}, mode={self.mode!r},"
            f" side_by_side={self.side_by_side!r}, difference={self.difference!r},"
            f" over_with_opacity={self.over_with_opacity},"
            f" angular_mask={self.angular_mask!r},"
            f" secondary_element_id={self.secondary_element_id!r},"
            f" is_swapped={self.is_swapped!r})"
        )
