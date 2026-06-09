# Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from rv import commands, extra_commands, rvtypes, qtutils

try:
    from PySide2 import QtCore, QtWidgets, QtGui
except ImportError:
    from PySide6 import QtCore, QtWidgets, QtGui

from annotate_toolbar_widget import (
    AnnotateToolbarDockWidget,
    TOOL_PEN,
    TOOL_TEXT,
    TOOL_AIRBRUSH,
    TOOL_ERASER,
    TOOL_EYEDROPPER,
    COLOR_MOD_ADDITIVE,
    COLOR_MOD_DARKEN,
)
from annotate_toolbar_engine import AnnotateDrawEngine, TABLE_NAME, _DRAWING_TOOLS

_DEFAULT_COLOUR_HEX = "#ffdc00"
_DEFAULT_SIZE = 32
_DEFAULT_OPACITY = 50

_SETTINGS_GROUP = "AnnotateToolbar"
_ALL_TOOLS = ("cursor", "pen", "airbrush", "eraser", "rect", "circle", "arrow", "line", "text", "eyedropper")


class AnnotateToolbarMode(rvtypes.MinorMode):
    def __init__(self):
        rvtypes.MinorMode.__init__(self)

        self._dock = None
        self._shape_table_pushed = False

        # Current tool state — drawing engine reads these
        self._tool = TOOL_PEN
        self._colour = QtGui.QColor(_DEFAULT_COLOUR_HEX)
        self._size = _DEFAULT_SIZE
        self._opacity = _DEFAULT_OPACITY
        self._filled = False
        self._font_family = "Helvetica"
        self._font_size = "medium"
        self._font_bold = False
        self._font_italic = False
        self._font_underline = False
        self._color_modifier = "normal"
        self._eraser_brush = "circle"  # "circle" = hard, "gauss" = soft

        # Paint node override set by live_review via set-current-annotate-mode-node.
        # When non-empty, the engine uses this node instead of metaEvaluate so that
        # RV-drawn annotations land on the annotation source group's RVPaint node
        # (the same one live_review tracks) rather than the local pipeline node.
        self._preferred_paint_node = ""

        # Set to True when the user explicitly closes the dock via the × button so
        # that activate() does not force-reopen it on mode re-activation.
        self._user_closed = False

        # Tracks whether the old "Draw" dock was visible when the user opened the new
        # toolbar, so we can restore it if they close the new toolbar.
        self._old_draw_dock_was_visible = False

        # True when the dock was hidden because Live Review revoked annotate_category
        # permission.  Used to restore the toolbar when permission is re-granted,
        # without auto-showing it on unrelated event-category-state-changed events
        # (e.g., menu-bar clicks).
        self._hidden_by_lr = False

        # Configure settings — match old annotate_mode.mu defaults
        self._store_on_src = False  # "Draw On Source When Possible"
        self._auto_mark = False  # "Automatically Mark Annotated Frames"
        self._link_tool_colors = False  # "Unique Color For Each Tool" = NOT linked
        self._sync_whole_strokes = True  # True = batch stroke; False = send each point live
        self._sync_auto_start = False  # "Start Automatically During Sync"
        self._scale_brush = True  # "Brush Size Relative to View"
        self._auto_save_settings = True  # "Always Save Settings as Defaults On Exit"

        # Per-tool state memory — keyed by tool identifier.
        self._tool_colours = {}
        self._tool_sizes = {}
        self._tool_opacities = {}
        self._tool_color_modifiers = {}

        self._engine = AnnotateDrawEngine(self)

        self.init(
            "annotate_toolbar_mode",
            self.global_bindings,
            [],  # no global pointer bindings — only the named table below
            self.menu,
        )

        # Register the named event table AFTER init() so mode name is set.
        self._engine.setup_event_table(self)

        self._create_dock()

    # ------------------------------------------------------------------
    # MinorMode interface
    # ------------------------------------------------------------------

    def activate(self):
        rvtypes.MinorMode.activate(self)
        commands.sendInternalEvent("annotate-mode-activated", "")
        # Re-push the event table in case the mode was deactivated by a view change.
        if self._tool in _DRAWING_TOOLS:
            self._push_shape_table()

    def deactivate(self):
        self._pop_shape_table()
        self._engine.commit_text_if_active()
        self._dock.toolbar_widget.hide_popups()
        self._dock.hide()
        if self._auto_save_settings:
            self._save_configure_settings()
        rvtypes.MinorMode.deactivate(self)

    # ------------------------------------------------------------------
    # Event table push / pop
    # ------------------------------------------------------------------

    def _push_shape_table(self):
        if not self._shape_table_pushed:
            commands.pushEventTable(TABLE_NAME)
            self._shape_table_pushed = True

    def _pop_shape_table(self):
        if self._shape_table_pushed:
            commands.popEventTable()
            self._shape_table_pushed = False

    # ------------------------------------------------------------------
    # Settings persistence
    # ------------------------------------------------------------------

    @staticmethod
    def _read(group, key, default):
        """readSettings wrapper that handles None returns and missing-key exceptions."""
        try:
            val = commands.readSettings(group, key, default)
            return val if val is not None else default
        except Exception:
            return default

    def _load_settings(self):
        """Read per-tool state from RV settings and apply to the in-memory dicts and widget."""
        g = _SETTINGS_GROUP
        for tool in _ALL_TOOLS:
            hex_col = self._read(g, f"{tool}_colour", _DEFAULT_COLOUR_HEX)
            colour = QtGui.QColor(hex_col)
            self._tool_colours[tool] = colour if colour.isValid() else QtGui.QColor(_DEFAULT_COLOUR_HEX)
            self._tool_sizes[tool] = int(self._read(g, f"{tool}_size", _DEFAULT_SIZE))
            self._tool_opacities[tool] = int(self._read(g, f"{tool}_opacity", _DEFAULT_OPACITY))
            self._tool_color_modifiers[tool] = self._read(g, f"{tool}_color_modifier", "normal")

        saved_tool = self._read(g, "active_tool", TOOL_PEN)
        if saved_tool in _ALL_TOOLS:
            self._tool = saved_tool

        self._eraser_brush = self._read(g, "eraser_brush", "circle")

        self._font_family = self._read(g, "font_family", "Helvetica")
        self._font_size = self._read(g, "font_size", "medium")
        self._font_bold = bool(self._read(g, "font_bold", False))
        self._font_italic = bool(self._read(g, "font_italic", False))
        self._font_underline = bool(self._read(g, "font_underline", False))

        # Configure settings
        self._store_on_src = bool(self._read(g, "cfg_store_on_src", False))
        self._auto_mark = bool(self._read(g, "cfg_auto_mark", False))
        self._link_tool_colors = bool(self._read(g, "cfg_link_tool_colors", False))
        self._sync_whole_strokes = bool(self._read(g, "cfg_sync_whole_strokes", True))
        self._scale_brush = bool(self._read(g, "cfg_scale_brush", True))
        self._auto_save_settings = bool(self._read(g, "cfg_auto_save_settings", True))
        self._sync_auto_start = self._read_sync_auto_start()

        # Apply the saved state for the active tool to the live variables and widget.
        self._colour = QtGui.QColor(self._tool_colours.get(self._tool, QtGui.QColor(_DEFAULT_COLOUR_HEX)))
        self._size = self._tool_sizes.get(self._tool, _DEFAULT_SIZE)
        self._opacity = self._tool_opacities.get(self._tool, _DEFAULT_OPACITY)
        self._color_modifier = self._tool_color_modifiers.get(self._tool, "normal")

        w = self._dock.toolbar_widget
        w.strip.set_active_tool(self._tool)
        w.panel.set_page_for_tool(self._tool)
        w.set_colour(self._colour)
        w.set_size(self._size)
        w.set_opacity(self._opacity)
        w.panel.set_color_modifier(self._color_modifier)
        w.panel.set_eraser_brush(self._eraser_brush)
        w.panel.set_font_family(self._font_family)
        w.panel.set_font_size(self._font_size)
        w.panel.set_bold(self._font_bold)
        w.panel.set_italic(self._font_italic)
        w.panel.set_underline(self._font_underline)

    def _save_tool_state(self, tool):
        """Persist colour/size/opacity for one tool."""
        g = _SETTINGS_GROUP
        colour = self._tool_colours.get(tool, QtGui.QColor(_DEFAULT_COLOUR_HEX))
        try:
            commands.writeSettings(g, f"{tool}_colour", colour.name())
            commands.writeSettings(g, f"{tool}_size", self._tool_sizes.get(tool, _DEFAULT_SIZE))
            commands.writeSettings(g, f"{tool}_opacity", self._tool_opacities.get(tool, _DEFAULT_OPACITY))
            commands.writeSettings(g, f"{tool}_color_modifier", self._tool_color_modifiers.get(tool, "normal"))
        except Exception as e:
            print(f"[annotate_toolbar] settings write error: {e}")

    def _save_configure_settings(self):
        """Persist Configure submenu settings."""
        g = _SETTINGS_GROUP
        try:
            commands.writeSettings(g, "cfg_store_on_src", self._store_on_src)
            commands.writeSettings(g, "cfg_auto_mark", self._auto_mark)
            commands.writeSettings(g, "cfg_link_tool_colors", self._link_tool_colors)
            commands.writeSettings(g, "cfg_sync_whole_strokes", self._sync_whole_strokes)
            commands.writeSettings(g, "cfg_scale_brush", self._scale_brush)
            commands.writeSettings(g, "cfg_auto_save_settings", self._auto_save_settings)
        except Exception as e:
            print(f"[annotate_toolbar] configure settings write error: {e}")

    def _read_sync_auto_start(self):
        """Return True if this mode is in the Sync extraModes list."""
        try:
            modes = self._read("Sync", "extraModes", [])
            if isinstance(modes, str):
                modes = [modes] if modes else []
            return "annotate_toolbar_mode" in modes
        except Exception:
            return False

    def _toggle_sync_auto_start(self):
        """Add or remove this mode from the Sync extraModes list."""
        try:
            modes = self._read("Sync", "extraModes", [])
            if isinstance(modes, str):
                modes = [modes] if modes else []
            else:
                modes = list(modes)
            name = "annotate_toolbar_mode"
            if name in modes:
                modes.remove(name)
                self._sync_auto_start = False
            else:
                modes.append(name)
                self._sync_auto_start = True
            commands.writeSettings("Sync", "extraModes", modes)
        except Exception as e:
            print(f"[annotate_toolbar] sync auto-start toggle error: {e}")

    # ------------------------------------------------------------------
    # Dock creation
    # ------------------------------------------------------------------

    def _create_dock(self):
        sw = qtutils.sessionWindow()
        self._dock = AnnotateToolbarDockWidget(sw)
        # Hide the native title bar while docked (we have our own Draw header).
        # Restore it when floating so the user can drag the window.
        self._dock.setTitleBarWidget(QtWidgets.QWidget())
        self._dock.topLevelChanged.connect(self._on_dock_top_level_changed)
        sw.addDockWidget(QtCore.Qt.RightDockWidgetArea, self._dock)
        self._dock.hide()  # Start hidden; user opens via Annotation menu
        sw.resizeDocks([self._dock], [115], QtCore.Qt.Horizontal)

        for existing in sw.findChildren(QtWidgets.QDockWidget):
            if existing is self._dock:
                continue
            if sw.dockWidgetArea(existing) == QtCore.Qt.RightDockWidgetArea:
                sw.tabifyDockWidget(existing, self._dock)
                break

        w = self._dock.toolbar_widget
        w.tool_changed.connect(self._on_tool_changed)
        w.colour_changed.connect(self._on_colour_changed)
        w.size_changed.connect(self._on_size_changed)
        w.opacity_changed.connect(self._on_opacity_changed)
        w.filled_changed.connect(self._on_filled_changed)
        w.font_family_changed.connect(self._on_font_family_changed)
        w.font_size_changed.connect(self._on_font_size_changed)
        w.font_bold_changed.connect(self._on_font_bold_changed)
        w.font_italic_changed.connect(self._on_font_italic_changed)
        w.font_underline_changed.connect(self._on_font_underline_changed)
        w.undo_requested.connect(self._on_undo)
        w.redo_requested.connect(self._on_redo)
        w.clear_requested.connect(self._on_clear)
        w.clear_all_requested.connect(self._on_clear_all)
        w.close_requested.connect(self._on_close_requested)
        w.dock_requested.connect(self._on_dock_requested)
        w.color_modifier_changed.connect(self._on_color_modifier_changed)
        w.eraser_brush_changed.connect(self._on_eraser_brush_changed)

        # Load persisted settings after all signals are wired so the widget
        # updates (set_colour/set_size/set_opacity) don't emit change signals
        # back into the mode before it's fully ready.
        self._load_settings()

    # ------------------------------------------------------------------
    # Signal handlers
    # ------------------------------------------------------------------

    def _on_tool_changed(self, tool):
        # Always commit text when leaving the text tool, regardless of where we go.
        if self._tool == TOOL_TEXT:
            self._engine.commit_text_if_active()

        # Save current state for the outgoing tool.
        self._tool_colours[self._tool] = QtGui.QColor(self._colour)
        self._tool_sizes[self._tool] = self._size
        self._tool_opacities[self._tool] = self._opacity
        self._tool_color_modifiers[self._tool] = self._color_modifier
        self._save_tool_state(self._tool)

        outgoing_tool = self._tool
        self._tool = tool
        commands.writeSettings(_SETTINGS_GROUP, "active_tool", tool)

        # If we just came from the eyedropper, propagate the sampled colour to
        # the incoming tool instead of restoring its previous colour.
        if outgoing_tool == TOOL_EYEDROPPER:
            self._tool_colours[tool] = QtGui.QColor(self._colour)
            self._save_tool_state(tool)
        else:
            # Restore colour for the incoming tool.
            restored_colour = self._tool_colours.get(tool, QtGui.QColor(self._colour))
            if restored_colour != self._colour:
                self._colour = QtGui.QColor(restored_colour)
                self._dock.toolbar_widget.set_colour(restored_colour)

        # Restore size/opacity for the incoming tool.
        restored_size = self._tool_sizes.get(tool, self._size)
        restored_opacity = self._tool_opacities.get(tool, self._opacity)
        if restored_size != self._size or restored_opacity != self._opacity:
            self._size = restored_size
            self._opacity = restored_opacity
            self._dock.toolbar_widget.set_size(restored_size)
            self._dock.toolbar_widget.set_opacity(restored_opacity)

        restored_blend = self._tool_color_modifiers.get(tool, "normal")
        if restored_blend != self._color_modifier:
            self._color_modifier = restored_blend
            self._dock.toolbar_widget.panel.set_color_modifier(restored_blend)

        if tool in _DRAWING_TOOLS:
            self._push_shape_table()
        else:
            self._pop_shape_table()

        # Eyedropper: crosshair cursor over the viewport while active.
        sw = qtutils.sessionWindow()
        if tool == TOOL_EYEDROPPER:
            sw.setCursor(QtCore.Qt.CrossCursor)
        else:
            sw.unsetCursor()

    def _on_colour_changed(self, colour):
        self._colour = colour
        if self._link_tool_colors:
            for tool in _ALL_TOOLS:
                self._tool_colours[tool] = QtGui.QColor(colour)
                self._save_tool_state(tool)
        else:
            self._tool_colours[self._tool] = QtGui.QColor(colour)
            self._save_tool_state(self._tool)

    def _on_size_changed(self, v):
        self._size = v
        self._tool_sizes[self._tool] = v
        self._save_tool_state(self._tool)

    def _on_opacity_changed(self, v):
        self._opacity = v
        self._tool_opacities[self._tool] = v
        self._save_tool_state(self._tool)

    def _on_eraser_brush_changed(self, brush):
        self._eraser_brush = brush
        try:
            commands.writeSettings(_SETTINGS_GROUP, "eraser_brush", brush)
        except Exception as e:
            print(f"[annotate_toolbar] settings write error: {e}")

    def _on_filled_changed(self, v):
        self._filled = v

    def _on_font_family_changed(self, v):
        self._engine.commit_text_if_active()
        self._font_family = v
        commands.writeSettings(_SETTINGS_GROUP, "font_family", v)

    def _on_font_size_changed(self, v):
        self._engine.commit_text_if_active()
        self._font_size = v
        commands.writeSettings(_SETTINGS_GROUP, "font_size", v)

    def _on_font_bold_changed(self, v):
        self._engine.commit_text_if_active()
        self._font_bold = v
        commands.writeSettings(_SETTINGS_GROUP, "font_bold", v)

    def _on_font_italic_changed(self, v):
        self._engine.commit_text_if_active()
        self._font_italic = v
        commands.writeSettings(_SETTINGS_GROUP, "font_italic", v)

    def _on_font_underline_changed(self, v):
        self._engine.commit_text_if_active()
        self._font_underline = v
        commands.writeSettings(_SETTINGS_GROUP, "font_underline", v)

    def _on_color_modifier_changed(self, mode):
        self._color_modifier = mode
        self._tool_color_modifiers[self._tool] = mode
        self._save_tool_state(self._tool)

    def _on_undo(self):
        self._engine.undo()
        self._update_undo_redo_buttons()

    def _on_redo(self):
        self._engine.redo()
        self._update_undo_redo_buttons()

    def _on_undo_event(self, event):
        if not self._dock or not self._dock.isVisible():
            event.reject()
            return
        self._engine.undo()
        self._update_undo_redo_buttons()

    def _on_redo_event(self, event):
        if not self._dock or not self._dock.isVisible():
            event.reject()
            return
        self._engine.redo()
        self._update_undo_redo_buttons()

    def _on_clear(self):
        self._engine.clear_frame()
        self._update_undo_redo_buttons()

    def _on_clear_all(self):
        self._engine.clear_all_frames()
        self._update_undo_redo_buttons()

    def _on_dock_top_level_changed(self, floating):
        """Restore the native title bar when floating (enables dragging); hide it when docked."""
        if floating:
            self._dock.setTitleBarWidget(None)
        else:
            self._dock.setTitleBarWidget(QtWidgets.QWidget())

    def _find_old_draw_dock(self):
        """Return the old annotate_mode 'Draw' QDockWidget, or None if not present."""
        sw = qtutils.sessionWindow()
        for dock in sw.findChildren(QtWidgets.QDockWidget):
            try:
                if dock is not self._dock and dock.windowTitle() == "Draw":
                    return dock
            except RuntimeError:
                continue
        return None

    def _show_new_toolbar(self):
        """Show the new toolbar and hide the old Draw dock (mutual exclusion)."""
        old_dock = self._find_old_draw_dock()
        if old_dock:
            try:
                self._old_draw_dock_was_visible = old_dock.isVisible()
                old_dock.hide()
            except RuntimeError:
                old_dock = None
        self._user_closed = False
        self._dock.show()
        self._dock.raise_()

    def _hide_new_toolbar(self):
        """Hide the new toolbar and restore the old Draw dock if it was visible."""
        self._user_closed = True
        self._dock.hide()
        if self._old_draw_dock_was_visible:
            old_dock = self._find_old_draw_dock()
            if old_dock:
                try:
                    old_dock.show()
                except RuntimeError:
                    pass
        self._old_draw_dock_was_visible = False

    def _on_close_requested(self):
        self._hide_new_toolbar()

    def _on_dock_requested(self):
        self._user_closed = False
        self._dock.setFloating(not self._dock.isFloating())

    def _hide_colour_picker(self):
        if self._dock:
            self._dock.toolbar_widget.hide_popups()

    def _update_undo_redo_buttons(self):
        if self._dock:
            w = self._dock.toolbar_widget
            w.set_undo_enabled(self._engine.has_undo())
            w.set_redo_enabled(self._engine.has_redo())

    def _update_tool_availability(self):
        """Enable/disable tools whose RV event categories are currently disabled.

        Live Review uses commands.disableEventCategory() to prevent webclient
        users from accessing tools not supported over the web protocol.
        Also hides the dock entirely when annotate_category is disabled (viewer role).
        """
        if not self._dock:
            return
        w = self._dock.toolbar_widget

        # Hide/show the toolbar based on Live Review role permissions.
        # Only restore visibility when permission was previously revoked by Live Review
        # (_hidden_by_lr=True); do NOT auto-show on unrelated category-state events
        # (e.g., menu-bar clicks) which would pop the toolbar open unexpectedly.
        if commands.isEventCategoryEnabled("annotate_category"):
            if self._hidden_by_lr and not self._user_closed:
                self._show_new_toolbar()
            self._hidden_by_lr = False
        else:
            if self._dock.isVisible():
                self._hidden_by_lr = True
            self._dock.hide()

        airbrush_on = commands.isEventCategoryEnabled("annotate_airbrush_category")
        burn_on = commands.isEventCategoryEnabled("annotate_burn_category")
        dodge_on = commands.isEventCategoryEnabled("annotate_dodge_category")
        soft_erase_on = commands.isEventCategoryEnabled("annotate_softerase_category")
        hard_erase_on = commands.isEventCategoryEnabled("annotate_harderase_category")
        text_on = commands.isEventCategoryEnabled("annotate_text_category")
        sample_on = commands.isEventCategoryEnabled("annotate_sample_category")

        w.set_tool_enabled(TOOL_AIRBRUSH, airbrush_on)
        w.set_blend_mode_enabled(COLOR_MOD_DARKEN, burn_on)
        w.set_blend_mode_enabled(COLOR_MOD_ADDITIVE, dodge_on)
        w.set_soft_erase_enabled(soft_erase_on)
        # Disable the eraser tool entirely only when both erase modes are unavailable.
        w.set_tool_enabled(TOOL_ERASER, hard_erase_on or soft_erase_on)
        w.set_tool_enabled(TOOL_TEXT, text_on)
        w.set_tool_enabled(TOOL_EYEDROPPER, sample_on)

    def _on_category_state_changed(self, event):
        self._update_tool_availability()
        event.reject()

    def _on_undo_redo_clear_update(self, event):
        """Fired by live_review's paint manager after processing an incoming PAINT_BATCH_UPDATE.

        A remote client's undo/redo/clear changes the annotation state on the current
        frame, so our undo/redo button enabled state may be stale.
        """
        self._update_undo_redo_buttons()
        event.reject()

    def _on_set_current_annotate_node(self, event):
        """Receive the paint node that live_review wants annotations stored on.

        Live Review fires this event (via on_annotate_mode_activated) to direct
        us to draw on the annotation source group's RVPaint node rather than the
        local pipeline node, so that it can track and sync all annotations.

        Matches legacy annotate_mode.mu's setCurrentNodeEvent: only accept the
        node if it's actually part of the current view's paint nodes, otherwise
        clear the override rather than storing a name that may never resolve.
        """
        node_name = event.contents() or ""
        self._preferred_paint_node = ""
        if node_name:
            try:
                infos = commands.metaEvaluate(commands.frame(), commands.viewNode())
                if any(i.get("nodeType") == "RVPaint" and i.get("node") == node_name for i in infos):
                    self._preferred_paint_node = node_name
            except Exception:
                pass
        event.reject()

    # ------------------------------------------------------------------
    # Configure menu handlers
    # ------------------------------------------------------------------

    def _cfg_toggle_store_on_src(self, e):
        self._store_on_src = not self._store_on_src

    def _cfg_state_store_on_src(self):
        return commands.CheckedMenuState if self._store_on_src else commands.NeutralMenuState

    def _cfg_toggle_auto_mark(self, e):
        self._auto_mark = not self._auto_mark

    def _cfg_state_auto_mark(self):
        return commands.CheckedMenuState if self._auto_mark else commands.NeutralMenuState

    def _cfg_toggle_link_colors(self, e):
        self._link_tool_colors = not self._link_tool_colors
        if self._link_tool_colors:
            for tool in _ALL_TOOLS:
                self._tool_colours[tool] = QtGui.QColor(self._colour)
                self._save_tool_state(tool)

    def _cfg_state_link_colors(self):
        return commands.CheckedMenuState if self._link_tool_colors else commands.NeutralMenuState

    def _cfg_toggle_live_drawing(self, e):
        self._sync_whole_strokes = not self._sync_whole_strokes

    def _cfg_state_live_drawing(self):
        return commands.CheckedMenuState if not self._sync_whole_strokes else commands.NeutralMenuState

    def _cfg_toggle_sync_auto_start(self, e):
        self._toggle_sync_auto_start()

    def _cfg_state_sync_auto_start(self):
        return commands.CheckedMenuState if self._sync_auto_start else commands.NeutralMenuState

    def _cfg_toggle_scale_brush(self, e):
        self._scale_brush = not self._scale_brush

    def _cfg_state_scale_brush(self):
        return commands.CheckedMenuState if self._scale_brush else commands.NeutralMenuState

    def _cfg_toggle_auto_save(self, e):
        self._auto_save_settings = not self._auto_save_settings

    def _cfg_state_auto_save(self):
        return commands.CheckedMenuState if self._auto_save_settings else commands.NeutralMenuState

    # ------------------------------------------------------------------
    # Bindings and menu
    # ------------------------------------------------------------------

    @property
    def global_bindings(self):
        return [
            ("pointer-1--push", self._on_eyedropper_click, "Eyedropper sample"),
            ("stylus-pen--push", self._on_eyedropper_click, "Eyedropper sample (stylus)"),
            ("key-down--control--z", self._on_undo_event, "Undo"),
            ("key-down--control--y", self._on_redo_event, "Redo"),
            ("key-down--meta--z", self._on_undo_event, "Undo (mac)"),
            ("key-down--meta--shift--z", self._on_redo_event, "Redo (mac)"),
            ("event-category-state-changed", self._on_category_state_changed, "Update tool availability"),
            ("set-current-annotate-mode-node", self._on_set_current_annotate_node, "Set preferred paint node"),
            (
                "undo-redo-clear-ui-update",
                self._on_undo_redo_clear_update,
                "Sync undo/redo state after remote paint update",
            ),
            # Matches the F10 binding the legacy annotate_mode package used to show/hide
            # its "Draw" dock, so users don't have to learn a new hotkey.
            ("key-down--f10", self._toggle_toolbar, "Toggle Annotate Toolbar"),
            # Sent by the "Toggle Annotation tools" button in the bottom view toolbar
            # (RvBottomViewToolBar::paintActionTriggered).
            ("toggle-annotate-toolbar", self._toggle_toolbar, "Toggle Annotate Toolbar (bottom bar button)"),
            # Matches the legacy annotate_mode package's Next/Previous Annotated Frame hotkeys.
            # (RV joins simultaneous modifiers with a single dash, e.g. "alt-shift", and
            # brackets the modifier block with double dashes -- see QTTranslator::modifierString.)
            ("key-down--alt-shift--right", self._next_annotated_frame, "Next Annotated Frame"),
            ("key-down--alt-shift--left", self._prev_annotated_frame, "Previous Annotated Frame"),
        ]

    def _on_eyedropper_click(self, event):
        if self._tool != TOOL_EYEDROPPER:
            event.reject()
            return
        try:
            raw = event.pointer()
            dpr = commands.devicePixelRatio()
            x = raw[0] * dpr
            y = raw[1] * dpr
            colour = commands.framebufferPixelValue(x, y)
            if colour and len(colour) >= 3:
                qcol = QtGui.QColor.fromRgbF(
                    min(1.0, max(0.0, colour[0])),
                    min(1.0, max(0.0, colour[1])),
                    min(1.0, max(0.0, colour[2])),
                )
                self._colour = qcol
                self._tool_colours[self._tool] = QtGui.QColor(qcol)
                self._dock.toolbar_widget.set_colour(qcol)
                self._save_tool_state(self._tool)
        except Exception as e:
            print(f"[annotate_toolbar] eyedropper error: {e}")

    def _toggle_toolbar(self, event=None):
        if not self._dock:
            return
        if self._dock.isVisible():
            self._hide_new_toolbar()
        else:
            self._show_new_toolbar()

    def _toolbar_state(self):
        if self._dock and self._dock.isVisible():
            return commands.CheckedMenuState
        return commands.NeutralMenuState

    # ------------------------------------------------------------------
    # Next/Previous Annotated Frame navigation
    # ------------------------------------------------------------------
    # Ported from the legacy annotate_mode.mu nextAnnotatedFrame/prevAnnotatedFrame:
    # findAnnotatedFrames() returns an unsorted, possibly-duplicated frame list, so
    # scan the whole array for the closest next/previous frame rather than sorting.

    def _next_annotated_frame(self, event=None):
        frames = extra_commands.findAnnotatedFrames()
        if not frames:
            return
        current = commands.frame()
        new_frame = frames[0]
        for f in frames:
            if new_frame <= current or (f > current and f < new_frame):
                new_frame = f
        commands.setFrame(new_frame)

    def _prev_annotated_frame(self, event=None):
        frames = extra_commands.findAnnotatedFrames()
        if not frames:
            return
        current = commands.frame()
        new_frame = frames[0]
        for f in frames:
            if f < current and (new_frame >= current or f > new_frame):
                new_frame = f
        commands.setFrame(new_frame)

    def _next_prev_state(self):
        return commands.DisabledMenuState if extra_commands.isSessionEmpty() else commands.UncheckedMenuState

    @property
    def menu(self):
        configure_items = [
            ("Draw On Source When Possible", self._cfg_toggle_store_on_src, None, self._cfg_state_store_on_src),
            ("Automatically Mark Annotated Frames", self._cfg_toggle_auto_mark, None, self._cfg_state_auto_mark),
            ("Link Tool Colors", self._cfg_toggle_link_colors, None, self._cfg_state_link_colors),
            ("Live Drawing in Sync", self._cfg_toggle_live_drawing, None, self._cfg_state_live_drawing),
            (
                "Start Automatically During Sync",
                self._cfg_toggle_sync_auto_start,
                None,
                self._cfg_state_sync_auto_start,
            ),
            ("Brush Size Relative to View", self._cfg_toggle_scale_brush, None, self._cfg_state_scale_brush),
            ("Always Save Settings as Defaults On Exit", self._cfg_toggle_auto_save, None, self._cfg_state_auto_save),
        ]

        return [
            (
                "Tools",
                [
                    # Same menu path and hotkey as the legacy annotate_mode package, so
                    # switching to the new toolbar doesn't require re-training muscle memory.
                    ("Annotation", self._toggle_toolbar, "F10", self._toolbar_state),
                ],
            ),
            (
                "Annotation",
                [
                    (
                        "Next Annotated Frame",
                        self._next_annotated_frame,
                        "alt shift rightArrow",
                        self._next_prev_state,
                    ),
                    (
                        "Previous Annotated Frame",
                        self._prev_annotated_frame,
                        "alt shift leftArrow",
                        self._next_prev_state,
                    ),
                    ("Configure", configure_items),
                ],
            ),
        ]


def createMode():
    return AnnotateToolbarMode()
