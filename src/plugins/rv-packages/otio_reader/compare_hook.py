import logging
import effectHook
import opentimelineio as otio
from rv import commands, extra_commands


def hook_function(
    in_timeline: otio.schemadef.Compare.Compare, argument_map: dict | None = None
) -> None:
    """A hook for the compare schema"""
    breakpoint()
    pass
