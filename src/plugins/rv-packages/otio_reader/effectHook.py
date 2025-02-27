#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
import sys
from rv import commands
import opentimelineio as otio

SET_NODE_TYPE_MAP = {
    float: commands.setFloatProperty,
    bool: commands.setIntProperty,
    int: commands.setIntProperty,
    str: commands.setStringProperty,
}


ADD_NODE_TYPE_MAP = {
    float: commands.FloatType,
    bool: commands.IntType,
    int: commands.IntType,
    str: commands.StringType,
}

if sys.version_info < (3,):
    SET_NODE_TYPE_MAP[unicode] = commands.StringType
    ADD_NODE_TYPE_MAP[unicode] = commands.StringType


class NoMappingForRvNode(Exception):
    pass


def set_rv_effect_props(node_component, prop_map):
    """
    Set values on existing rv properties on a node component.

    Parameters
        ----------
        node_component : str
            The RV node and component name to set
        prop_map : Dictionary
            map of prop names of node_component to their value
    """
    for prop, value in prop_map.items():
        if value is not None:
            _set_rv_prop("{}.{}".format(node_component, prop), _get_rv_value(value))


def add_rv_effect_props(node_component, prop_map):
    """
    Add and set values on new rv properties to a node component.

    Parameters
        ----------
        node_component : str
            The RV node and component name to add to
        prop_map : Dictionary
            map of prop names of node_component to their value
    """
    for prop, value in prop_map.items():
        if value is not None:
            prop_name = "{}.{}".format(node_component, prop)
            rv_value = _get_rv_value(value)

            commands.newProperty(
                prop_name, ADD_NODE_TYPE_MAP[type(rv_value[0])], len(rv_value)
            )
            _set_rv_prop(prop_name, rv_value)


def add_otio_metadata(node_component, otio_node):
    """
    Add the metadata property of an otio object (serialized to json) to an
    rv node component as an otio_metadata property.

    Parameters
        ----------
        node_component : str
            The RV node and component name to add to
        otio : Object
            The otio object that contains the metadata
    """
    if otio_node.metadata:
        add_rv_effect_props(
            node_component,
            {
                "otio_metadata": otio.core.serialize_json_to_string(
                    otio_node.metadata, indent=-1
                )
            },
        )


def get_otio_metadata(node_component):
    """
    Get the metadata property of an otio object (deerialized from json) to an
    rv node component as an otio_metadata property.

    Parameters
        ----------
        node_component : str
            The RV node and component containing the metadata
    """
    metadata_prop = "{}.otio_metadata".format(node_component)
    if commands.propertyExists(metadata_prop):
        metadata_str = commands.getStringProperty(metadata_prop)[0]
        return otio.core.deserialize_json_from_string(metadata_str)
    return {}


def _get_rv_value(value):
    if type(value) == bool:
        return [1] if value is True else [0]

    # OTIO returns AnyVector, so ensure anything iterable is a list
    return list(value) if hasattr(value, "__iter__") else [value]


def _set_rv_prop(prop_name, value):
    try:
        SET_NODE_TYPE_MAP[type(value[0])](prop_name, value, True)
    except KeyError:
        raise NoMappingForRvNode("No RV node found for type {}".format(type(value[0])))
