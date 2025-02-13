#
# Copyright Contributors to the OpenTimelineIO project
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

# This code has been taken from opentimelineio's exten_rv rv adapter
# and converted to work interactively in RV.
#
# TODO: We would like to move this back into opentimelineio's rv adapter
# such that we can use this both interactively as well as standalone
#

import logging
from rv import commands
from rv import extra_commands

import os

import opentimelineio as otio
from contextlib import contextmanager


@contextmanager
def set_context(context, **kwargs):
    if context:
        old_context = context.copy()
        context.update(**kwargs)

        try:
            yield
        finally:
            context.clear()
            context.update(old_context)


class NoMappingForOtioTypeError(otio.exceptions.OTIOError):
    pass


class NoNodeFromHook(otio.exceptions.OTIOError):
    pass


def read_otio_string(otio_string: str, host_prefix: str | None = None) -> object | None:
    """
    Main entry point to expand a given otio string into the current RV session.

    Returns the top level node created that represents this otio
    timeline.
    """
    otio_obj = otio.adapters.read_from_string(otio_string)
    timeline = otio_obj["otio"]

    context = {"sg_url": host_prefix} if host_prefix else None

    return create_rv_node_from_otio(timeline, context), timeline.global_start_time


def read_otio_file(otio_file):
    """
    Main entry point to expand a given otio (or file otio can read)
    into the current RV session.

    Returns the top level node created that represents this otio
    timeline.
    """
    input_otio = otio.adapters.read_from_file(otio_file)
    context = {"otio_file": otio_file}
    return create_rv_node_from_otio(input_otio, context)


def _run_hook(hook_name, otio_obj, context={}, optional=True):
    try:
        return otio.hooks.run(hook_name, otio_obj, context)
    except KeyError:
        if not optional:
            raise NoMappingForOtioTypeError(
                str(type(otio_obj)) + " on object: {}".format(otio_obj)
            )


def create_rv_node_from_otio(otio_obj, context=None):
    WRITE_TYPE_MAP = {
        otio.schema.Timeline: _create_timeline,
        otio.schema.Stack: _create_stack,
        otio.schema.Track: _create_track,
        otio.schema.Clip: _create_item,
        otio.schema.Gap: _create_item,
        otio.schema.Transition: _create_transition,
        otio.schema.SerializableCollection: _create_collection,
    }

    if type(otio_obj) in WRITE_TYPE_MAP:
        process_schema = _run_hook(
            "pre_hook_{}".format(otio_obj.schema_name()), otio_obj, context
        )
        if process_schema is False:
            return None

        result = WRITE_TYPE_MAP[type(otio_obj)](otio_obj, context)
        with set_context(context, post_schema=result):
            _run_hook("post_hook_{}".format(otio_obj.schema_name()), otio_obj, context)
        return result

    return _run_hook(
        "{}_to_rv".format(otio_obj.schema_name()), otio_obj, context, optional=False
    )


def _retime_trx_input(pre_item, post_item, context=None):
    post_item_rv = create_rv_node_from_otio(post_item, context)

    node_to_insert = post_item_rv

    if (
        hasattr(pre_item, "media_reference")
        and pre_item.media_reference
        and pre_item.media_reference.available_range
        and hasattr(post_item, "media_reference")
        and post_item.media_reference
        and post_item.media_reference.available_range
        and (
            post_item.media_reference.available_range.start_time.rate
            != pre_item.media_reference.available_range.start_time.rate
        )
    ):
        # write a retime to make sure post_item is in the timebase of pre_item
        rt_node = commands.newNode("Retime", "transition_retime")
        rt_node.setTargetFps(pre_item.media_reference.available_range.start_time.rate)

        post_item_rv = create_rv_node_from_otio(post_item, context)

        rt_node.addInput(post_item_rv)
        node_to_insert = rt_node

    return node_to_insert


def _create_dissolve(in_dissolve, context=None):
    return commands.newNode("CrossDissolve", in_dissolve.name or "dissolve")


def _create_custom(in_transition, context=None):
    rv_trx = _run_hook("CustomTransition_to_rv", in_transition, context)
    if not rv_trx:
        raise NoNodeFromHook(
            "Custom Transition found but no node was returned from the hook"
        )
    return rv_trx


