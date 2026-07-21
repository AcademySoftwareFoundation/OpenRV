#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import rvtypes, commands
import os
import logging
import PyOpenColorIO as OCIO
from functools import partial
from typing import Any, Callable

logging.basicConfig(format="%(levelname)s: %(message)s")

package_logger = logging.getLogger("OCIOSourceSetup")

if "RV_OCIO_SOURCE_SETUP_DEBUG" in os.environ:
    package_logger.setLevel(logging.DEBUG)
else:
    package_logger.setLevel(logging.INFO)

#
#   Default implementations of helper methods
#
#

DEFAULT_PIPE: dict[str, list[str]] = {}

DEFAULT_RV_PIPE: dict[str, list[str]] = {
    "RVLinearizePipelineGroup": ["RVLinearize", "RVLensWarp"],
    "RVLookPipelineGroup": ["RVLookLUT"],
    "RVDisplayPipelineGroup": ["RVDisplayColor"],
}

OCIO_ROLES: dict[str, str] = {"OCIOFile": "RVLinearizePipelineGroup", "OCIOLook": "RVLookPipelineGroup"}

OCIO_DEFAULTS: dict[str, str] = {}

METHODS: list[str] = ["ocio_config_from_media", "ocio_node_from_media"]


def ocio_config_from_media(media: str | None, attributes: dict[str, Any] | None) -> OCIO.Config:
    """
    Retrieve the current OCIO configuration.

    Args:
        media: The media file path (unused in default implementation).
        attributes: Additional attributes (unused in default implementation).

    Returns:
        The current PyOpenColorIO configuration.

    Raises:
        Exception: If the OCIO environment variable is not set.
    """
    if os.getenv("OCIO") is None:
        raise Exception

    return OCIO.GetCurrentConfig()


def ocio_node_from_media(
    config: OCIO.Config, node: str, default: list[str], media: str | None = None, attributes: dict[str, Any] | None = None
) -> list[dict[str, Any]]:
    """
    Generate the OCIO node pipeline configuration based on the media and context.

    Args:
        config: The current OCIO configuration.
        node: The node or pipeline group name to evaluate.
        default: The default pipeline node types.
        media: The media file path.
        attributes: Dictionary containing source attributes and default settings.

    Returns:
        A list of dictionaries representing the node types, contexts, and properties
        required to build the OCIO pipeline.
    """
    if attributes is None:
        attributes = {}

    result = [{"nodeType": d, "context": {}, "properties": {}} for d in default]

    node_type = commands.nodeType(node)

    if node_type == "RVDisplayPipelineGroup":
        display = config.getDefaultDisplay()
        result = [
            {
                "nodeType": "OCIODisplay",
                "context": {},
                "properties": {
                    "ocio.function": "display",
                    "ocio.inColorSpace": OCIO.ROLE_SCENE_LINEAR,
                    "ocio_display.view": config.getDefaultView(display),
                    "ocio_display.display": display,
                },
            }
        ]

    elif node_type == "RVLinearizePipelineGroup":
        in_space = config.parseColorSpaceFromString(media)
        if in_space == "":
            in_space = attributes.get("default_setting", "")
        if in_space != "":
            result = [
                {
                    "nodeType": "OCIOFile",
                    "context": {},
                    "properties": {
                        "ocio.function": "color",
                        "ocio.inColorSpace": in_space,
                        "ocio_color.outColorSpace": OCIO.ROLE_SCENE_LINEAR,
                    },
                },
                {"nodeType": "RVLensWarp", "context": {}, "properties": {}},
            ]

    elif node_type == "RVLookPipelineGroup":
        # If our config has a Look named "shot_specific_look" and uses the
        # environment/context variable "$SHOT" to locate any required files
        # on disk, then this is what that would likely look like:
        #
        # result = [
        #     {"nodeType"   : "OCIOLook",
        #      "context"    : {"SHOT" : os.environ.get("SHOT", "def123")}
        #      "properties" : {
        #          "ocio.function"     : "look",
        #          "ocio.inColorSpace" : OCIO.ROLE_SCENE_LINEAR,
        #          "ocio_look.look"    : "shot_specific_look"}}]

        look = attributes.get("default_setting", "")
        if look != "":
            result = [
                {
                    "nodeType": "OCIOLook",
                    "context": {},
                    "properties": {"ocio.function": "look", "ocio_look.look": look},
                }
            ]

    return result


