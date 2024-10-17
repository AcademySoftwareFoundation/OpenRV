#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import math
import numbers
import six
import os
import re

import opentimelineio as otio

from rv import commands
from rv import extra_commands
from rv import runtime


class NoMappingForNodeTypeError(otio.exceptions.OTIOError):
    pass


class UnhandledEffectInput(otio.exceptions.OTIOError):
    pass


# RV min-int, max-int
MIN_INT = int(runtime.eval("int.min;", []))
MAX_INT = int(runtime.eval("int.max;", []))

DEFAULT_FPS = 24.0

# The reverse color mapping is in otio_reader
PINK = [1.0, 0.753, 0.796, 1.0]
RED = [1.0, 0.0, 0.0, 1.0]
ORANGE = [1.0, 0.647, 0.0, 1.0]
YELLOW = [1.0, 1.0, 0.0, 1.0]
GREEN = [0.0, 0.5, 0.0, 1.0]
CYAN = [0.0, 1.0, 1.0, 1.0]
BLUE = [0.0, 0.0, 1.0, 1.0]
PURPLE = [0.5, 0.0, 0.5, 1.0]
MAGENTA = [1.0, 0.0, 1.0, 1.0]
BLACK = [0.0, 0.0, 0.0, 1.0]
WHITE = [1.0, 1.0, 1.0, 1.0]

TRANSITION_TYPE_MAP = {
    "CrossDissolve": otio.schema.TransitionTypes.SMPTE_Dissolve,
    "Wipe": otio.schema.TransitionTypes.Custom,
}


def write_otio_file(root_node_name, file_path):
    """
    Create an OTIO Timeline starting from the supplied RV node and write it
    to the path pointed to file
    :param root_node_name: `str`
    :param file_path: `str`
    """

    timeline = otio.schema.Timeline()

    otio_root = create_otio_from_rv_node(root_node_name, timeline=timeline)
    if not otio_root:
        return

    if commands.nodeType(root_node_name) == "RVStackGroup":
        # check if the OTIO import saved any timeline properties to the stack
        timeline.metadata.update(
            get_node_otio_metadata(root_node_name, "timeline_metadata")
        )
        name_prop = "{}.otio.timeline_name".format(root_node_name)
        if commands.propertyExists(name_prop):
            timeline.name = commands.getStringProperty(name_prop)[0]

        timeline.tracks = otio_root
    else:
        timeline.tracks[:] = [otio_root]

    otio.adapters.write_to_file(timeline, file_path)


def _run_hook(hook_name, optional=True, *args, **kwargs):
    try:
        return otio.hooks.run(hook_name, kwargs.get("timeline"), kwargs)
    except KeyError:
        if not optional:
            raise NoMappingForNodeTypeError("No {} hook found".format(hook_name))


def create_otio_from_rv_node(node_name, *args, **kwargs):
    """
    Create an OTIO Timeline instance from a given RV node.
    :param node_name: `str`
    :return: `otio.schema.*`
    """
    create_type_map = {
        "RVSequenceGroup": _create_track,
        "RVStackGroup": _create_stack,
        "RVSourceGroup": _create_item,
        "RVSwitchGroup": _create_item,
        "CrossDissolve": _create_transition,
        "Wipe": _create_transition,
    }

    node_type = commands.nodeType(node_name)
    if node_type in create_type_map:
        process_node = _run_hook(
            "pre_export_hook_{}".format(node_type),
            rv_node_name=node_name,
            optional=True,
            *args,
            **kwargs
        )
        if process_node is False:
            return None

        result = create_type_map[node_type](node_name, *args, **kwargs)
        _run_hook(
            "post_export_hook_{}".format(node_type),
            rv_node_name=node_name,
            post_node=result,
            optional=True,
            *args,
            **kwargs
        )
    else:
        result = _run_hook(
            "export_{}".format(node_type),
            rv_node_name=node_name,
            optional=False,
            *args,
            **kwargs
        )

    if not isinstance(result, otio.schema.Effect):
        return result

    input_node_names, _ = commands.nodeConnections(node_name)
    num_effect_inputs = len(input_node_names)

    # Each effect should have one input (another effect or something that
    # results in an otio item).  If not, we don't know how to handle it
    if num_effect_inputs != 1:
        raise UnhandledEffectInput(
            "{} effect ({}) should have 1 input".format(node_type, node_name)
        )

    # Since we are iterate through inputs, we will find effects before we find
    # the items they belong to. So recurse, keeping track of the effects we've
    # found until we find a item and then add the effects to it.
    next_node = create_otio_from_rv_node(input_node_names[0], *args, **kwargs)

    if hasattr(next_node, "effects"):
        result.metadata.update(get_node_otio_metadata(node_name))
        next_node.effects.append(result)

    return next_node