def _create_transition(pre_item, in_trx, post_item, context=None):
    trx_map = {
        otio.schema.TransitionTypes.SMPTE_Dissolve: _create_dissolve,
        otio.schema.TransitionTypes.Custom: _create_custom,
    }

    if in_trx.transition_type not in trx_map:
        return

    rv_trx = trx_map[in_trx.transition_type](in_trx, context)

    extra_commands.setUIName(rv_trx, str(in_trx.name or "custom_transition"))

    commands.setFloatProperty(rv_trx + ".parameters.startFrame", [1.0], True)

    num_frames = (
        (in_trx.in_offset + in_trx.out_offset)
        .rescaled_to(pre_item.trimmed_range().duration.rate)
        .value
    )

    in_offset_prop = "{}.otio.in_offset".format(rv_trx)
    commands.newProperty(in_offset_prop, commands.IntType, 1)
    commands.setIntProperty(in_offset_prop, [in_trx.in_offset.to_frames()], True)

    commands.setFloatProperty(
        rv_trx + ".parameters.numFrames", [float(num_frames)], True
    )

    commands.setFloatProperty(
        rv_trx + ".output.fps", [float(pre_item.trimmed_range().duration.rate)], True
    )

    trx_inputs = []
    with set_context(context, transition=rv_trx):
        pre_item_rv = create_rv_node_from_otio(pre_item, context)
        trx_inputs.append(pre_item_rv)
        node_to_insert = _retime_trx_input(pre_item_rv, post_item, context)
    trx_inputs.append(node_to_insert)
    commands.setNodeInputs(rv_trx, trx_inputs)
    _add_metadata_to_node(in_trx, rv_trx)

    return rv_trx


def _create_stack(in_stack, context=None):
    new_stack = commands.newNode("RVStackGroup", in_stack.name or "tracks")
    extra_commands.setUIName(new_stack, str(in_stack.name or "tracks"))

    with set_context(context, stack=new_stack):
        new_inputs = []
        for seq in in_stack:
            result = create_rv_node_from_otio(seq, context)
            if result:
                new_inputs.append(result)

    commands.setNodeInputs(new_stack, new_inputs)

    _add_markers(in_stack, new_stack)
    _add_metadata_to_node(in_stack, new_stack)

    # disable reversed order blending so the top node is on top
    stack_node = extra_commands.nodesInGroupOfType(new_stack, "RVStack")[0]
    commands.setIntProperty(
        "{}.mode.supportReversedOrderBlending".format(stack_node), [0]
    )

    return new_stack


def _create_track(in_seq, context=None):
    context = context or {}

    new_seq = commands.newNode("RVSequenceGroup", str(in_seq.name or "track"))
    extra_commands.setUIName(new_seq, str(in_seq.name or "track"))

    items_to_serialize = otio.algorithms.track_with_expanded_transitions(in_seq)

    with set_context(context, sequence=new_seq, track_kind=in_seq.kind):
        new_inputs = []

        edl_time = context.get("global_start_time")
        if not edl_time and len(items_to_serialize) > 0:
            edl_time = otio.opentime.RationalTime(
                0, items_to_serialize[0].trimmed_range().start_time.rate
            )
        edl = {
            "in": [],
            "out": [],
            "frame": [],
        }

        for thing in items_to_serialize:
            if isinstance(thing, tuple):
                result = _create_transition(*thing, context=context)
                edl_item, pre_item = thing[1], thing[0]

            elif thing.duration().value == 0:
                continue
            else:
                result = create_rv_node_from_otio(thing, context)
                edl_item, pre_item = thing, None

            if result:
                new_inputs.append(result)

                edl_range = _calculate_edl(edl, edl_time, edl_item, pre_item)
                edl_time = edl_range.end_time_exclusive()

        commands.setNodeInputs(new_seq, new_inputs)

        seq_node = extra_commands.nodesInGroupOfType(new_seq, "RVSequence")[0]
        _set_sequence_edl(seq_node, edl_time, edl)
        _add_markers(in_seq, new_seq)
        _add_metadata_to_node(in_seq, new_seq)

        # disable reversed order blending so the top node is on top
        commands.setIntProperty(
            "{}.mode.supportReversedOrderBlending".format(seq_node), [0]
        )

    return new_seq


def _get_global_transform(tl) -> dict:
    # since there's no global scale in otio, calculate the minimum box size
    # that can contain all clips
    def find_display_bounds(tl):
        display_bounds = None
        for clip in tl.find_clips():
            try:
                bounds = clip.media_reference.available_image_bounds
                if bounds:
                    if display_bounds:
                        display_bounds.extendBy(bounds)
                    else:
                        display_bounds = bounds
            except AttributeError:
                continue
        return display_bounds

    bounds = find_display_bounds(tl)
    if bounds is None:
        return {}

    translate = bounds.center()
    scale = bounds.max - bounds.min

    # RV's global coordinate system has a width and height of 1 where the
    # width will be scaled to the image aspect ratio.  So scale globally by
    # height. The source width will later be scaled to aspect ratio.
    global_scale = otio.schema.V2d(1.0 / scale.y, 1.0 / scale.y)

    return {
        "global_scale": global_scale,
        "global_translate": translate * global_scale,
    }