#
#   A couple of convenience functions
#


def _is_ocio_managed(node_type: str) -> int:
    """
    Internal callback logic to determine if a specific node type is currently managed by OCIO.

    Args:
        node_type: The node type to check.

    Returns:
        The RV menu state (CheckedMenuState if managed, UncheckedMenuState otherwise).
    """
    try:
        managed = commands.getIntProperty(f"#{node_type}.ocio.active")[0] != 0
        return commands.CheckedMenuState if managed else commands.UncheckedMenuState
    except Exception:
        return commands.UncheckedMenuState


def isOCIOManaged(node_type: str) -> Callable[[], int]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_is_ocio_managed, node_type=...)`.
    """
    return partial(_is_ocio_managed, node_type=node_type)


def _is_ocio_display_managed(group: str) -> int:
    """
    Internal callback logic to determine if a display group is currently managed by OCIO.

    Args:
        group: The display group node name.

    Returns:
        The RV menu state (CheckedMenuState if managed, UncheckedMenuState otherwise).
    """
    try:
        group_name = "RVDisplayPipelineGroup"
        d_pipeline = groupMemberOfType(group, group_name)
        d_ocio = groupMemberOfType(d_pipeline, "OCIODisplay")
        managed = commands.getIntProperty(f"{d_ocio}.ocio.active")[0] != 0
        return commands.CheckedMenuState if managed else commands.UncheckedMenuState
    except Exception:
        return commands.UncheckedMenuState


def isOCIODisplayManaged(group: str) -> Callable[[], int]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_is_ocio_display_managed, group=...)`.
    """
    return partial(_is_ocio_display_managed, group=group)


def _ocio_menu_check(node_type: str, prop: str, value: str) -> int:
    """
    Internal callback logic to determine the menu check state for a specific OCIO property.

    Args:
        node_type: The OCIO node type.
        prop: The property name to check.
        value: The value to compare against the current property value.

    Returns:
        The RV menu state (Checked, Neutral, or Disabled).
    """
    try:
        current = commands.getStringProperty(f"#{node_type}.{prop}")[0]
        managed = _is_ocio_managed(node_type) == commands.CheckedMenuState
        checked = current == value and managed
        return commands.CheckedMenuState if checked else commands.NeutralMenuState
    except Exception:
        return commands.DisabledMenuState


def ocioMenuCheck(node_type: str, prop: str, value: str) -> Callable[[], int]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_ocio_menu_check, node_type=..., prop=..., value=...)`.
    """
    return partial(_ocio_menu_check, node_type=node_type, prop=prop, value=value)


def _ocio_display_menu_check(group: str, display: str, view: str) -> int:
    """
    Internal callback logic to determine the menu check state for a display/view combination.

    Args:
        group: The display group node name.
        display: The OCIO display name.
        view: The OCIO view name.

    Returns:
        The RV menu state (Checked, Unchecked, or Disabled).
    """
    try:
        group_name = "RVDisplayPipelineGroup"
        d_pipeline = groupMemberOfType(group, group_name)
        d_ocio = groupMemberOfType(d_pipeline, "OCIODisplay")
        d = commands.getStringProperty(f"{d_ocio}.ocio_display.display")[0]
        v = commands.getStringProperty(f"{d_ocio}.ocio_display.view")[0]
        if d == display and v == view:
            return commands.CheckedMenuState
        return commands.UncheckedMenuState
    except Exception:
        return commands.DisabledMenuState


