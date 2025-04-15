import effectHook
import opentimelineio as otio
from rv import commands, extra_commands


def hook_function(
    in_timeline: otio.schemadef.Compare.Compare, argument_map: dict | None = None
) -> None:
    """A hook for the compare schema"""
    compare_mode = in_timeline.mode
    is_swapped = in_timeline.is_swapped

    match (compare_mode.lower()):
        case "side_by_side":
            # Only the bounds need to be handled for this mode, and this is done in
            # the Live Review plugin, so nothing to do here
            pass

        case "over_with_opacity":
            opacity = float(in_timeline.over_with_opacity["opacity"])

            sequence_group = argument_map["sequence"]
            sequence_node = extra_commands.nodesInGroupOfType(
                sequence_group, "RVSequence"
            )[0]
            composite_property = f"{sequence_node}.composite"

            effectHook.add_rv_effect_props(
                composite_property,
                {
                    "inputBlendModes": "over",
                    "inputOpacities": 1 - opacity if is_swapped else opacity,
                },
            )

        case "difference":
            pass

        case "angular_mask":
            angle_in_radians = in_timeline.angular_mask["angle_in_radians"]
            pivot = in_timeline.angular_mask["pivot"]

            pivot_x = pivot.x
            pivot_y = pivot.y

            source = argument_map["src"]
            transform = extra_commands.associatedNode("RVTransform2D", source)
            if commands.getIntProperty(f"{transform}.transform.active")[0] != 0:
                global_translate_vec = otio.schema.V2d(0.0, 0.0)
                global_scale_vec = otio.schema.V2d(1.0, 1.0)

                if commands.propertyExists(
                    f"{transform}.otio.global_translate"
                ) and commands.propertyExists(f"{transform}.otio.global_scale"):
                    global_translate = commands.getFloatProperty(
                        f"{transform}.otio.global_translate"
                    )
                    global_scale = commands.getFloatProperty(
                        f"{transform}.otio.global_scale"
                    )
                    global_translate_vec = otio.schema.V2d(
                        global_translate[0], global_translate[1]
                    )
                    global_scale_vec = otio.schema.V2d(global_scale[0], global_scale[1])

                translate = commands.getFloatProperty(
                    f"{transform}.transform.translate",
                )
                scale = commands.getFloatProperty(
                    f"{transform}.transform.scale",
                )

                translate_vec = otio.schema.V2d(translate[0], translate[1])
                scale_vec = otio.schema.V2d(scale[0], scale[1])

                media_info = commands.sourceMediaInfo(source)
                height = media_info["height"]
                aspect_ratio = 1.0 if height == 0 else media_info["width"] / height

                bounds_size = scale_vec / global_scale_vec
                bounds_center = (
                    translate_vec + global_translate_vec
                ) / global_scale_vec

                # Since pivot coordinates are not local to the source,
                # we must remove the source transformation from them when
                # they are applied.
                pivot_x = (pivot_x - bounds_center.x) / bounds_size.x
                pivot_y = (pivot_y - bounds_center.y) / bounds_size.y

                # pivot is evaluated in the [0.0],[frameRatio,1.0] coord
                # system, but we want [0,0] to be in the center so we have to
                # offset our bounds values.
                pivot_x += 0.5 * aspect_ratio
                pivot_y += 0.5

            sequence_group = argument_map["sequence"]
            sequence = extra_commands.nodesInGroupOfType(sequence_group, "RVSequence")[
                0
            ]
            composite_property = f"{sequence}.composite"

            effectHook.set_rv_effect_props(
                composite_property,
                {
                    "inputAngularMaskActive": [1],
                    "inputBlendModes": ["over"],
                    "inputAngularMaskPivotX": [pivot_x],
                    "inputAngularMaskPivotY": [pivot_y],
                    "inputAngularMaskAngleInRadians": [angle_in_radians],
                    "swapAngularMaskInput": [is_swapped],
                },
            )