def _calculate_edl(edl, edl_time, item, pre_transition_item=None):
    # EDL values don't make much sense for a transitions, since it has two
    # different sources as inputs. So if we have a transition, we'll just set
    # EDL values to consume the whole transition, and rely on the cut values
    # being set on each input source.
    if pre_transition_item:
        in_frame = 1
        rate = pre_transition_item.trimmed_range().duration.rate
        duration = (item.in_offset + item.out_offset).rescaled_to(rate)
        out_frame = otio.opentime.to_frames(duration, rate)
    else:
        in_frame, out_frame = _get_in_out_frame(item, item.trimmed_range())
        duration = item.trimmed_range().duration

    edl["in"].append(in_frame)
    edl["out"].append(out_frame)
    edl["frame"].append(edl_time.to_frames())

    return otio.opentime.TimeRange(edl_time, duration)


def _set_sequence_edl(sequence, edl_time, edl):
    # edl.in/out are terminated by 0. edl.frame is terminated by last frame + 1.
    edl["in"].append(0)
    edl["out"].append(0)
    edl["frame"].append(edl_time.to_frames() + 1)

    # This effectively forces each cut to use the otio trimmed_range, regardless
    # of effects or other modifications to a sources timing.
    commands.setIntProperty("{}.edl.in".format(sequence), edl["in"])
    commands.setIntProperty("{}.edl.out".format(sequence), edl["out"])
    commands.setIntProperty("{}.edl.frame".format(sequence), edl["frame"])
    commands.setIntProperty(
        "{}.edl.source".format(sequence), list(range(len(edl["frame"])))
    )
    commands.setIntProperty("{}.mode.autoEDL".format(sequence), [0])


def _get_in_out_frame(it, range_to_read):
    in_frame = out_frame = None

    if hasattr(it, "media_reference") and it.media_reference:
        if (
            isinstance(it.media_reference, otio.schema.ImageSequenceReference)
            and it.media_reference.available_range
        ):
            in_frame, out_frame = it.media_reference.frame_range_for_time_range(
                range_to_read
            )

    if not in_frame and not out_frame:
        # because OTIO has no global concept of FPS, the rate of the duration
        # is used as the rate for the range of the source.
        in_frame = otio.opentime.to_frames(
            range_to_read.start_time, rate=range_to_read.duration.rate
        )
        out_frame = otio.opentime.to_frames(
            range_to_read.end_time_inclusive(), rate=range_to_read.duration.rate
        )
    return (in_frame, out_frame)


def _create_timeline(tl, context=None):
    with set_context(
        context, global_start_time=tl.global_start_time, **_get_global_transform(tl)
    ):
        stack = create_rv_node_from_otio(tl.tracks, context)

        # since we don't have a timeline node in RV, add the metadata to the
        # stack so we can round trip it
        _add_metadata_to_node(tl, stack, "timeline_metadata")

        # also add the timeline name if it has one
        if tl.name:
            name_prop = "{}.otio.timeline_name".format(stack)

            commands.newProperty(name_prop, commands.StringType, 1)
            commands.setStringProperty(name_prop, [tl.name], True)

        return stack


def _create_collection(collection, context=None):
    results = []
    for item in collection:
        result = create_rv_node_from_otio(item, context)
        if result:
            results.append(result)

    if results:
        return results[0]


def _create_media(media_ref, trimmed_range, context=None):
    context = context or {}
    media_range = media_ref.available_range or trimmed_range

    if isinstance(media_ref, otio.schema.ExternalReference):
        media = [_get_media_path(str(media_ref.target_url), context)]

        if context.get("track_kind", None) == otio.schema.TrackKind.Audio:
            # Create blank video media to accompany audio for valid source
            blank = _create_movieproc(media_range)
            # Appending blank to media promotes name of audio file in RV
            media.append(blank)
        return media

    elif isinstance(media_ref, otio.schema.ImageSequenceReference):
        return [
            _get_media_path(
                str(
                    media_ref.abstract_target_url(
                        symbol="%0{n}d".format(n=media_ref.frame_zero_padding)
                    )
                ),
                context,
            )
        ]
    return [_create_movieproc(media_range, "smptebars")]