def ocioDisplayMenuCheck(group: str, display: str, view: str) -> Callable[[], int]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_ocio_display_menu_check, group=..., display=..., view=...)`.
    """
    return partial(_ocio_display_menu_check, group=group, display=display, view=view)


def _ocio_event(event: Any, node_type: str, prop: str, value: str) -> None:
    """
    Internal callback logic to set a property on the current node of node_type in the evaluation path.

    Args:
        event: The RV event object.
        node_type: The OCIO node type.
        prop: The property name to set.
        value: The value to assign to the property.
    """
    commands.setStringProperty(f"#{node_type}.{prop}", [value], True)
    commands.redraw()


def ocioEvent(node_type: str, prop: str, value: str) -> Callable[[Any], None]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_ocio_event, node_type=..., prop=..., value=...)`.
    Note: The internal `_ocio_event` accepts `event` as its first parameter to allow kwargs binding.
    """
    return partial(_ocio_event, node_type=node_type, prop=prop, value=value)


def _ocio_event_on_all_of_type(event: Any, node_type: str, prop: str, value: str) -> None:
    """
    Internal callback logic to set a property on all nodes of node_type.

    Args:
        event: The RV event object.
        node_type: The OCIO node type.
        prop: The property name to set.
        value: The value to assign to the property.
    """
    for node in commands.nodesOfType(node_type):
        commands.setStringProperty(f"{node}.{prop}", [value], True)
    commands.redraw()


def ocioEventOnAllOfType(node_type: str, prop: str, value: str) -> Callable[[Any], None]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_ocio_event_on_all_of_type, node_type=..., prop=..., value=...)`.
    """
    return partial(_ocio_event_on_all_of_type, node_type=node_type, prop=prop, value=value)


def _ocio_display_event(event: Any, group: str, display: str, view: str) -> None:
    """
    Internal callback logic to change the active display and view for a display group.

    Args:
        event: The RV event object.
        group: The display group node name.
        display: The OCIO display name.
        view: The OCIO view name.
    """
    group_name = "RVDisplayPipelineGroup"
    d_pipeline = groupMemberOfType(group, group_name)
    d_ocio = groupMemberOfType(d_pipeline, "OCIODisplay")
    # Both 'display' and 'view' must be set together.
    # Disable the OCIONode during display/view propety changes.
    # Prevents node from rebuilding shaders while it may be in an invalid state.
    commands.setIntProperty(f"{d_ocio}.ocio.active", [0], True)
    commands.setStringProperty(f"{d_ocio}.ocio_display.display", [display], True)
    commands.setStringProperty(f"{d_ocio}.ocio_display.view", [view], True)
    commands.setIntProperty(f"{d_ocio}.ocio.active", [1], True)
    commands.redraw()


def ocioDisplayEvent(group: str, display: str, view: str) -> Callable[[Any], None]:
    """
    Deprecated: Public API maintained for backward compatibility.
    Internal code should use `functools.partial(_ocio_display_event, group=..., display=..., view=...)`.
    """
    return partial(_ocio_display_event, group=group, display=display, view=view)


def groupMemberOfType(node: str, member_type: str) -> str | None:
    """
    Find the first member of a group node that matches a specific node type.

    Args:
        node: The parent group node name.
        member_type: The node type to search for.

    Returns:
        The name of the child node if found, otherwise None.
    """
    for n in commands.nodesInGroup(node):
        if commands.nodeType(n) == member_type:
            return n
    return None


def applyProps(node: str, context_props: dict[str, str], properties_props: dict[str, str]) -> None:
    """
    Apply standard and context properties to an OCIO node.

    Args:
        node: The target node name.
        context_props: A dictionary of context variables and their values.
        properties_props: A dictionary of standard properties and their values.
    """
    for p_prop, a_value in properties_props.items():
        commands.setStringProperty(f"{node}.{p_prop}", [a_value], True)
    for c_prop, c_value in context_props.items():
        prop = f"{node}.ocio_context.{c_prop}"
        if not commands.propertyExists(prop):
            commands.newProperty(prop, commands.StringType, 1)
        commands.setStringProperty(prop, [c_value], True)


#
#   OCIOSourceSetupMode
#


