import logging
import effectHook
import opentimelineio as otio
from rv import commands, extra_commands


def hook_function(
    in_timeline: otio.schemadef.Compare.Compare, argument_map: dict | None = None
) -> None:
    """A hook for the compare schema"""
    breakpoint()
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

            commands.setViewNode("defaultLayout")