def _create_sources(item, context=None):
    def add_media(media_ref, active_key, cmd, *cmd_args):
        media = _create_media(media_ref, item.trimmed_range(), context)

        if active_key:
            media += ["+mediaRepName", active_key]

        try:
            new_cmd_args = cmd_args + (media,)
            src = cmd(*new_cmd_args)
        except Exception as e:
            print("ERROR adding media (replacing with smptebars): {}".format(e))
            error_media = _create_movieproc(item.trimmed_range(), "smptebars")
            src = commands.addSourceVerbose([error_media])

        commands.setFloatProperty(
            src + ".group.fps", [item.trimmed_range().duration.rate]
        )

        _add_metadata_to_node(media_ref, src)
        active_src = cmd_args[0] if cmd_args else src
        _add_source_bounds(media_ref, src, active_src, context)
        extra_commands.setUIName(src, str(media_ref.name or "media"))

        return src

    if isinstance(item, otio.schema.Gap):
        src = commands.addSourceVerbose(
            [_create_movieproc(item.trimmed_range(), "blank")]
        )
        return commands.nodeGroup(src), src

    active_source = None
    active_key = (
        item.active_media_reference_key
        if hasattr(item, "active_media_reference_key")
        else None
    )
    if hasattr(item, "media_reference") and item.media_reference:
        active_source = add_media(
            item.media_reference, active_key, commands.addSourceVerbose
        )

    source_group = commands.nodeGroup(active_source)
    if active_source and hasattr(item, "media_references"):
        for key, media_ref in item.media_references().items():
            if key != active_key:
                add_media(
                    media_ref, None, commands.addSourceMediaRep, active_source, key
                )

        switch_group = commands.nodeConnections(source_group)[1][0]
        switch = extra_commands.nodesInGroupOfType(switch_group, "RVSwitch")[0]
        commands.setIntProperty("{}.mode.autoEDL".format(switch), [0])

        return switch_group, active_source

    return source_group, active_source


def _get_media_path(target_url: str, context: dict | None = None) -> str:
    context = context or {}

    if "sg_url" in context:
        return context.get("sg_url") + target_url

    if not os.path.isabs(target_url):
        # if this is a relative file path, assume relative to the otio file
        otio_path = context.get("otio_file")
        if otio_path:
            relative_path = os.path.join(os.path.dirname(otio_path), target_url)
            if os.path.exists(relative_path):
                return relative_path
    return target_url


def _add_transition_timings(clip, source_range, node):
    # If the item is part of a transition, we cannot rely on EDL values
    # since the transition is a single input to the sequence and the
    # sources have different in/out points, so we'll use cut values on
    # the sources directly if this is the case.
    frames = _get_in_out_frame(clip, source_range)

    if commands.nodeType(node) == "RVSwitchGroup":
        for src_group in commands.nodeConnections(node)[0]:
            src = extra_commands.nodesInGroupOfType(src_group, "RVSource")[0]
            commands.setIntProperty(src + ".cut.in", [frames[0]])
            commands.setIntProperty(src + ".cut.out", [frames[1]])
        return

    src = extra_commands.nodesInGroupOfType(node, "RVSource")[0]
    commands.setIntProperty(src + ".cut.in", [frames[0]])
    commands.setIntProperty(src + ".cut.out", [frames[1]])


def _create_item(it, context=None):
    context = context or {}
    range_to_read = it.trimmed_range()

    if not range_to_read:
        raise otio.exceptions.OTIOError("No valid range on clip: {0}.".format(str(it)))

    src_or_switch_group, active_src = _create_sources(it, context)
    _add_metadata_to_node(it, src_or_switch_group)
    extra_commands.setUIName(src_or_switch_group, str(it.name or "clip"))

    with set_context(
        context,
        src=active_src,
        source_group=commands.nodeGroup(active_src),
        switch_group=(
            src_or_switch_group
            if commands.nodeType(src_or_switch_group) == "RVSwitchGroup"
            else None
        ),
    ):
        if context.get("transition"):
            _add_transition_timings(it, range_to_read, src_or_switch_group)

        _add_markers(it, src_or_switch_group)
        return _add_effects(it, src_or_switch_group, context)


def _create_movieproc(time_range, kind="blank"):
    movieproc = "{},start={},end={},fps={}.movieproc".format(
        kind,
        time_range.start_time.value,
        time_range.end_time_inclusive().value,
        time_range.duration.rate,
    )
    return movieproc


def _add_effects(it, last_result, context=None):
    for effect in it.effects:
        result = create_rv_node_from_otio(effect, context)

        if result:
            commands.setNodeInputs(result, [last_result])
            _add_metadata_to_node(effect, result)
            last_result = result

    return last_result


