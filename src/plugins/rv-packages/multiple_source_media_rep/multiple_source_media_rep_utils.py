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

from collections import namedtuple


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


def get_switch_nodes_at_current_frame():
    """
    Retrieves the list of RVSwitch nodes at current frame.
    """
    switch_nodes = []

    sources = rvc.sourcesAtFrame(rvc.frame())
    for source in sources:
        src_switch_nodes = rve.nodesInEvalPath(rvc.frame(), "RVSwitch", source)
        if src_switch_nodes:
            switch_nodes.append(src_switch_nodes[0])

    return switch_nodes


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


def get_media_rep(switch_nodes):
    """
    Returns the common media representation for the specified nodes if any
    Note: Returns an empty string if no media representation exists.
    Note: Returns "Mixed" if there are different media representations.
    """
    rep = ""

    if switch_nodes:
        rep = rvc.sourceMediaRep(switch_nodes[0])
        for i in range(1, len(switch_nodes)):
            new_rep = rvc.sourceMediaRep(switch_nodes[i])
            if new_rep != rep:
                rep = "Mixed"
                break

    return rep


def get_media_rep_at_current_frame():
    """
    Returns the media representation at current frame.
    Note: Returns an empty string if no media representation exists.
    Note: Returns "Mixed" if there are different media representations
    at the current frame.
    """

    switch_nodes = get_switch_nodes_at_current_frame()
    return get_media_rep(switch_nodes)


def get_extension_from_info_file(info_file):
    """
    Returns the extension from the infos["file"] media info
    """
    return "" if not info_file else "URL" if is_url(info_file) else info_file.split(".")[-1].upper()


def get_common_source_media_infos(sources):
    """
    Returns the common infos shared by all the source nodes specified.
    Note that the following info are returned as a named tupple:
    resolution and extension.
    """
    resolution = ""
    extension = ""
    if sources:
        infos = get_source_media_info(sources[0])
        info_width = infos.get("width")
        info_height = infos.get("height")
        extension = get_extension_from_info_file(infos.get("file"))
        for i in range(1, len(sources)):
            new_infos = get_source_media_info(sources[i])
            new_width = new_infos.get("width")
            new_height = new_infos.get("height")

            # Skip gaps
            if new_width == 1 and new_height == 1:
                continue

            if new_width != info_width:
                info_width = ""
            if new_height != info_height:
                info_height = ""
            new_extension = get_extension_from_info_file(new_infos.get("file"))
            if new_extension != extension:
                extension = ""

        if info_width and info_height:
            resolution = "%s x %s" % (info_width, info_height)

    infos = namedtuple("infos", ["resolution", "extension"])
    return infos(resolution, extension)
