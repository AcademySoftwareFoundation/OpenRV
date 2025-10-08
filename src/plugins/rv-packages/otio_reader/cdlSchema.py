# *****************************************************************************
# Copyright 2018 Autodesk, Inc. All rights reserved.
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
myObject = otio.schemadef.CDL.CDL( name, visible, slope, offest, power, saturation )

"""

import opentimelineio as otio


@otio.core.register_type
class CDL(otio.schema.Effect):
    """A schema for SGC CDL."""

    _serializable_label = "CDL.1"
    _name = "CDL"

    def __init__(
        self,
        name="",
        visible=None,
        slope=None,
        offset=None,
        power=None,
        saturation=None,
    ):
        otio.schema.Effect.__init__(self, name=name, effect_name="CDL")
        self.visible = visible
        self.slope = slope
        self.offset = offset
        self.power = power
        self.saturation = saturation

    visible = otio.core.serializable_field("visible", required_type=bool, doc=("Visible: expects either true or false"))

    _slope = otio.core.serializable_field("slope", required_type=list, doc=("Slope: expects a list of three floats"))

    @property
    def slope(self):
        return self._slope

    @slope.setter
    def slope(self, val):
        if val:
            if len(val) != 3:
                raise Exception("Invalid slope length")
            elif not all(isinstance(element, float) for element in val):
                raise Exception("Invalid slope element type")
        self._slope = val

    _offset = otio.core.serializable_field("offset", required_type=list, doc=("Offset: expects a list of three floats"))

    @property
    def offset(self):
        return self._offset

    @offset.setter
    def offset(self, val):
        if val:
            if len(val) != 3:
                raise Exception("Invalid offset length")
            elif not all(isinstance(element, float) for element in val):
                raise Exception("Invalid offset element type")
        self._offset = val

    _power = otio.core.serializable_field("power", required_type=list, doc=("Visible: expects a list of three floats"))

    @property
    def power(self):
        return self._power

    @power.setter
    def power(self, val):
        if val:
            if len(val) != 3:
                raise Exception("Invalid power length")
            elif not all(isinstance(element, float) for element in val):
                raise Exception("Invalid power element type")
        self._power = val

    saturation = otio.core.serializable_field("saturation", required_type=float, doc=("Saturation: expects a float"))

    def __str__(self):
        return 'CDL("{}", "{}", "{}", "{}", "{}", "{}", "{}")'.format(
            str(self.name),
            str(self.effect_name),
            str(self.visible),
            str(self.slope),
            str(self.offset),
            str(self.power),
            str(self.saturation),
        )

    def __repr__(self):
        return (
            "otio.schema.CDL("
            "name={}, "
            "effect_name={}, "
            "visible={}, "
            "slope={}, "
            "offset={}, "
            "power={}, "
            "saturation={} "
            ")".format(
                repr(self.name),
                repr(self.effect_name),
                repr(self.visible),
                repr(self.slope),
                repr(self.offset),
                repr(self.power),
                repr(self.saturation),
            )
        )