def _add_markers(it, node):
    frames, colors, names, metadata = [], [], [], []
    for marker in it.markers:
        names.append(marker.name)
        frames.append(_get_in_out_frame(it, marker.marked_range))
        colors.append(_get_color_from_name(marker.color))
        metadata.append(otio.core.serialize_json_to_string(marker.metadata, indent=-1))

    if frames:
        ins, outs = map(list, zip(*frames))
        commands.setStringProperty("{}.markers.name".format(node), names, True)
        commands.setIntProperty("{}.markers.in".format(node), ins, True)
        commands.setIntProperty("{}.markers.out".format(node), outs, True)
        commands.setFloatProperty(
            "{}.markers.color".format(node),
            [color for rgba in colors for color in rgba],
            True,
        )
        commands.newProperty(
            "{}.markers.otio_metadata".format(node), commands.StringType, 1
        )
        commands.setStringProperty(
            "{}.markers.otio_metadata".format(node), metadata, True
        )


def _get_color_from_name(name, defaultColor="PURPLE"):
    # Color values from: https://www.w3schools.com/colors/colors_names.asp
    # The reverse mapping is in otio_writer
    colors = {
        "PINK": (1.0, 0.753, 0.796, 1.0),
        "RED": (1.0, 0.0, 0.0, 1.0),
        "ORANGE": (1.0, 0.647, 0.0, 1.0),
        "YELLOW": (1.0, 1.0, 0.0, 1.0),
        "GREEN": (0.0, 0.5, 0.0, 1.0),
        "CYAN": (0.0, 1.0, 1.0, 1.0),
        "BLUE": (0.0, 0.0, 1.0, 1.0),
        "PURPLE": (0.5, 0.0, 0.5, 1.0),
        "MAGENTA": (1.0, 0.0, 1.0, 1.0),
        "BLACK": (0.0, 0.0, 0.0, 1.0),
        "WHITE": (1.0, 1.0, 1.0, 1.0),
    }
    return colors.get(name, colors[defaultColor])


def _add_source_bounds(media_ref, src, active_src, context=None):
    # for releases < 0.15
    if not hasattr(media_ref, "available_image_bounds"):
        return

    context = context or {}

    bounds = media_ref.available_image_bounds
    if not bounds:
        return

    global_scale = context.get("global_scale")
    global_translate = context.get("global_translate")
    if global_scale is None or global_translate is None:
        return

    # A width of 1.0 in RV means draw to the aspect ratio, so scale the
    # width by the inverse of the aspect ratio
    #
    try:
        # Note: When multiple media representation sources are created, we can only retrieve the media_info of a source if it is the active source
        # That's because multiple media representation in RV is optimized to only load a source when it gets activated.
        # If the current source is not the active source, we can't get the media_info so we fall back to the active source's media_info as our best guess.
        media_info = commands.sourceMediaInfo(active_src)
        height = media_info["height"]
        aspect_ratio = media_info["width"] / height
    except Exception:
        logging.exception(
            "Unable to determine aspect ratio, using default value of 16:9"
        )
        aspect_ratio = 1920 / 1080

    translate = bounds.center() * global_scale - global_translate
    scale = (bounds.max - bounds.min) * global_scale

    transform_node = extra_commands.associatedNode("RVTransform2D", src)

    commands.setFloatProperty(
        "{}.transform.scale".format(transform_node), [scale.x / aspect_ratio, scale.y]
    )
    commands.setFloatProperty(
        "{}.transform.translate".format(transform_node), [translate.x, translate.y]
    )

    # write the bounds global_scale and global_translate to the node so we can
    # preserve the original values if we round-trip
    commands.newProperty(
        "{}.otio.global_scale".format(transform_node), commands.FloatType, 2
    )
    commands.newProperty(
        "{}.otio.global_translate".format(transform_node), commands.FloatType, 2
    )
    commands.setFloatProperty(
        "{}.otio.global_scale".format(transform_node),
        [global_scale.x, global_scale.y],
        True,
    )
    commands.setFloatProperty(
        "{}.otio.global_translate".format(transform_node),
        [global_translate.x, global_translate.y],
        True,
    )


def _add_metadata_to_node(item, rv_node, prop_name="metadata"):
    """
    Add metadata from otio "item" to rv_node
    """
    if item.metadata:
        otio_metadata_property = "{}.otio.{}".format(rv_node, prop_name)
        otio_metadata = otio.core.serialize_json_to_string(item.metadata, indent=-1)
        commands.newProperty(otio_metadata_property, commands.StringType, 1)
        commands.setStringProperty(otio_metadata_property, [otio_metadata], True)