def _create_track(node_name, *args, **kwargs):
    """
    Create an OTIO Track instance from an RVSequenceGroup.
    :param node_name: `str`
    :return: `otio.schema.Track`
    """
    seq_node = group_member_of_type(node_name, "RVSequence")
    fps = commands.getFloatProperty(seq_node + ".output.fps")[0]

    track = otio.schema.Track(extra_commands.uiName(node_name))
    track.markers.extend(_create_markers(node_name, fps))

    input_node_names, _ = commands.nodeConnections(node_name)

    # Set timing for Sequence elements
    edl = {
        "in_frame": commands.getIntProperty(seq_node + ".edl.in"),
        "out_frame": commands.getIntProperty(seq_node + ".edl.out"),
        "cut_in_frame": commands.getIntProperty(seq_node + ".edl.frame"),
        "source": commands.getIntProperty(seq_node + ".edl.source"),
    }

    has_edl = edl["in_frame"] and edl["out_frame"] and edl["source"]

    transition = None
    for edl_index, rv_node in enumerate(
        [input_node_names[i] for i in edl["source"][:-1]]
        if has_edl
        else input_node_names
    ):
        # We need the item after the transition to correctly process it,
        # so delay this until the next input
        if TRANSITION_TYPE_MAP.get(commands.nodeType(rv_node)):
            transition = rv_node
            continue

        edl_in = edl["in_frame"][edl_index] if has_edl else None
        edl_out = edl["out_frame"][edl_index] if has_edl else None
        cut_in_frame = edl["cut_in_frame"][edl_index] if has_edl else None

        kwargs["in_frame"] = edl_in
        kwargs["out_frame"] = edl_out
        kwargs["cut_in_frame"] = cut_in_frame
         
        item = create_otio_from_rv_node(rv_node, *args, **kwargs)

        if has_edl:
            timeline = kwargs.get("timeline")
            # set the global start time to the EDL start of the first input
            if edl_index == 0 and timeline:
                timeline.global_start_time = otio.opentime.RationalTime(edl_in, fps)

            # If we couldn't create an item, add a gap to preserve the
            # EDL times in the subsequent OTIO items
            if not item:
                item = otio.schema.Gap(
                    source_range=frames_to_time_range(edl_in, edl_out, fps)
                )

        # Now that we have the items surrounding the transition, create it
        if transition:
            kwargs["pre_item"] = track[-1] if len(track) > 0 else None,
            kwargs["post_item"] = item

            transition_node = create_otio_from_rv_node(transition, *args, **kwargs)

            if transition_node:
                track.append(transition_node)

            transition = None

        track.append(item)
    return track


def _create_stack(node_name, *args, **kwargs):
    """
    Create an OTIO Stack instance from an RVStackGroup.
    :param node_name: `str`
    :return: `otio.schema.Stack`
    """
    stack_node = group_member_of_type(node_name, "RVStack")
    fps = commands.getFloatProperty(stack_node + ".output.fps")[0]

    input_node_names, _ = commands.nodeConnections(node_name)

    stack = otio.schema.Stack(
        extra_commands.uiName(node_name), metadata=get_node_otio_metadata(node_name)
    )
    stack.markers.extend(_create_markers(node_name, fps))

    for input_node_name in input_node_names:
        stack.append(create_otio_from_rv_node(input_node_name, *args, **kwargs))

    return stack


