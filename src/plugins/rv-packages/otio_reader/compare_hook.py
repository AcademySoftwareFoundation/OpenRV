import effectHook
import opentimelineio as otio
from rv import commands, extra_commands


def hook_function(
    in_timeline: otio.schemadef.Compare.Compare, argument_map: dict | None = None
) -> None:
    """A hook for the compare schema"""
    compare_mode = in_timeline.mode

    match (compare_mode):
        case "side_by_side":
            orientation = in_timeline.side_by_side["orientation"]
            gap = in_timeline.side_by_side["gap"]

            all_nodes = commands.nodes()
            layout_node = [
                node for node in all_nodes if commands.nodeType(node) == "RVLayoutGroup"
            ][0]

            if orientation == "horizontal":
                commands.setStringProperty(f"{layout_node}.layout.mode", ["row"])
            else:
                commands.setStringProperty(f"{layout_node}.layout.mode", ["column"])
            # commands.setFloatProperty(f"{layout_node}.layout.spacing", [gap])

        case "over_with_opacity":
            opacity = in_timeline.over_with_opacity["opacity"]
            if not isinstance(opacity, float):
                opacity = float(opacity)
            stack_group = argument_map["stack"]

            blend_modes_property = "CompareTrack_sequence.composite.inputBlendModes"
            if not commands.propertyExists(blend_modes_property):
                commands.newProperty(blend_modes_property, commands.StringType, 0)
            commands.setStringProperty(blend_modes_property, ["over"], True)

            opacities_property = "CompareTrack_sequence.composite.inputOpacities"
            if not commands.propertyExists(opacities_property):
                commands.newProperty(opacities_property, commands.FloatType, 0)
            commands.setFloatProperty(opacities_property, [opacity], True)

            commands.setViewNode(stack_group)

        case "difference":
            offset = in_timeline.difference["offset"]
            slope = in_timeline.difference["slope"]

            stack_group = argument_map["stack"]

            rv_cdl = commands.newNode("RVLinearize", "linearize")

            cdl_property = f"{rv_cdl}.CDL"
            effectHook.set_rv_effect_props(
                cdl_property,
                {
                    "active": [1],
                    "slope": [slope, slope, slope],
                    "offset": [offset, offset, offset],
                },
            )

            blend_modes_property = "tracks_stack.composite.type"
            if not commands.propertyExists(blend_modes_property):
                commands.newProperty(blend_modes_property, commands.StringType, 0)
            commands.setStringProperty(blend_modes_property, ["difference"], True)

            commands.setViewNode(stack_group)

            return rv_cdl
        case "angular_mask":
            angle_in_radians = in_timeline.angular_mask["angle_in_radians"]
            pivot = in_timeline.angular_mask["pivot"]
            pivot_x = float(pivot["x"])
            pivot_y = float(pivot["y"])

            source = argument_map["src"]
            transform = extra_commands.associatedNode("RVTransform2D", source)
            if commands.getIntProperty("{}.transform.active".format(transform))[0] != 0:
                global_translate_vec = otio.schema.V2d(0.0, 0.0)
                global_scale_vec = otio.schema.V2d(1.0, 1.0)

                if commands.propertyExists(
                    "{}.otio.global_translate".format(transform)
                ) and commands.propertyExists("{}.otio.global_scale".format(transform)):
                    global_translate = commands.getFloatProperty(
                        "{}.otio.global_translate".format(transform)
                    )
                    global_scale = commands.getFloatProperty(
                        "{}.otio.global_scale".format(transform)
                    )
                    global_translate_vec = otio.schema.V2d(
                        global_translate[0], global_translate[1]
                    )
                    global_scale_vec = otio.schema.V2d(global_scale[0], global_scale[1])

                translate = commands.getFloatProperty(
                    "{}.transform.translate".format(transform),
                )
                scale = commands.getFloatProperty(
                    "{}.transform.scale".format(transform),
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

                pivot_x = (pivot_x - bounds_center.x) / bounds_size.x
                pivot_y = (pivot_y - bounds_center.y) / bounds_size.y

                pivot_x += 0.5 * aspect_ratio
                pivot_y += 0.5

            stack_group = argument_map["stack"]
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
                },
            )

            commands.setViewNode(stack_group)
