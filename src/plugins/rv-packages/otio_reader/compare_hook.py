import effectHook
import opentimelineio as otio
from rv import commands


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
            stack_group = argument_map["stack"]

            blend_modes_property = "CompareTrack_sequence.composite.inputBlendModes"
            if not commands.propertyExists(blend_modes_property):
                commands.newProperty(blend_modes_property, commands.StringType, 0)
            commands.setStringProperty(blend_modes_property, ["over"], True)

            opacities_property = "CompareTrack_sequence.composite.inputOpacities"
            if not commands.propertyExists(opacities_property):
                commands.newProperty(opacities_property, commands.FloatType, 0)
            commands.setFloatProperty(opacities_property, [1.0 - opacity], True)

            commands.setViewNode(stack_group)