def _create_item(node_name, *args, **kwargs):
    """
    Create an OTIO Clip or Gap for an RVSourceGroup.
    :param node_name: `str`
    :param in_frame: `int`
    :param out_frame: `int`
    :param cut_in_frame: `int`
    :return: `otio.schema.Clip` or `otio.schema.Gap`
    """

    in_frame = kwargs.get("in_frame")
    out_frame = kwargs.get("out_frame")
    cut_in_frame = kwargs.get("cut_in_frame")

    active_source = get_source_node(node_name)
    active_source_group = node_name
    media_references = {}

    if commands.nodeType(node_name) == "RVSwitchGroup":
        # handle multi-media reppresentation
        active_key = None

        for src_group in commands.nodeConnections(node_name)[0]:
            source = get_source_node(src_group)
            key = commands.getStringProperty("{}.media.repName".format(source))[0]

            if commands.getIntProperty("{}.media.active".format(source))[0] == 1:
                active_key = key
                active_source = source
                active_source_group = src_group

            media_references[key] = _create_media_reference(src_group, source)

    fps = get_source_fps(active_source_group)

    # Create TimeRange
    start_time, duration = frames_to_rational_times(
        start_frame=in_frame
        if in_frame is not None
        else get_source_start_frame(active_source_group),
        end_frame=out_frame
        if out_frame is not None
        else get_source_end_frame(active_source_group),
        fps=fps,
    )
    if start_time or duration:
        source_range = otio.opentime.TimeRange(start_time=start_time, duration=duration)
    else:
        duration = None
        source_range = None

    # special case: blank source is a gap (so no media reference)
    source_path = get_source_path(active_source_group)
    if source_path.startswith("blank,") and source_path.endswith(".movieproc"):
        gap = otio.schema.Gap(
            name=extra_commands.uiName(node_name),
            source_range=source_range,
            metadata=get_node_otio_metadata(node_name),
        )
        gap.markers.extend(_create_markers(node_name, fps))
        return gap

    item = otio.schema.Clip(
        name=extra_commands.uiName(node_name),
        source_range=source_range,
        metadata=get_node_otio_metadata(node_name) or {},
    )

    if media_references:
        if hasattr(otio.schema.Clip, "media_references"):
            item.set_media_references(media_references, active_key)
        else:
            item.media_reference = media_references[active_key]
    else:
        item.media_reference = _create_media_reference(node_name, active_source)

    item.markers.extend(_create_markers(node_name, fps))
    return item


def _create_media_reference(node_name, source_node):

    source_path = get_source_path(node_name)
    source_name = get_source_name(node_name)

    is_movieproc = source_path.endswith(".movieproc")
    source_basename = os.path.basename(source_path)

    frame_zero_padding = None
    image_seq_pattern = re.findall("\.%0\d+d\.", source_basename)
    if image_seq_pattern:
        frame_zero_padding = int(re.search("\d+", image_seq_pattern[0]).group(0))
    else:
        image_seq_pattern = re.findall("\.\d+-\d+#|@+", source_basename)
        if image_seq_pattern:
            pattern = re.search("#|@+", image_seq_pattern[0]).group(0)
            frame_zero_padding = 4 if "#" in pattern else len(pattern)

    if is_movieproc:
        if source_path.startswith("smptebars,"):
            media_reference = otio.schema.GeneratorReference(
                generator_kind="SMPTEBars", metadata=get_node_otio_metadata(source_node)
            )
        else:
            media_reference = otio.schema.MissingReference(
                metadata=get_node_otio_metadata(source_node)
            )
    elif len(image_seq_pattern) == 1:
        media_reference = otio.schema.ImageSequenceReference(
            os.path.dirname(source_path),
            source_basename.split(".")[0] + ".",
            os.path.splitext(source_basename)[1],
            frame_zero_padding=frame_zero_padding,
            metadata=get_node_otio_metadata(source_node),
        )
    else:
        media_reference = otio.schema.ExternalReference(
            target_url=source_path, metadata=get_node_otio_metadata(source_node)
        )

    first_frame = get_movie_first_frame(node_name)
    if first_frame is not None:
        end_frame = get_movie_last_frame(node_name)
        if end_frame is not None:
            fps = get_movie_fps(node_name)
            if fps is not None:
                media_reference.available_range = frames_to_time_range(
                    start_frame=get_movie_first_frame(node_name),
                    end_frame=get_movie_last_frame(node_name),
                    fps=get_movie_fps(node_name),
                )

    media_reference.name = source_name
    return media_reference