class OCIOSourceSetupMode(rvtypes.MinorMode):
    """
    This mode integrates both with the base RV source_setup package and
    OCIO. The idea is that incoming source media is first examined by the
    base setup package then when appropriate, this package will switch the
    source to use OCIO. If any source uses OCIO, the display is also
    switched over to OCIO control.

    There are many assumptions here. First and foremost is that your OCIO
    worflow uses parseColorSpaceFromString() to determine incoming color
    space.

    ORDERING: this mode uses a sort key of "source_setup" with an
    ordering value of 10 which is after the default source_setup
    mode's ordering value of 0. This ensures that the default
    source_setup is run first and is then followed by the
    ocio_source_setup. If you are using this as an example for a
    different source_setup mode which you wish to have interoperate
    with the default and ocio modes use the same key of "source_setup"
    but with an ordering number that places it relative to the default
    and ocio modes. So for example if you want yours to come before
    the ocio mode, but after the default source_setup use 5 (since its
    between 0 and 10).
    """

    def useSourceOCIO(self, source: str, node_type: str, default_setting: str = "") -> None:
        """
        This tells the source group to use OCIO instead of the RV
        linearize node. There is also ocio.look and ocio.preCache
        which can be activated in this way. For this code we're
        only assuming that OCIO is going to be used to linearize
        the source.

        Args:
            source: The name of the source group node.
            node_type: The OCIO node type to activate (e.g., 'OCIOFile').
            default_setting: The default fallback setting for color space or look.
        """

        medias = commands.getStringProperty(f"{source}.media.movie")
        media = medias[0]

        try:
            src_attrs = commands.sourceAttributes(source, media)
            attr_dict = dict(zip([i[0] for i in src_attrs], [j[1] for j in src_attrs]))
            attr_dict["source_node"] = source
            attr_dict["default_setting"] = default_setting
        except Exception:
            attr_dict = {}

        if self._config is None:
            try:
                self._config = ocio_config_from_media(media, attr_dict)
                OCIO.SetCurrentConfig(self._config)
                commands.defineModeMenu("OCIO Source Setup", self.buildOCIOMenu(), True)
            except Exception:
                return

        #
        # If we already have this OCIO node and we are reading a session,
        # then use the one we have and return
        #

        pipe_slot = OCIO_ROLES[node_type]
        src_pipeline = groupMemberOfType(commands.nodeGroup(source), pipe_slot)
        ocio_node = groupMemberOfType(src_pipeline, node_type)
        if ocio_node is not None and self._reading_session:
            for p_node in commands.nodesInGroup(src_pipeline):
                if commands.nodeType(p_node).startswith("OCIO"):
                    commands.ocioUpdateConfig(p_node)

            package_logger.info("using %s node for %s %s", node_type, source, pipe_slot)
            return

        #
        #   Anywhere in RV there is a pipeline "slot" (File, Linearize,
        #   Look, Display, View) you can use an OCIO node.  Each OCIO node
        #   can futher be configured to act in a manner similar to the nuke
        #   OCIO color, look, or display nodes. In this case we want it to
        #   act as an OCIO color node so we can transform from the incoming
        #   file space to the ROLE_SCENE_LINEAR space (the working space)
        #
        #   You can only get the OCIO node *after* the source group has
        #   been configured to use it. Otherwise the pipelines will not
        #   have been created yet.
        #
        #   Under the hood, an RV "color" OCIO node builds a ColorSpace for
        #   inspace and outspace and uses the processor which converts from
        #   one to the other.
        #

        try:
            if pipe_slot not in DEFAULT_PIPE:
                current_pipeline_nodes = commands.getStringProperty(f"{src_pipeline}.pipeline.nodes")

                # We need to handle the following special case here:
                # We might be in the process of reloading an RV session that
                # is already OCIO color corrected in which case we do not
                # want this pipeline to be considered the default (non OCIO).
                # Example: srcPipelineNodes = [ "OCIOFile" "RVLensWarp" ]
                # We will use the RV default instead in that special case.
                if node_type in current_pipeline_nodes and pipe_slot in DEFAULT_RV_PIPE:
                    DEFAULT_PIPE[pipe_slot] = DEFAULT_RV_PIPE[pipe_slot]
                else:
                    DEFAULT_PIPE[pipe_slot] = current_pipeline_nodes
            pipeline_list = ocio_node_from_media(self._config, src_pipeline, DEFAULT_PIPE[pipe_slot], media, attr_dict)
        except Exception as inst:
            package_logger.error("Problem occurred while loading OCIO settings for %s: %s", node_type, inst)
            return

        try:
            pipeline = [p["nodeType"] for p in pipeline_list]
        except KeyError as inst:
            package_logger.error("Unable to make use of ocio_node_from_media return: %s", inst)
            return
            
        if pipeline == DEFAULT_PIPE[pipe_slot]:
            return

        package_logger.info("using %s node for %s %s", node_type, source, pipe_slot)

        commands.setStringProperty(f"{src_pipeline}.pipeline.nodes", pipeline, True)
        pipe_nodes = commands.nodesInGroup(src_pipeline)
        pipe_nodes.sort()
        for index, p_node in enumerate(pipeline_list):
            stage_ocio = pipe_nodes[index]
            try:
                applyProps(stage_ocio, p_node["context"], p_node["properties"])
            except KeyError as inst:
                package_logger.error("Unable to apply properties to %s: %s", stage_ocio, inst)

        commands.redraw()

    def disableSourceOCIO(self, source: str, node_type: str) -> None:
        """
        This reverts the source group's linearize node back to using
        a native RVLinearize node.

        Args:
            source: The name of the source group node.
            node_type: The OCIO node type being disabled.
        """

        pipe_slot = OCIO_ROLES[node_type]
        src_pipeline = groupMemberOfType(commands.nodeGroup(source), pipe_slot)
        nodes_prop = f"{src_pipeline}.pipeline.nodes"
        current = commands.getStringProperty(nodes_prop)

        if pipe_slot not in DEFAULT_PIPE or current == DEFAULT_PIPE[pipe_slot]:
            return

        package_logger.info("resetting %s for %s", pipe_slot, source)

        commands.setStringProperty(f"{src_pipeline}.pipeline.nodes", DEFAULT_PIPE[pipe_slot], True)
        commands.redraw()

    def useDisplayOCIO(self, group: str) -> None:
        """
        This installs the OCIODisplay node in the DisplayGroup's display pipeline
        in place of RV's RVDisplayColor node.

        NOTE: in RV4 all display devices are separate
        DisplayGroups. So each one can have a completely different
        view and display transform.

        Args:
            group: The display group node name.
        """

        if self._using_ocio_for_display.get(group, False) or self._config is None:
            return

        group_name = "RVDisplayPipelineGroup"
        try:
            d_pipeline = groupMemberOfType(group, group_name)
            if group_name not in DEFAULT_PIPE:
                current_pipeline_nodes = commands.getStringProperty(f"{d_pipeline}.pipeline.nodes")

                # We need to handle the following special case here:
                # We might be in the process of reloading an RV session that
                # is already OCIO color corrected in which case we do not
                # want this pipeline to be considered the default (non OCIO).
                # We will use the RV default instead in that special case.
                if "OCIODisplay" in current_pipeline_nodes and group_name in DEFAULT_RV_PIPE:
                    DEFAULT_PIPE[group_name] = DEFAULT_RV_PIPE[group_name]
                else:
                    DEFAULT_PIPE[group_name] = current_pipeline_nodes
            pipeline_list = ocio_node_from_media(self._config, d_pipeline, DEFAULT_PIPE[group_name])
        except Exception as inst:
            package_logger.error("Problem occurred while loading OCIO settings for OCIODisplay: %s", inst)
            return

        try:
            pipeline = [p["nodeType"] for p in pipeline_list]
        except KeyError as inst:
            package_logger.error("Unable to make use of ocio_node_from_media return: %s", inst)
            return
            
        if pipeline == DEFAULT_PIPE[group_name]:
            return

        device = commands.getStringProperty(f"{group}.device.name")[0]
        package_logger.info("using OCIODisplay for display: %s", device)

        commands.setStringProperty(f"{d_pipeline}.pipeline.nodes", pipeline, True)

        pipe_nodes = commands.nodesInGroup(d_pipeline)
        pipe_nodes.sort()
        for index, p_node in enumerate(pipeline_list):
            stage_ocio = pipe_nodes[index]
            try:
                applyProps(stage_ocio, p_node["context"], p_node["properties"])
            except KeyError as inst:
                package_logger.error("Unable to apply properties to %s: %s", stage_ocio, inst)

        self._using_ocio_for_display[group] = True
        commands.redraw()

    def disableDisplayOCIO(self, group: str) -> None:
        """
        This reverts the DisplayGroup's display pipeline back to using
        RV's native RVDisplayColor node.

        Args:
            group: The display group node name.
        """

        group_name = "RVDisplayPipelineGroup"
        d_pipeline = groupMemberOfType(group, group_name)
        nodes_prop = f"{d_pipeline}.pipeline.nodes"
        current = commands.getStringProperty(nodes_prop)

        if group_name not in DEFAULT_PIPE or current == DEFAULT_PIPE[group_name]:
            return

        commands.setStringProperty(f"{d_pipeline}.pipeline.nodes", DEFAULT_PIPE[group_name], True)

        device = commands.getStringProperty(f"{group}.device.name")[0]
        package_logger.info("using RVDisplayColor for display: %s", device)

        self._using_ocio_for_display[group] = False
        commands.redraw()

    def sourceSetup(self, event: Any) -> None:
        """
        This function should be bound to the "source-group-complete" event. It
        will attempt to use OCIO to infer the incoming file space. If
        it succeeds, the OCIOFile node of the source group is
        activated and used to convert to the ROLE_SCENE_LINEAR space.

        Args:
            event: The RV event object triggering the setup.
        """

        event.reject()  # don't eat this event -- allow others to get it too

        args = event.contents().split(";;")
        group = args[0]
        file_source = groupMemberOfType(group, "RVFileSource")
        image_source = groupMemberOfType(group, "RVImageSource")
        source = file_source if image_source is None else image_source

        for node_type in OCIO_ROLES.keys():
            self.useSourceOCIO(source, node_type)

        #
        #   If this is the first OCIO color pipeline for a source assume
        #   that we also want to use OCIO for display. In this case we're
        #   just going to assume the defaults
        #

        if len(commands.nodesOfType("OCIOFile")) == 1:
            for d_group in commands.nodesOfType("RVDisplayGroup"):
                if not self._using_ocio_for_display.get(d_group, False):
                    self.useDisplayOCIO(d_group)

    def beforeSessionRead(self, event: Any) -> None:
        """
        Flag that a session is currently being read.

        Args:
            event: The RV event object.
        """
        event.reject()
        self._reading_session = True

    def afterSessionRead(self, event: Any) -> None:
        """
        Clear the session read flag and re-initialize OCIO display if needed.

        Args:
            event: The RV event object.
        """
        event.reject()
        self._reading_session = False
        if len(commands.nodesOfType("OCIOFile")) > 1:
            for group in commands.nodesOfType("RVDisplayGroup"):
                if not self._using_ocio_for_display.get(group, False):
                    self.useDisplayOCIO(group)

    def _ocio_active_event(self, event: Any, node_type: str) -> None:
        """
        Toggle the active state of an OCIO node or display group.

        Args:
            event: The RV event object.
            node_type: The OCIO node type or display group to toggle.
        """
        if node_type not in ["OCIOFile", "OCIOLook"]:
            if _is_ocio_display_managed(node_type) == commands.CheckedMenuState:
                self.disableDisplayOCIO(node_type)
            else:
                self.useDisplayOCIO(node_type)
            return

        eval_info = commands.metaEvaluateClosestByType(commands.frame(), "RVFileSource", None)
        if len(eval_info) == 0:
            eval_info = commands.metaEvaluateClosestByType(commands.frame(), "RVImageSource", None)
        if len(eval_info) == 0:
            return
        source = eval_info[0]["node"]

        if _is_ocio_managed(node_type) == commands.CheckedMenuState:
            self.disableSourceOCIO(source, node_type)
        else:
            self.useSourceOCIO(source, node_type, OCIO_DEFAULTS[node_type])

    def ocioActiveEvent(self, node_type: str) -> Callable[[Any], None]:
        """
        Deprecated: Public API maintained for backward compatibility.
        Internal code should use `functools.partial(self._ocio_active_event, node_type=...)`.
        """
        return partial(self._ocio_active_event, node_type=node_type)

    def checkForDisplayGroup(self, event: Any) -> None:
        """
        Check for newly created or modified display groups and rebuild the menu.

        Args:
            event: The RV event object.
        """
        event.reject()
        try:
            node = event.contents()
            if commands.nodeType(node) == "RVDisplayGroup":
                self._using_ocio_for_display[node] = False
                commands.defineModeMenu("OCIO Source Setup", self.buildOCIOMenu(), True)
        except Exception as inst:
            package_logger.error("%s %s", inst, node)

    def maybeUpdateViews(self, event: Any) -> None:
        """
        Rebuild the OCIO menu if a display view has changed.

        Args:
            event: The RV event object.
        """
        event.reject()
        if event.contents().endswith("ocio_display.display"):
            commands.defineModeMenu("OCIO Source Setup", self.buildOCIOMenu(), True)

    def selectConfig(self, event: Any) -> None:
        """
        Prompt the user to manually select an OCIO configuration file.

        Args:
            event: The RV event object.
        """
        try:
            config = commands.openFileDialog(True, False, False, "ocio|OCIO Config", None)[0]
            self._config = OCIO.Config.CreateFromFile(config)
            OCIO.SetCurrentConfig(self._config)
            for source in commands.nodesOfType("RVFileSource") + commands.nodesOfType("RVImageSource"):
                for node_type in OCIO_ROLES.keys():
                    self.disableSourceOCIO(source, node_type)
            for group in commands.nodesOfType("RVDisplayGroup"):
                self.disableDisplayOCIO(group)
            DEFAULT_PIPE.clear()
            for source in commands.nodesOfType("RVFileSource") + commands.nodesOfType("RVImageSource"):
                for node_type in OCIO_ROLES.keys():
                    self.useSourceOCIO(source, node_type)
            for group in commands.nodesOfType("RVDisplayGroup"):
                self._using_ocio_for_display[group] = False
                self.useDisplayOCIO(group)
            commands.defineModeMenu("OCIO Source Setup", self.buildOCIOMenu(), True)
            commands.writeSettings("ocio_source_setup", "ocio_config", config)
        except Exception as inst:
            package_logger.error(inst)

    def buildOCIOMenu(self) -> list[tuple[str, list[Any]]]:
        """
        Construct the RV menu items required for OCIO management.

        Returns:
            A list defining the OCIO menu structure.
        """
        #
        #   Try to acquire OCIO config to populate the display menu
        #

        if self._config is None:
            try:
                self._config = ocio_config_from_media(None, None)
                OCIO.SetCurrentConfig(self._config)
            except Exception:
                return [("OCIO", [("Choose Config...", self.selectConfig, None, None)])]

        #
        #   Make a unique entry for each device's display group
        #

        da_list = []
        for display in commands.nodesOfType("RVDisplayGroup"):
            d_list = [
                (
                    "Active",
                    partial(self._ocio_active_event, node_type=display),
                    None,
                    partial(_is_ocio_display_managed, group=display),
                ),
                ("_", None),
            ]
            for d in self._config.getDisplays():
                v_list = []
                for v in self._config.getViews(d):
                    v_list.append(
                        (
                            v,
                            partial(_ocio_display_event, group=display, display=d, view=v),
                            None,
                            partial(_ocio_display_menu_check, group=display, display=d, view=v),
                        )
                    )
                d_list.append((d, v_list))
            device_name = commands.getStringProperty(f"{display}.device.name")[0]
            device = f"  {device_name}"
            da_list.append((device, d_list))

        #
        #   Apply file space changes only to the visible source
        #

        css_list: list[Any] = [
            (
                "Active",
                partial(self._ocio_active_event, node_type="OCIOFile"),
                None,
                partial(_is_ocio_managed, node_type="OCIOFile"),
            ),
            ("_", None),
        ]
        csa_list: list[Any] = []

        def addPath(family: list[str], tree: list[list[str]]) -> None:
            for f in family:
                for t in tree:
                    if f in t:
                        return addPath(family[1:], t)
                tree.append([f])
                return addPath(family, tree)

        families = [(cs.getFamily().split("/") + [cs.getName()]) for cs in self._config.getColorSpaces()]
        root: list[list[str]] = []
        for family in families:
            addPath(family, root)

        def addMenu(root_node: list[Any], is_single: bool) -> list[Any]:
            if len(root_node) == 1:
                name = root_node[0]
                if is_single:
                    OCIO_DEFAULTS.setdefault("OCIOFile", name)
                    return [
                        (
                            name,
                            partial(_ocio_event, node_type="OCIOFile", prop="ocio.inColorSpace", value=name),
                            None,
                            partial(_ocio_menu_check, node_type="OCIOFile", prop="ocio.inColorSpace", value=name),
                        )
                    ]
                else:
                    return [
                        (
                            name,
                            partial(_ocio_event_on_all_of_type, node_type="OCIOFile", prop="ocio.inColorSpace", value=name),
                            None,
                            partial(_ocio_menu_check, node_type="OCIOFile", prop="ocio.inColorSpace", value=name),
                        )
                    ]
            else:
                menu = []
                for r in root_node[1:]:
                    menu += addMenu(r, is_single)
                return [(root_node[0], menu)]

        for r in root:
            css_list += addMenu(r, True)
            csa_list += addMenu(r, False)

        #
        #   Apply file look changes only to the visible source
        #

        ls_list: list[Any] = [
            (
                "Active",
                partial(self._ocio_active_event, node_type="OCIOLook"),
                None,
                partial(_is_ocio_managed, node_type="OCIOLook"),
            ),
            ("_", None),
        ]
        la_list: list[Any] = []
        for look in self._config.getLooks():
            OCIO_DEFAULTS.setdefault("OCIOLook", look.getName())
            ls_list.append(
                (
                    look.getName(),
                    partial(_ocio_event, node_type="OCIOLook", prop="ocio_look.look", value=look.getName()),
                    None,
                    partial(_ocio_menu_check, node_type="OCIOLook", prop="ocio_look.look", value=look.getName()),
                )
            )
            la_list.append(
                (
                    look.getName(),
                    partial(_ocio_event_on_all_of_type, node_type="OCIOLook", prop="ocio_look.look", value=look.getName()),
                    None,
                    partial(_ocio_menu_check, node_type="OCIOLook", prop="ocio_look.look", value=look.getName()),
                )
            )

        final: list[Any] = [
            ("Current Source", None, None, lambda: commands.DisabledMenuState),
            ("  File Color Space", css_list),
        ]
        if len(ls_list) > 2:
            final += [("  Look", ls_list)]
        final += [
            ("All Sources", None, None, lambda: commands.DisabledMenuState),
            ("  File Color Space", csa_list),
        ]
        if len(la_list) > 0:
            final += [("  Look", la_list)]
        final += [
            ("_", None),
            ("Displays", None, None, lambda: commands.DisabledMenuState),
        ]
        final += da_list
        final += [("_", None)]
        final += [("Change Config...", self.selectConfig, None, None)]

        return [("OCIO", final)]

    def __init__(self) -> None:
        """
        Initialize the minor mode, attempt to load inherited configuration,
        and bind the mode events to RV.
        """
        super().__init__()

        self._using_ocio_for_display: dict[str, bool] = {}
        self._reading_session: bool = False
        self._config: OCIO.Config | None = None

        #
        #   Look for an implementation of the OCIOHelper on the PATH.
        #   Use the default if the import failed.
        #

        try:
            import rv_ocio_setup

            inherited = []
            for method in METHODS:
                try:
                    override_method = getattr(rv_ocio_setup, method)
                    globals()[method] = override_method
                    inherited.append(method)
                except AttributeError:
                    pass

            package_logger.info("Using %s for OCIO setup methods: %s", rv_ocio_setup.__file__, " ".join(inherited))

        except ImportError:
            pass

        # Restore saved OCIO config from previously loaded config
        # An externally set OCIO env var takes precendence
        if os.getenv("OCIO") is None:
            config = commands.readSettings("ocio_source_setup", "ocio_config", "")
            if config != "" and os.path.isfile(config):
                self._config = OCIO.Config.CreateFromFile(config)
                OCIO.SetCurrentConfig(self._config)
            else:
                package_logger.warning("$OCIO environment variable unset!")

        self.init(
            "OCIO Source Setup",
            None,
            [
                (
                    "source-group-complete",
                    self.sourceSetup,
                    "Color and Geometry Management",
                ),
                ("before-session-read", self.beforeSessionRead, ""),
                ("after-session-read", self.afterSessionRead, ""),
                ("graph-new-node", self.checkForDisplayGroup, ""),
                ("graph-node-inputs-changed", self.checkForDisplayGroup, ""),
                ("graph-state-change", self.maybeUpdateViews, ""),
            ],
            self.buildOCIOMenu(),
            "source_setup",
            10,
        )  # source_setup key used by source_setup and this mode


#
#   Dynamically looked up by the mode manager to create this mode. The name
#   matters
#


def createMode() -> OCIOSourceSetupMode:
    """
    Factory function used by the RV Mode Manager to instantiate the mode.

    Returns:
        An instance of OCIOSourceSetupMode.
    """
    return OCIOSourceSetupMode()
