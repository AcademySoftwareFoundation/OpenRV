# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from functools import partial

from rv import commands, extra_commands, rvtypes, qtutils

from PySide6 import QtCore, QtWidgets, QtGui

import annotate_beta_constants as constants
import annotate_beta_paint as paint
from annotate_beta_engine import AnnotateDrawEngine, DrawSettings, ToolState
from annotate_beta_widget import AnnotateToolbarDockWidget

_TOOL_CURSORS = {constants.Tool.CURSOR: QtCore.Qt.ArrowCursor, constants.Tool.TEXT: QtCore.Qt.IBeamCursor}


def _set_tool_cursor(tool):
    commands.setCursor(_TOOL_CURSORS.get(tool, QtCore.Qt.CrossCursor).value)


def _menu_state(checked):
    return commands.CheckedMenuState if checked else commands.NeutralMenuState


class AnnotateBetaMode(rvtypes.MinorMode):
    def __init__(self):
        super().__init__()

        self._dock = None
        self._dock_area = QtCore.Qt.LeftDockWidgetArea
        self._top_level = True
        self._shape_table_pushed = False

        self._settings = DrawSettings(tool_states={})
        self._link_tool_colors = False
        self._auto_save_settings = True

        self._engine = AnnotateDrawEngine(self._settings, self._update_undo_redo_buttons, self._hide_color_picker)

        self.init(
            constants.MODE_NAME,
            self.global_bindings,
            [],
            self.menu,
        )

        commands.bind("default", "global", "session-clear-everything", self._on_session_clear, "Clear annotate history")

        # Register the named event table AFTER init() so mode name is set.
        self._engine.setup_event_table(self)

        self._create_dock()

    # ------------------------------------------------------------------
    # MinorMode interface
    # ------------------------------------------------------------------

    def activate(self):
        super().activate()
        commands.sendInternalEvent("annotate-mode-activated", "")
        # deactivate() pops the event table, so push it again.
        if self._settings.tool in constants.DRAWING_TOOLS:
            self._push_shape_table()

        _set_tool_cursor(self._settings.tool)
        self._update_tool_availability()
        self._update_undo_redo_buttons()
        paint.set_tags()

    def deactivate(self):
        commands.setCursor(QtCore.Qt.ArrowCursor.value)
        self._pop_shape_table()
        self._engine.commit_text_if_active()
        self._dock.toolbar_widget.hide_popups()
        self._dock.hide()

        if self._auto_save_settings:
            self._save_configure_settings()
            self._save_dock_settings()

        paint.remove_tags()
        super().deactivate()

    # ------------------------------------------------------------------
    # Event table push / pop
    # ------------------------------------------------------------------

    def _push_shape_table(self):
        if not self._shape_table_pushed:
            commands.pushEventTable(constants.EVENT_TABLE_NAME)
            self._shape_table_pushed = True

    def _pop_shape_table(self):
        if self._shape_table_pushed:
            commands.popEventTable()
            self._shape_table_pushed = False

    # ------------------------------------------------------------------
    # Settings persistence
    # ------------------------------------------------------------------

    def _load_settings(self):
        """Read the persisted state from RV settings and show it in the widget."""
        for tool in constants.Tool:
            color = QtGui.QColor(
                commands.readSettings(constants.SETTINGS_GROUP, f"{tool}_color", constants.DEFAULT_COLOR)
            )
            self._settings.tool_states[tool] = ToolState(
                color=color if color.isValid() else QtGui.QColor(constants.DEFAULT_COLOR),
                size=commands.readSettings(constants.SETTINGS_GROUP, f"{tool}_size", constants.DEFAULT_SIZE),
                opacity=commands.readSettings(constants.SETTINGS_GROUP, f"{tool}_opacity", constants.DEFAULT_OPACITY),
                color_modifier=commands.readSettings(
                    constants.SETTINGS_GROUP, f"{tool}_color_modifier", constants.ColorModifier.NORMAL
                ),
                filled=commands.readSettings(constants.SETTINGS_GROUP, f"{tool}_filled", False),
            )

        saved_tool = commands.readSettings(constants.SETTINGS_GROUP, "active_tool", constants.Tool.PEN)
        if saved_tool in self._settings.tool_states:
            self._settings.tool = saved_tool

        self._settings.eraser_brush = commands.readSettings(
            constants.SETTINGS_GROUP, "eraser_brush", constants.Brush.CIRCLE
        )

        self._settings.font_family = commands.readSettings(
            constants.SETTINGS_GROUP, "font_family", constants.DEFAULT_FONT_FAMILY
        )
        self._settings.font_size = commands.readSettings(
            constants.SETTINGS_GROUP, "font_size", constants.FontSize.MEDIUM
        )
        self._settings.font_bold = commands.readSettings(constants.SETTINGS_GROUP, "font_bold", False)
        self._settings.font_italic = commands.readSettings(constants.SETTINGS_GROUP, "font_italic", False)
        self._settings.font_underline = commands.readSettings(constants.SETTINGS_GROUP, "font_underline", False)

        # Configure settings
        self._settings.store_on_source = commands.readSettings(constants.SETTINGS_GROUP, "store_on_source", False)
        self._settings.auto_mark = commands.readSettings(constants.SETTINGS_GROUP, "auto_mark", False)
        self._link_tool_colors = commands.readSettings(constants.SETTINGS_GROUP, "link_tool_colors", False)
        self._settings.sync_whole_strokes = commands.readSettings(constants.SETTINGS_GROUP, "sync_whole_strokes", False)
        self._settings.scale_brush = commands.readSettings(constants.SETTINGS_GROUP, "scale_brush", True)
        self._auto_save_settings = commands.readSettings(constants.SETTINGS_GROUP, "auto_save_settings", True)

        toolbar = self._dock.toolbar_widget
        toolbar.strip.set_active_tool(self._settings.tool)
        toolbar.panel.set_page_for_tool(self._settings.tool)
        toolbar.panel.set_eraser_brush(self._settings.eraser_brush)
        toolbar.panel.set_font_family(self._settings.font_family)
        toolbar.panel.set_font_size(self._settings.font_size)
        toolbar.panel.set_bold(self._settings.font_bold)
        toolbar.panel.set_italic(self._settings.font_italic)
        toolbar.panel.set_underline(self._settings.font_underline)
        self._show_tool_state()

    def _save_tool_state(self, tool):
        state = self._settings.tool_states[tool]
        commands.writeSettings(constants.SETTINGS_GROUP, f"{tool}_color", state.color.name())
        commands.writeSettings(constants.SETTINGS_GROUP, f"{tool}_size", state.size)
        commands.writeSettings(constants.SETTINGS_GROUP, f"{tool}_opacity", state.opacity)
        commands.writeSettings(constants.SETTINGS_GROUP, f"{tool}_color_modifier", state.color_modifier)
        commands.writeSettings(constants.SETTINGS_GROUP, f"{tool}_filled", state.filled)

    def _show_tool_state(self):
        state = self._settings.tool_state
        toolbar = self._dock.toolbar_widget
        toolbar.set_color(state.color)
        toolbar.set_size(state.size)
        toolbar.set_opacity(state.opacity)
        toolbar.set_filled(state.filled)
        toolbar.panel.set_color_modifier(state.color_modifier)

    def _save_configure_settings(self):
        commands.writeSettings(constants.SETTINGS_GROUP, "store_on_source", self._settings.store_on_source)
        commands.writeSettings(constants.SETTINGS_GROUP, "auto_mark", self._settings.auto_mark)
        commands.writeSettings(constants.SETTINGS_GROUP, "link_tool_colors", self._link_tool_colors)
        commands.writeSettings(constants.SETTINGS_GROUP, "sync_whole_strokes", self._settings.sync_whole_strokes)
        commands.writeSettings(constants.SETTINGS_GROUP, "scale_brush", self._settings.scale_brush)
        commands.writeSettings(constants.SETTINGS_GROUP, "auto_save_settings", self._auto_save_settings)

    def _save_dock_settings(self):
        commands.writeSettings(constants.SETTINGS_GROUP, "dock_area", self._dock_area.value)
        commands.writeSettings(constants.SETTINGS_GROUP, "top_level", self._top_level)

    # ------------------------------------------------------------------
    # Dock creation
    # ------------------------------------------------------------------

    def _create_dock(self):
        session_window = qtutils.sessionWindow()
        self._dock = AnnotateToolbarDockWidget(session_window)
        self._dock.close_requested.connect(self._on_close_requested)

        self._dock.dockLocationChanged.connect(self._on_dock_location_changed)
        self._dock_area = QtCore.Qt.DockWidgetArea(
            commands.readSettings(constants.SETTINGS_GROUP, "dock_area", QtCore.Qt.LeftDockWidgetArea.value)
        )

        self._dock.topLevelChanged.connect(self._on_top_level_changed)
        self._top_level = commands.readSettings(constants.SETTINGS_GROUP, "top_level", True)

        session_window.addDockWidget(self._dock_area, self._dock)
        session_window.resizeDocks([self._dock], [115], QtCore.Qt.Horizontal)

        for existing in session_window.findChildren(QtWidgets.QDockWidget):
            if existing is self._dock:
                continue
            if session_window.dockWidgetArea(existing) == self._dock_area:
                session_window.tabifyDockWidget(existing, self._dock)
                break

        toolbar = self._dock.toolbar_widget
        toolbar.tool_changed.connect(self._on_tool_changed)
        toolbar.color_changed.connect(self._on_color_changed)
        toolbar.size_changed.connect(partial(self._on_tool_state_changed, "size"))
        toolbar.opacity_changed.connect(partial(self._on_tool_state_changed, "opacity"))
        toolbar.filled_changed.connect(partial(self._on_tool_state_changed, "filled"))
        toolbar.color_modifier_changed.connect(partial(self._on_tool_state_changed, "color_modifier"))
        toolbar.font_family_changed.connect(partial(self._on_font_setting_changed, "font_family"))
        toolbar.font_size_changed.connect(partial(self._on_font_setting_changed, "font_size"))
        toolbar.font_bold_changed.connect(partial(self._on_font_setting_changed, "font_bold"))
        toolbar.font_italic_changed.connect(partial(self._on_font_setting_changed, "font_italic"))
        toolbar.font_underline_changed.connect(partial(self._on_font_setting_changed, "font_underline"))
        toolbar.undo_requested.connect(self._engine.undo)
        toolbar.redo_requested.connect(self._engine.redo)
        toolbar.clear_requested.connect(self._engine.clear_frame)
        toolbar.clear_all_requested.connect(self._engine.clear_all_frames)
        toolbar.eraser_brush_changed.connect(self._on_eraser_brush_changed)

        self._load_settings()

    # ------------------------------------------------------------------
    # Signal handlers
    # ------------------------------------------------------------------

    def _on_tool_changed(self, tool):
        # Always commit text when leaving the text tool, regardless of where we go.
        if self._settings.tool == constants.Tool.TEXT:
            self._engine.commit_text_if_active()

        outgoing_tool = self._settings.tool
        self._settings.tool = tool
        commands.writeSettings(constants.SETTINGS_GROUP, "active_tool", tool)

        # Coming from the eyedropper, the incoming tool takes the sampled color.
        if outgoing_tool == constants.Tool.EYEDROPPER:
            self._settings.tool_state.color = QtGui.QColor(self._settings.tool_states[outgoing_tool].color)
            self._save_tool_state(tool)

        self._show_tool_state()

        if tool in constants.DRAWING_TOOLS:
            self._push_shape_table()
        else:
            self._pop_shape_table()

        _set_tool_cursor(tool)

    def _on_color_changed(self, color):
        tools = constants.Tool if self._link_tool_colors else [self._settings.tool]
        for tool in tools:
            self._settings.tool_states[tool].color = QtGui.QColor(color)
            self._save_tool_state(tool)

    def _on_tool_state_changed(self, attribute, value):
        setattr(self._settings.tool_state, attribute, value)
        self._save_tool_state(self._settings.tool)

    def _on_eraser_brush_changed(self, brush):
        self._settings.eraser_brush = brush
        commands.writeSettings(constants.SETTINGS_GROUP, "eraser_brush", brush)

    def _on_font_setting_changed(self, attribute, value):
        self._engine.commit_text_if_active()
        setattr(self._settings, attribute, value)
        commands.writeSettings(constants.SETTINGS_GROUP, attribute, value)

    def _show_toolbar(self):
        self._dock.show()
        if self._top_level and not self._dock.isFloating():
            self._dock.setFloating(True)
        self._dock.raise_()

    def _on_close_requested(self):
        if self.isActive():
            self.toggle()
        else:
            self._dock.hide()

    def _on_dock_location_changed(self, area: QtCore.Qt.DockWidgetArea):
        if area is not QtCore.Qt.NoDockWidgetArea:
            self._dock_area = area

    def _on_top_level_changed(self, top_level: bool):
        self._top_level = top_level

    def _hide_color_picker(self):
        self._dock.toolbar_widget.hide_popups()

    def _update_undo_redo_buttons(self):
        toolbar = self._dock.toolbar_widget
        toolbar.set_undo_enabled(self._engine.has_undo())
        toolbar.set_redo_enabled(self._engine.has_redo())

    def _update_tool_availability(self):
        """Enable/disable tools whose RV event categories are currently disabled."""
        toolbar = self._dock.toolbar_widget

        # Hide/show the toolbar depending on if the annotate category is enabled or not.
        # isActive() is the authority on whether the user wants the toolbar open, so unrelated
        # category-state events (e.g., menu-bar clicks) can't pop it open, and the
        # toolbar comes back on its own once the category is re-enabled.
        if commands.isEventCategoryEnabled("annotate_category"):
            if self.isActive():
                self._show_toolbar()
        else:
            self._dock.hide()

        airbrush_on = commands.isEventCategoryEnabled("annotate_airbrush_category")
        burn_on = commands.isEventCategoryEnabled("annotate_burn_category")
        dodge_on = commands.isEventCategoryEnabled("annotate_dodge_category")
        soft_erase_on = commands.isEventCategoryEnabled("annotate_softerase_category")
        hard_erase_on = commands.isEventCategoryEnabled("annotate_harderase_category")
        text_on = commands.isEventCategoryEnabled("annotate_text_category")
        sample_on = commands.isEventCategoryEnabled("annotate_sample_category")

        toolbar.set_tool_enabled(constants.Tool.AIRBRUSH, airbrush_on)
        toolbar.set_blend_mode_enabled(constants.ColorModifier.DARKEN, burn_on)
        toolbar.set_blend_mode_enabled(constants.ColorModifier.ADDITIVE, dodge_on)
        toolbar.set_soft_erase_enabled(soft_erase_on)
        # Disable the eraser tool entirely only when both erase modes are unavailable.
        toolbar.set_tool_enabled(constants.Tool.ERASER, hard_erase_on or soft_erase_on)
        toolbar.set_tool_enabled(constants.Tool.TEXT, text_on)
        toolbar.set_tool_enabled(constants.Tool.EYEDROPPER, sample_on)

    def _on_node_inputs_changed(self, event):
        node = event.contents()
        view_node = commands.viewNode()

        if view_node and node == view_node:
            paint.remove_tags()
            paint.set_tags()

        event.reject()

    def _on_before_graph_view_change(self, event):
        paint.remove_tags()
        event.reject()

    def _on_after_graph_view_change(self, event):
        paint.set_tags()
        event.reject()

    def _on_category_state_changed(self, event):
        self._update_tool_availability()
        event.reject()

    def _on_undo_redo_clear_update(self, event):
        """Fired once an incoming remote paint update has been applied.

        A remote client's undo/redo/clear changes the annotation state on the current
        frame, so our undo/redo button enabled state may be stale.
        """
        self._update_undo_redo_buttons()
        event.reject()

    def _on_set_current_annotate_node(self, event):
        """Receive the paint node that annotations should be stored on.

        An external package sends this event to direct us to draw on the annotation
        source group's RVPaint node rather than the local pipeline node, so that it can track
        and sync all annotations.

        Only accept the node if it's actually part of the current view's paint nodes, otherwise
        clear the override rather than storing a name that may never resolve.
        """
        node_name = event.contents() or ""
        self._engine.preferred_paint_node = ""
        if node_name:
            try:
                infos = commands.metaEvaluate(commands.frame(), commands.viewNode())
            except Exception:  # raised when the view node name is invalid
                infos = []
            if any(info.get("nodeType") == "RVPaint" and info.get("node") == node_name for info in infos):
                self._engine.preferred_paint_node = node_name
        event.reject()

    def _on_session_clear(self, event):
        self._engine.clear_annotate_history()
        event.reject()

    # ------------------------------------------------------------------
    # Configure menu handlers
    # ------------------------------------------------------------------

    @staticmethod
    def _toggle_item(label, target, attribute):
        """Menu entry that flips the boolean `attribute` on `target`."""

        def toggle(event):
            setattr(target, attribute, not getattr(target, attribute))

        return (label, toggle, None, lambda: _menu_state(getattr(target, attribute)))

    def _toggle_link_colors(self, event):
        self._link_tool_colors = not self._link_tool_colors
        if self._link_tool_colors:
            self._on_color_changed(self._settings.tool_state.color)

    def _toggle_live_drawing(self, event):
        self._settings.sync_whole_strokes = not self._settings.sync_whole_strokes

    def _toggle_sync_auto_start(self, event):
        """Add or remove this mode from the Sync extraModes list."""
        modes = commands.readSettings("Sync", "extraModes", [])
        if constants.MODE_NAME in modes:
            modes.remove(constants.MODE_NAME)
        else:
            modes.append(constants.MODE_NAME)
        commands.writeSettings("Sync", "extraModes", modes)

    def _sync_auto_start_state(self):
        return _menu_state(constants.MODE_NAME in commands.readSettings("Sync", "extraModes", []))

    # ------------------------------------------------------------------
    # Bindings and menu
    # ------------------------------------------------------------------

    @property
    def global_bindings(self):
        return [
            ("pointer-1--push", self._on_eyedropper_click, "Eyedropper sample"),
            ("stylus-pen--push", self._on_eyedropper_click, "Eyedropper sample (stylus)"),
            ("graph-node-inputs-changed", self._on_node_inputs_changed, "Update UI"),
            ("before-graph-view-change", self._on_before_graph_view_change, "Update UI"),
            ("after-graph-view-change", self._on_after_graph_view_change, "Update UI"),
            ("event-category-state-changed", self._on_category_state_changed, "Update tool availability"),
            ("set-current-annotate-mode-node", self._on_set_current_annotate_node, "Set preferred paint node"),
            (
                "undo-redo-clear-ui-update",
                self._on_undo_redo_clear_update,
                "Sync undo/redo state after remote paint update",
            ),
            # Matches the legacy annotate_mode package's Next/Previous Annotated Frame hotkeys.
            # (RV joins simultaneous modifiers with a single dash, e.g. "alt-shift", and
            # brackets the modifier block with double dashes -- see QTTranslator::modifierString.)
            ("key-down--alt-shift--right", self._next_annotated_frame, "Next Annotated Frame"),
            ("key-down--alt-shift--left", self._previous_annotated_frame, "Previous Annotated Frame"),
        ]

    def _on_eyedropper_click(self, event):
        if self._settings.tool != constants.Tool.EYEDROPPER:
            event.reject()
            return
        pointer = event.pointer()
        device_pixel_ratio = commands.devicePixelRatio()
        x = pointer[0] * device_pixel_ratio
        y = pointer[1] * device_pixel_ratio
        pixel = commands.framebufferPixelValue(x, y)
        if pixel and len(pixel) >= 3:
            color = QtGui.QColor.fromRgbF(
                min(1.0, max(0.0, pixel[0])),
                min(1.0, max(0.0, pixel[1])),
                min(1.0, max(0.0, pixel[2])),
            )
            self._settings.tool_state.color = color
            self._dock.toolbar_widget.set_color(color)
            self._save_tool_state(self._settings.tool)

    # ------------------------------------------------------------------
    # Next/Previous Annotated Frame navigation
    # ------------------------------------------------------------------
    # Ported from the legacy annotate_mode.mu nextAnnotatedFrame/prevAnnotatedFrame.
    # findAnnotatedFrames() returns an unsorted, possibly-duplicated list; when there
    # is no frame in the requested direction, Mu falls back to the list's last entry
    # (next) or first entry (previous).

    def _next_annotated_frame(self, event):
        frames = extra_commands.findAnnotatedFrames()
        if frames:
            current = commands.frame()
            commands.setFrame(min((frame for frame in frames if frame > current), default=frames[-1]))

    def _previous_annotated_frame(self, event):
        frames = extra_commands.findAnnotatedFrames()
        if frames:
            current = commands.frame()
            commands.setFrame(max((frame for frame in frames if frame < current), default=frames[0]))

    def _annotated_frame_navigation_state(self):
        return commands.DisabledMenuState if extra_commands.isSessionEmpty() else commands.UncheckedMenuState

    @property
    def menu(self):
        configure_items = [
            self._toggle_item("Draw On Source When Possible", self._settings, "store_on_source"),
            self._toggle_item("Automatically Mark Annotated Frames", self._settings, "auto_mark"),
            ("Link Tool Colors", self._toggle_link_colors, None, lambda: _menu_state(self._link_tool_colors)),
            (
                "Live Drawing in Sync",
                self._toggle_live_drawing,
                None,
                lambda: _menu_state(not self._settings.sync_whole_strokes),
            ),
            ("Start Automatically During Sync", self._toggle_sync_auto_start, None, self._sync_auto_start_state),
            self._toggle_item("Brush Size Relative to View", self._settings, "scale_brush"),
            self._toggle_item("Always Save Settings as Defaults On Exit", self, "_auto_save_settings"),
        ]

        return [
            (
                "Annotation",
                [
                    (
                        "Next Annotated Frame",
                        self._next_annotated_frame,
                        "alt shift rightArrow",
                        self._annotated_frame_navigation_state,
                    ),
                    (
                        "Previous Annotated Frame",
                        self._previous_annotated_frame,
                        "alt shift leftArrow",
                        self._annotated_frame_navigation_state,
                    ),
                    ("Configure", configure_items),
                ],
            ),
        ]


def createMode():
    return AnnotateBetaMode()