def _create_transition(rv_trx, *args, **kwargs):
    """
    Create an OTIO Transition for a CrossDissolve node.
    :param node_name: `str`
    :return: `otio.schema.Transition`
    """
    pre_item = kwargs.get("pre_item")[0]
    post_item = kwargs.get("post_item")

    transition_type = TRANSITION_TYPE_MAP.get(
        commands.nodeType(rv_trx), otio.schema.TransitionTypes.Custom
    )

    trx_inputs, _ = commands.nodeConnections(rv_trx)
    if len(trx_inputs) != 2:
        return None

    # if inputs are effect nodes, try to find their source
    out_source = get_input_node(trx_inputs[0], "RVSourceGroup")
    in_source = get_input_node(trx_inputs[1], "RVSourceGroup")

    if not out_source or not in_source:
        return None

    # Assume FPS matches first input (RV Transition don't have FPS settings)
    fps = get_source_fps(out_source)

    duration_frames_prop = commands.getFloatProperty(rv_trx + ".parameters.numFrames")
    duration_frames = duration_frames_prop[0] if duration_frames_prop else 20

    # If we imported from OTIO, respect the original in_offset.  If not,
    # assume the transition frames are split evenly between the two clips
    in_offset_prop = "{}.otio.in_offset".format(rv_trx)

    if commands.propertyExists(in_offset_prop):
        in_offset_frames = commands.getIntProperty(in_offset_prop)[0]
    else:
        in_offset_frames = int(duration_frames / 2)

    in_offset = otio.opentime.RationalTime(
        in_offset_frames,
        rate=fps,
    )
    out_offset = otio.opentime.RationalTime(
        duration_frames - in_offset.to_frames(),
        rate=fps,
    )

    if pre_item.source_range is not None:
        if not is_same_media(out_source, pre_item):
            return None
        pre_item.source_range = pre_item.source_range.duration_extended_by(in_offset)

    if post_item.source_range is not None:
        if not is_same_media(in_source, post_item):
            return None
        post_item.source_range = otio.opentime.TimeRange(
            start_time=post_item.source_range.start_time - out_offset,
            duration=post_item.source_range.duration + out_offset,
        )

    return otio.schema.Transition(
        name=extra_commands.uiName(rv_trx),
        transition_type=transition_type,
        in_offset=in_offset,
        out_offset=out_offset,
        metadata=get_node_otio_metadata(rv_trx),
    )


def _create_markers(node_name, fps):
    """
    Create an OTIO Markers for RV Markers.
    :param node_name: `str`
    :param fps: `float`
    :return: `[otio.schema.Marker]`
    """

    def get_marker_color(rgba, defaultColor=otio.schema.MarkerColor.PURPLE):

        if is_equal(rgba, PINK):
            return otio.schema.MarkerColor.PINK
        if is_equal(rgba, RED):
            return otio.schema.MarkerColor.RED
        if is_equal(rgba, ORANGE):
            return otio.schema.MarkerColor.ORANGE
        if is_equal(rgba, YELLOW):
            return otio.schema.MarkerColor.YELLOW
        if is_equal(rgba, GREEN):
            return otio.schema.MarkerColor.GREEN
        if is_equal(rgba, CYAN):
            return otio.schema.MarkerColor.CYAN
        if is_equal(rgba, BLUE):
            return otio.schema.MarkerColor.BLUE
        if is_equal(rgba, PURPLE):
            return otio.schema.MarkerColor.PURPLE
        if is_equal(rgba, MAGENTA):
            return otio.schema.MarkerColor.MAGENTA
        if is_equal(rgba, BLACK):
            return otio.schema.MarkerColor.BLACK
        if is_equal(rgba, WHITE):
            return otio.schema.MarkerColor.WHITE

        return defaultColor

    markers = []
    if not commands.propertyExists(node_name + ".markers.otio_metadata"):
        return markers

    colors = commands.getFloatProperty(node_name + ".markers.color")

    for name, marker_in, marker_out, metadata, color in zip(
        commands.getStringProperty(node_name + ".markers.name"),
        commands.getIntProperty(node_name + ".markers.in"),
        commands.getIntProperty(node_name + ".markers.out"),
        commands.getStringProperty(node_name + ".markers.otio_metadata"),
        [colors[x : x + 4] for x in range(0, len(colors), 4)],
    ):
        markers.append(
            otio.schema.Marker(
                name=name,
                marked_range=frames_to_time_range(marker_in, marker_out, fps),
                color=get_marker_color(color),
                metadata=otio.core.deserialize_json_from_string(metadata),
            )
        )
    return markers


