# Copyright (c) 2022 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.

from rv import commands as rvc
from rv import extra_commands as rve


def is_url(url):
    """
    Returns True if it is an URL.
    """
    return url.startswith("https://") or url.startswith("http://")


def get_source_at_current_frame():
    """
    Returns first source at current frame.
    """
    sources = rvc.sourcesAtFrame(rvc.frame())
    return "" if not sources else sources[0]


def get_switch_node(source):
    """
    Retrieves the first RVSwitch node at current frame.
    """

    if source:
        switch_nodes = rve.nodesInEvalPath(rvc.frame(), "RVSwitch", source)
        if switch_nodes:
            return switch_nodes[0]

    return ""


def get_media_reps_sources(switch_node):
    """
    Retrieves all source nodes of the RVSwitch node.
    """
    if switch_node:
        source_groups = rvc.nodeConnections(rvc.nodeGroup(switch_node))[0]

        nodes = []
        for group in source_groups:
            nodes += rve.nodesInGroupOfType(group, "RVFileSource")

        return nodes

    return []


def get_source_media_info(source):
    """
    Retrieves the media information of the given source.
    """
    info = {}
    try:
        if source:
            media = rvc.getStringProperty(source + ".media.movie")[0]
            info = rvc.sourceMediaInfo(source, media)
    finally:
        return info