def get_node_otio_metadata(node_name, prop_name="metadata"):
    """
    Retrieve the value of the OTIO Metadata property for a node, if it exists.
    This is set by otio_reader.py application plugin.
    :param node_name: `str`
    :return: `str`
    """
    # Standard OTIO Metadata prop from OTIO Reader
    otio_prop = "{}.otio.{}".format(node_name, prop_name)

    # Persist the general OTIO metadata separately to match OTIO Reader
    if commands.propertyExists(otio_prop):
        metadata_str = commands.getStringProperty(otio_prop)[0]
        return otio.core.deserialize_json_from_string(metadata_str)

    return {}


def is_equal(a, b, rel_tol=1e-05, abs_tol=0.0):
    """
    Equal if values (or all values in iterables) are within tolerance.
    rel_tol is a relatively high value to account for the difference in
    Python and C++ floating point values and we're dealing with primarily
    normalized values.
    """

    def compare(v1, v2, rel_t, abs_t):
        if not isinstance(v1, numbers.Number):
            return v1 == v2
        if six.PY2:
            return abs(v1 - v2) <= max(rel_t * max(abs(v1), abs(v2)), abs_t)
        return math.isclose(v1, v2, rel_tol=rel_t, abs_tol=abs_t)

    if hasattr(a, "__iter__") and hasattr(b, "__iter__"):
        for x, y in zip(a, b):
            if not compare(x, y, rel_tol, abs_tol):
                return False
        return True

    return compare(a, b, rel_tol, abs_tol)


def frames_to_rational_times(start_frame, end_frame, fps):
    """
    Convert the start and end frames and fps to OTIO Rational Times.
    :param start_frame: `int`
    :param end_frame: `int`
    :param fps: `float`
    :return: (`opentimelineio.opentime.RationalTime`, `opentimelineio.opentime.RationalTime`)
    """
    if start_frame is None or end_frame is None or not fps:
        return None, None

    start_frame = otio.opentime.RationalTime(start_frame, rate=fps)
    duration = otio.opentime.RationalTime.duration_from_start_end_time_inclusive(
        start_frame, otio.opentime.RationalTime(end_frame, rate=fps)
    )
    return (start_frame, duration)


def frames_to_time_range(start_frame, end_frame, fps):
    """
    Convert the start and end frames and fps to an OTIO Rational TimesRange.
    :param start_frame: `int`
    :param end_frame: `int`
    :param fps: `float`
    :return: `opentimelineio.opentime.TimeRange`
    """
    start_frame, duration = frames_to_rational_times(start_frame, end_frame, fps)
    if not start_frame or not duration:
        return None

    return otio.opentime.TimeRange(start_frame, duration)


def is_same_media(node_name, item):
    """
    Determine whether an RV node and an OTIO item point to the same media.
    :param node_name: `str`
    :param item: `otio.schema.Item`
    :return: `boolean`
    """
    if isinstance(item.media_reference, otio.schema.ExternalReference):
        return get_source_path(node_name) == item.media_reference.target_url
    elif isinstance(item.media_reference, otio.schema.ImageSequenceReference):
        return get_source_path(node_name) == item.media_reference_abstract_target_url(
            symbol="%0{n}d".format(n=item.media_reference.frame_zero_padding)
        )
    elif isinstance(item.media_reference, otio.schema.GeneratorReference):
        if get_source_path(node_name).startswith("smptebars,"):
            return item.media_reference.generator_kind == "SMPTEBars"

    return False


def get_source_node(node_name):
    """
    Get the source node name from the source node group name
    :param node_name: `str`
    :return: `str`
    """

    return group_member_of_type(node_name, "RVFileSource") or group_member_of_type(
        node_name, "RVImageSource"
    )


def get_source_path(node_name):
    """
    Get media movie prop for an RVSourceGroup.
    :param node_name: `str`
    :return: `str`
    """
    return commands.getStringProperty(get_source_node(node_name) + ".media.movie")[0]


def get_source_start_frame(node_name):
    """
    Get start frame prop for an RVSourceGroup.
    :param node_name: `str`
    :return: `int`
    """
    start_frame = commands.getIntProperty(get_source_node(node_name) + ".cut.in")[0]

    if start_frame == MIN_INT or start_frame == -MAX_INT:
        return None

    return start_frame


def get_source_end_frame(node_name):
    """
    Get end frame prop for an RVSourceGroup.
    :param node_name: `str`
    :return: `int`
    """
    end_frame = commands.getIntProperty(get_source_node(node_name) + ".cut.out")[0]

    return end_frame if end_frame != MAX_INT else None


def get_source_fps(node_name):
    """
    Get fps prop for an RVSourceGroup.
    :param node_name: `str`
    :return: `int`
    """
    fps = None
    file_source = group_member_of_type(node_name, "RVFileSource")
    if file_source:
        fps = commands.getFloatProperty(file_source + ".group.fps")[0]

    image_source = group_member_of_type(node_name, "RVImageSource")
    if image_source:
        fps = commands.getFloatProperty(image_source + ".image.fps")[0]

    if not fps:
        fps = get_movie_fps(node_name)

    # TODO Should we check for the Sequence FPS or global playback FPS?
    return fps or DEFAULT_FPS


def get_source_name(node_name):
    """
    Get UI name for an RVSource.
    :param node_name: `str`
    :return: `str`
    """
    source = group_member_of_type(node_name, "RVFileSource")
    if not source:
        source = group_member_of_type(node_name, "RVImageSource")

    if source and commands.propertyExists(source + ".ui.name"):
        return commands.getStringProperty(source + ".ui.name")[0]

    return ""


def get_movie_first_frame(node_name):
    """
    Get first frame from the Media Info
    :param node_name: `str`
    :return: `int`
    """
    try:
        return commands.sourceMediaInfo(get_source_node(node_name)).get("startFrame")
    except:
        return None


def get_movie_last_frame(node_name):
    """
    Get lsst frame from the Media Info
    :param node_name: `str`
    :return: `int`
    """
    try:
        return commands.sourceMediaInfo(get_source_node(node_name)).get("endFrame")
    except:
        return None


def get_movie_fps(node_name):
    """
    Get fps from the Media Info
    :param node_name: `str`
    :return: `int`
    """
    try:
        return commands.sourceMediaInfo(get_source_node(node_name)).get(
            "fps", DEFAULT_FPS
        )
    except:
        return None


def group_member_of_type(node, member_type):
    """
    Because it isn't RV code without group_member_of_type.
    :param node: `str`
    :param member_type: `str`
    :return: `str`
    """
    for n in commands.nodesInGroup(node):
        if commands.nodeType(n) == member_type:
            return n
    return None


def get_input_node(node, node_type):
    """
    Return the first input node with type node_type
    :param node: `str`
    :param node_type: `str`
    :return: `str`
    """
    if commands.nodeType(node) == node_type:
        return node

    inputs, _ = commands.nodeConnections(node)
    for i in inputs:
        input_node = get_input_node(i, node_type)
        if input_node:
            return input_node

    return None
