# Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import math
import os
import uuid

from rv import commands

from annotate_toolbar_widget import (
    TOOL_PEN,
    TOOL_AIRBRUSH,
    TOOL_ERASER,
    TOOL_RECT,
    TOOL_CIRCLE,
    TOOL_ARROW,
    TOOL_LINE,
    TOOL_TEXT,
)

_SHAPE_TOOLS = {TOOL_RECT, TOOL_CIRCLE, TOOL_ARROW, TOOL_LINE}
_DRAWING_TOOLS = _SHAPE_TOOLS | {TOOL_TEXT, TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER}

TABLE_NAME = "annotate_toolbar_shape"

_PREFIX = {
    TOOL_RECT: "rect",
    TOOL_CIRCLE: "ellipse",
    TOOL_ARROW: "arrow",
    TOOL_LINE: "line",
}

_SIZE_SCALE = 1.0 / 10000.0

# Pen/eraser width range matches the Mu annotate_mode penDrawMode (minSize/maxSize).
# Slider 1→100 maps linearly to 0.001→0.024 in normalized image-space units.
_PEN_WIDTH_MIN = 0.001
_PEN_WIDTH_MAX = 0.024
_PEN_SLIDER_MIN = 1
_PEN_SLIDER_MAX = 100

_SIZE_MIN = 0.001

# Base WCS fractions for each font size tier (desired px at zoom=1 / 1080).
# Multiplied by _screen_scale() at draw time — identical to how stroke
# border_width uses _SIZE_SCALE * _screen_scale(). This gives:
#   - constant initial screen size regardless of zoom (draw-time compensation)
#   - existing text scales with the image as you zoom in (fixed WCS stored)
_FONT_SIZE_WCS_BASE = {"small": 24.0 / 1080.0, "medium": 48.0 / 1080.0, "large": 72.0 / 1080.0}

# US keyboard shift-symbol mapping used for text input
_SHIFT_MAP = {
    "1": "!",
    "2": "@",
    "3": "#",
    "4": "$",
    "5": "%",
    "6": "^",
    "7": "&",
    "8": "*",
    "9": "(",
    "0": ")",
    "-": "_",
    "=": "+",
    "[": "{",
    "]": "}",
    "\\": "|",
    ";": ":",
    "'": '"',
    ",": "<",
    ".": ">",
    "/": "?",
    "`": "~",
}


class AnnotateDrawEngine:
    def __init__(self, mode):
        self._mode = mode

        # Shape drag state
        self._anchor = None
        self._last_pei = None
        self._current_shape = None
        self._current_node = None
        self._current_frame = None
        self._shape_type = None
        self._shape_active = False
        self._shift_transition = False
        self._constraint_angle = None

        # Text editing state
        self._text_active = False
        self._text_buffer = ""
        self._text_node = None  # full property path — None until first char typed
        self._text_anchor = None  # _Vec2 position recorded on pointer-push
        self._text_paint_node = None
        self._text_frame = None

        # Pen/eraser stroke state
        self._pen_stroke = None  # current stroke node name (set on push, cleared on release)
        self._pen_paint_node = None
        self._pen_frame = None
        self._pen_stroke_width = 0.0  # constant width for the active stroke, used per-drag insert
        # True while the physical eraser end of a Wacom stylus is in use; forces erase
        # mode regardless of the selected tool.
        self._stylus_erasing = False

        # Source image node name captured on each pointer-push event.
        # Used by _screen_scale() so widths stored in image space remain
        # screen-constant across different zoom levels and image aspect ratios.
        self._current_source_name = None

        # Undo/redo stacks — each entry: (paint_node, frame, node_name)
        # node_name is the full prop path prefix, e.g. "RVPaint_1.rect:3:42:host_123"
        self._undo_stack = []  # list of (paint_node, frame, node_name)
        self._redo_stack = []

    # ------------------------------------------------------------------
    # Event table setup
    # ------------------------------------------------------------------

    def setup_event_table(self, mode):
        mode.defineEventTable(TABLE_NAME, self.bindings)
        mode.defineEventTableRegex(TABLE_NAME, self.regex_bindings)

    @property
    def bindings(self):
        return [
            # Shape pointer events
            ("pointer-1--push", self.on_push, "Start shape/text"),
            ("pointer-1--drag", self.on_drag, "Update shape"),
            ("pointer-1--release", self.on_release, "Commit shape"),
            ("pointer-1--shift--push", self.on_push_shift, "Start shape (constrained)"),
            ("pointer-1--shift--drag", self.on_drag_shift, "Update shape (constrained)"),
            ("pointer-1--shift--release", self.on_release_shift, "Commit shape (constrained)"),
            ("stylus-pen--push", self.on_push, "Start (stylus)"),
            ("stylus-pen--drag", self.on_drag, "Update (stylus)"),
            ("stylus-pen--release", self.on_release, "Commit (stylus)"),
            ("stylus-eraser--push", self.on_stylus_eraser_push, "Start (stylus eraser end)"),
            ("stylus-eraser--drag", self.on_stylus_eraser_drag, "Draw (stylus eraser end)"),
            ("stylus-eraser--release", self.on_stylus_eraser_release, "Commit (stylus eraser end)"),
            # Shape shift-constraint tracking
            ("key-down--shift--shift", self.on_shift_down, "Constrain shape"),
            ("key-up--shift", self.on_shift_up, "Release constraint"),
            # Text editing — explicit keys
            ("key-down--backspace", self.on_text_backspace, "Delete char"),
            ("key-down--delete", self.on_text_backspace, "Delete char"),
            ("key-down--space", self.on_text_space, "Insert space"),
            ("key-down--return", self.on_text_commit, "Commit text"),
            ("key-down--enter", self.on_text_commit, "Commit text"),
            ("key-down--keypad-enter", self.on_text_commit, "Commit text"),
            ("key-down--escape", self.on_text_cancel, "Cancel text"),
            # Commit when frame changes so text isn't left open during playback
            ("frame-changed", self.on_frame_changed, "Commit text on frame change"),
        ]

    @property
    def regex_bindings(self):
        return [
            (r"^key-down--.$", self.on_text_key, "Insert char"),
            (r"^key-down--shift--.$", self.on_text_key, "Insert shifted char"),
            (r"^key-up--.$", self.on_key_up, "Key up"),
            (r"^key-up--shift--.$", self.on_key_up, "Key up (shift)"),
        ]

    # ------------------------------------------------------------------
    # Public helpers called by the mode
    # ------------------------------------------------------------------

    def update_text_style(self):
        """Update all font properties on the active text node.

        Called whenever the user changes family, size, bold, italic, or underline
        while a text node is being edited so changes apply immediately.
        """
        if not self._text_active or self._text_node is None:
            return
        try:
            font_size = _FONT_SIZE_WCS_BASE.get(self._mode._font_size, 48.0 / 1080.0) * self._screen_scale()
            font_weight = "bold" if self._mode._font_bold else "normal"
            font_style = "italic" if self._mode._font_italic else "normal"
            text_deco = "underline" if self._mode._font_underline else "none"
            commands.setStringProperty(f"{self._text_node}.fontFamily", [self._mode._font_family], True)
            commands.setFloatProperty(f"{self._text_node}.fontSize", [font_size], True)
            commands.setStringProperty(f"{self._text_node}.fontWeight", [font_weight], True)
            commands.setStringProperty(f"{self._text_node}.fontStyle", [font_style], True)
            commands.setStringProperty(f"{self._text_node}.textDecoration", [text_deco], True)
            commands.redraw()
        except Exception as e:
            print(f"[annotate_toolbar] update_text_style error: {e}")

    def commit_text_if_active(self):
        if self._text_active:
            self._commit_text()

    # ------------------------------------------------------------------
    # State helpers
    # ------------------------------------------------------------------

    def _is_shape_tool(self):
        return self._mode._tool in _SHAPE_TOOLS

    def _screen_scale(self):
        """Scale factor that makes image-space widths screen-constant.

        At zoom=1 (fit-to-window) for a 16:9 image returns 1.0.
        At zoom=2 returns 0.5 — stored width halved so the rendered screen
        width stays the same as at zoom=1.
        Also accounts for image aspect ratio: a letterboxed 2.39:1 image has
        a smaller proj_scale than 16:9, so this factor compensates to give the
        same screen-pixel width for any image format.
        """
        name = self._current_source_name
        if not name:
            return 1.0
        try:
            view_h = commands.viewSize()[1]
            p0 = commands.imageToEventSpace(name, (0.0, 0.0), True)
            p1 = commands.imageToEventSpace(name, (0.0, 0.01), True)
            px_per_unit = abs(p1[1] - p0[1]) / 0.01
            return view_h / max(px_per_unit, 0.1)
        except Exception:
            return 1.0

    def _border_width(self):
        scale = self._screen_scale() if getattr(self._mode, "_scale_brush", True) else 1.0
        return max(_SIZE_MIN, self._mode._size * _SIZE_SCALE * scale)

    def _colors(self):
        c = self._mode._colour
        alpha = self._mode._opacity / 100.0
        border = [c.redF(), c.greenF(), c.blueF(), alpha]
        tool = self._mode._tool
        if tool == TOOL_ARROW:
            inner = list(border)
        elif self._mode._filled and tool in (TOOL_RECT, TOOL_CIRCLE):
            inner = list(border)
        else:
            inner = [c.redF(), c.greenF(), c.blueF(), 0.0]
        return border, inner

    # ------------------------------------------------------------------
    # Paint node resolution
    # ------------------------------------------------------------------

    def _find_paint_node(self):
        try:
            frame = commands.frame()
            infos = commands.metaEvaluate(frame, commands.viewNode())

            # If live_review has nominated a specific paint node (via
            # set-current-annotate-mode-node), prefer it so that RV-drawn
            # annotations land on the annotation source group's node — the same
            # one live_review tracks — rather than the local pipeline node.
            preferred = getattr(self._mode, "_preferred_paint_node", "")
            if preferred:
                for info in infos:
                    if info.get("node") == preferred:
                        return preferred, info["frame"]
                # Preferred node isn't part of the current view (e.g. it was set
                # while viewing a different view/layout) — fall through to normal
                # resolution below rather than trusting a stale override. Matches
                # legacy annotate_mode.mu's updateCurrentNode(), which never uses
                # _userSelectedNode unless it's found in the current metaEvaluate.

            store_on_src = getattr(self._mode, "_store_on_src", False)
            if store_on_src:
                for info in infos:
                    if info.get("nodeType") == "RVPaint":
                        node = info["node"]
                        try:
                            grp = commands.nodeGroup(node)
                            if grp and commands.nodeType(grp) == "RVSourceGroup":
                                return node, info["frame"]
                        except Exception:
                            pass

            for info in infos:
                if info.get("nodeType") == "RVPaint":
                    return info["node"], info["frame"]
            all_paint = commands.nodesOfType("RVPaint")
            if all_paint:
                return all_paint[0], frame
            return None, None
        except Exception:
            return None, None

    def _next_id(self, paint_node):
        prop = f"{paint_node}.paint.nextId"
        if not commands.propertyExists(prop):
            commands.newProperty(prop, commands.IntType, 1)
            commands.setIntProperty(prop, [0])
        i = commands.getIntProperty(prop)[0] + 1
        commands.setIntProperty(prop, [i])
        return i

    def _ensure_visible(self, paint_node):
        prop = f"{paint_node}.paint.show"
        if not commands.propertyExists(prop):
            commands.newProperty(prop, commands.IntType, 1)
        commands.setIntProperty(prop, [1], True)

    def _unique_name(self, paint_node, prefix, frame):
        node_id = self._next_id(paint_node)
        host = commands.myNetworkHost().replace(".", "_")
        pid = os.getpid()
        return f"{paint_node}.{prefix}:{node_id}:{frame}:{host}_{pid}"

    def _frame_order_and_undo(self, paint_node, frame, node_name, shape_uuid):
        """Insert the component into the frame draw order and undo stack."""

        def _ensure(prop, ptype, w):
            if not commands.propertyExists(prop):
                commands.newProperty(prop, ptype, w)

        component = node_name.split(".")[-1]
        order_prop = f"{paint_node}.frame:{frame}.order"
        _ensure(order_prop, commands.StringType, 1)
        if component not in commands.getStringProperty(order_prop):
            commands.insertStringProperty(order_prop, [component])

        host = commands.myNetworkHost().replace(".", "_")
        pid = os.getpid()
        undo_prop = f"{paint_node}.frame:{frame}.userUndoStack:{host}_{pid}"
        _ensure(undo_prop, commands.StringType, 1)
        commands.insertStringProperty(undo_prop, [shape_uuid, "create"])

        if getattr(self._mode, "_auto_mark", False):
            try:
                commands.markFrame(frame, True)
            except Exception:
                pass

    # ------------------------------------------------------------------
    # Pointer / coordinate helpers
    # ------------------------------------------------------------------

    def _pointer_location(self, event):
        """Return (image_name, _Vec2) in image space, or ("", None).

        imagesAtPixel returns nodes outermost→innermost (sorted by imageNum descending):
          [displayGroup_colorPipeline, defaultSequence_sequence, sourceGroup_source]
        The display pipeline entry has a zoom-dependent transform that diverges from
        PaintIPNode's source-image coordinate space after zoom/pan.
        We iterate from innermost (last) to outermost and use the first entry for which
        eventToImageSpace succeeds, because when viewing composite layouts (default stack,
        default sequence) the innermost entry may be a virtual composite image whose
        source name is not valid for eventToImageSpace.
        """
        try:
            raw = event.pointer()
            dpr = commands.devicePixelRatio()
            ip = (raw[0] * dpr, raw[1] * dpr)

            pinfos = commands.imagesAtPixel(raw)
            if not pinfos:
                return "", None

            for info in reversed(pinfos):
                name = info.get("name", "")
                if not name:
                    continue
                try:
                    pei_raw = commands.eventToImageSpace(name, ip, True)
                    return name, _Vec2(pei_raw[0], pei_raw[1])
                except Exception:
                    continue

            return "", None
        except Exception as e:
            import traceback

            print(f"[annotate_toolbar] _pointer_location error: {e}")
            traceback.print_exc()
            return "", None

    # ------------------------------------------------------------------
    # Shape node creation / update
    # ------------------------------------------------------------------

    def _new_shape(self, paint_node, frame, prefix, anchor, cur):
        try:
            self._ensure_visible(paint_node)
            n = self._unique_name(paint_node, prefix, frame)
            bw = self._border_width()
            border, inner = self._colors()
            shape_uuid = str(uuid.uuid4())

            def _ensure(prop, ptype, w):
                if not commands.propertyExists(prop):
                    commands.newProperty(prop, ptype, w)

            _ensure(f"{n}.startFrame", commands.IntType, 1)
            _ensure(f"{n}.duration", commands.IntType, 1)
            _ensure(f"{n}.eye", commands.IntType, 1)
            commands.setIntProperty(f"{n}.startFrame", [frame], True)
            commands.setIntProperty(f"{n}.duration", [1], True)
            commands.setIntProperty(f"{n}.eye", [2], True)

            if prefix in ("rect", "ellipse"):
                for prop, w in ((".min", 2), (".max", 2), (".innerColor", 4), (".borderColor", 4), (".borderWidth", 1)):
                    _ensure(f"{n}{prop}", commands.FloatType, w)
                min_x = min(anchor.x, cur.x)
                min_y = min(anchor.y, cur.y)
                max_x = max(anchor.x, cur.x)
                max_y = max(anchor.y, cur.y)
                commands.setFloatProperty(f"{n}.min", [min_x, min_y], True)
                commands.setFloatProperty(f"{n}.max", [max_x, max_y], True)
                commands.setFloatProperty(f"{n}.innerColor", inner, True)
                commands.setFloatProperty(f"{n}.borderColor", border, True)
                commands.setFloatProperty(f"{n}.borderWidth", [bw], True)
            else:
                for prop, w in ((".startPos", 2), (".endPos", 2), (".borderColor", 4), (".borderWidth", 1)):
                    _ensure(f"{n}{prop}", commands.FloatType, w)
                commands.setFloatProperty(f"{n}.startPos", [anchor.x, anchor.y], True)
                commands.setFloatProperty(f"{n}.endPos", [cur.x, cur.y], True)
                commands.setFloatProperty(f"{n}.borderColor", border, True)
                commands.setFloatProperty(f"{n}.borderWidth", [bw], True)
                if prefix == "arrow":
                    _ensure(f"{n}.innerColor", commands.FloatType, 4)
                    _ensure(f"{n}.thickness", commands.FloatType, 1)
                    commands.setFloatProperty(f"{n}.innerColor", inner, True)
                    commands.setFloatProperty(f"{n}.thickness", [bw], True)

            _ensure(f"{n}.uuid", commands.StringType, 1)
            _ensure(f"{n}.softDeleted", commands.IntType, 1)
            commands.setStringProperty(f"{n}.uuid", [shape_uuid], True)
            commands.setIntProperty(f"{n}.softDeleted", [0], True)

            self._frame_order_and_undo(paint_node, frame, n, shape_uuid)

            commands.redraw()
            return n
        except Exception as e:
            import traceback

            print(f"[annotate_toolbar] _new_shape error: {e}")
            traceback.print_exc()
            return None

    def _update_shape(self, shape_node, prefix, anchor, cur):
        if shape_node is None:
            return
        try:
            if prefix in ("rect", "ellipse"):
                commands.setFloatProperty(f"{shape_node}.min", [min(anchor.x, cur.x), min(anchor.y, cur.y)], True)
                commands.setFloatProperty(f"{shape_node}.max", [max(anchor.x, cur.x), max(anchor.y, cur.y)], True)
            else:
                commands.setFloatProperty(f"{shape_node}.endPos", [cur.x, cur.y], True)
            commands.redraw()
        except Exception as e:
            print(f"[annotate_toolbar] _update_shape error: {e}")

    # ------------------------------------------------------------------
    # Text node creation / update
    # ------------------------------------------------------------------

    def _new_text_node(self, paint_node, frame, pos):
        """Create an empty text node at pos and return its property path."""
        self._begin_sync()
        try:
            self._ensure_visible(paint_node)
            n = self._unique_name(paint_node, "text", frame)
            shape_uuid = str(uuid.uuid4())

            c = self._mode._colour
            color = [c.redF(), c.greenF(), c.blueF(), 1.0]

            font_size = _FONT_SIZE_WCS_BASE.get(self._mode._font_size, 48.0 / 1080.0) * self._screen_scale()
            font_weight = "bold" if self._mode._font_bold else "normal"
            font_style = "italic" if self._mode._font_italic else "normal"
            text_deco = "underline" if self._mode._font_underline else "none"

            def _ensure(prop, ptype, w):
                if not commands.propertyExists(prop):
                    commands.newProperty(prop, ptype, w)

            for prop, ptype, w in (
                (".position", commands.FloatType, 2),
                (".color", commands.FloatType, 4),
                (".size", commands.FloatType, 1),
                (".scale", commands.FloatType, 1),
                (".rotation", commands.FloatType, 1),
                (".spacing", commands.FloatType, 1),
                (".font", commands.StringType, 1),
                (".text", commands.StringType, 1),
                (".origin", commands.StringType, 1),
                (".debug", commands.IntType, 1),
                (".startFrame", commands.IntType, 1),
                (".duration", commands.IntType, 1),
                (".mode", commands.IntType, 1),
            ):
                _ensure(f"{n}{prop}", ptype, w)

            commands.setFloatProperty(f"{n}.position", [pos.x, pos.y], True)
            commands.setFloatProperty(f"{n}.color", color, True)
            commands.setFloatProperty(f"{n}.size", [0.01], True)
            commands.setFloatProperty(f"{n}.scale", [1.0], True)
            commands.setFloatProperty(f"{n}.rotation", [0.0], True)
            commands.setFloatProperty(f"{n}.spacing", [0.8], True)
            commands.setStringProperty(f"{n}.font", [""], True)
            commands.setStringProperty(f"{n}.text", ["|"], True)
            commands.setStringProperty(f"{n}.origin", [""], True)
            commands.setIntProperty(f"{n}.debug", [0], True)
            commands.setIntProperty(f"{n}.startFrame", [frame], True)
            commands.setIntProperty(f"{n}.duration", [1], True)
            commands.setIntProperty(f"{n}.mode", [0], True)

            for prop, ptype, w in (
                (".fontFamily", commands.StringType, 1),
                (".fontSize", commands.FloatType, 1),
                (".fontWeight", commands.StringType, 1),
                (".fontStyle", commands.StringType, 1),
                (".textDecoration", commands.StringType, 1),
                (".textAlign", commands.StringType, 1),
            ):
                _ensure(f"{n}{prop}", ptype, w)

            commands.setStringProperty(f"{n}.fontFamily", [self._mode._font_family], True)
            commands.setFloatProperty(f"{n}.fontSize", [font_size], True)
            commands.setStringProperty(f"{n}.fontWeight", [font_weight], True)
            commands.setStringProperty(f"{n}.fontStyle", [font_style], True)
            commands.setStringProperty(f"{n}.textDecoration", [text_deco], True)
            commands.setStringProperty(f"{n}.textAlign", ["left"], True)

            _ensure(f"{n}.uuid", commands.StringType, 1)
            _ensure(f"{n}.softDeleted", commands.IntType, 1)
            commands.setStringProperty(f"{n}.uuid", [shape_uuid], True)
            commands.setIntProperty(f"{n}.softDeleted", [0], True)

            self._frame_order_and_undo(paint_node, frame, n, shape_uuid)
            commands.redraw()
            self._end_sync()
            return n
        except Exception as e:
            import traceback

            print(f"[annotate_toolbar] _new_text_node error: {e}")
            traceback.print_exc()
            self._end_sync()
            return None

    def _ensure_text_node(self):
        """Create the text node on first keypress if not yet created."""
        if self._text_node is not None:
            return
        if not self._text_paint_node or self._text_anchor is None:
            return
        node = self._new_text_node(self._text_paint_node, self._text_frame, self._text_anchor)
        if node:
            self._text_node = node
            self._undo_stack.append((self._text_paint_node, self._text_frame, node))
            self._redo_stack.clear()
            self._notify_buttons()

    def _update_text_display(self, cursor=True):
        if self._text_node is None:
            return
        display = self._text_buffer + "|" if cursor else self._text_buffer
        try:
            commands.setStringProperty(f"{self._text_node}.text", [display], True)
            commands.redraw()
        except Exception as e:
            print(f"[annotate_toolbar] _update_text_display error: {e}")

    def _commit_text(self):
        # Nothing typed — clean up the placeholder node rather than leaving an empty annotation
        if not self._text_buffer:
            self._cancel_text()
            return
        self._update_text_display(cursor=False)
        self._text_active = False
        self._text_node = None
        self._text_buffer = ""
        self._text_paint_node = None
        self._text_frame = None
        commands.sendInternalEvent("annotate-text-committed")

    def _cancel_text(self):
        if self._text_node:
            try:
                self._remove_from_order(self._text_paint_node, self._text_frame, self._text_node)
                commands.setIntProperty(f"{self._text_node}.softDeleted", [1], True)
                commands.redraw()
            except Exception:
                pass
        self._text_active = False
        self._text_node = None
        self._text_anchor = None
        self._text_buffer = ""
        self._text_paint_node = None
        self._text_frame = None

    # ------------------------------------------------------------------
    # Pen/eraser stroke node creation
    # ------------------------------------------------------------------

    def _pressure_width(self, event):
        """Return pen width scaled by stylus pressure; falls back to 1.0 for mouse."""
        try:
            p = max(0.01, min(1.0, event.pressure()))
        except Exception:
            p = 1.0
        return self._pen_stroke_width * p

    def _new_stroke(self, paint_node, frame, first_point, brush, erase_mode, first_point_width=None):
        """Create a new pen/eraser stroke component and return its property path."""
        try:
            self._ensure_visible(paint_node)
            n = self._unique_name(paint_node, "pen", frame)
            stroke_uuid = str(uuid.uuid4())

            c = self._mode._colour
            alpha = self._mode._opacity / 100.0
            blend_mode = getattr(self._mode, "_color_modifier", "normal")

            if blend_mode == "additive":
                s = 1.0 + alpha
                color = [c.redF() * s, c.greenF() * s, c.blueF() * s, alpha * alpha]
            elif blend_mode == "darken":
                s = (1.0 - alpha) * 0.75 + 0.25
                color = [c.redF() * s, c.greenF() * s, c.blueF() * s, alpha * alpha]
            else:
                color = [c.redF(), c.greenF(), c.blueF(), alpha]

            t = (self._mode._size - _PEN_SLIDER_MIN) / (_PEN_SLIDER_MAX - _PEN_SLIDER_MIN)
            scale = self._screen_scale() if getattr(self._mode, "_scale_brush", True) else 1.0
            width = (_PEN_WIDTH_MIN + t * (_PEN_WIDTH_MAX - _PEN_WIDTH_MIN)) * scale
            self._pen_stroke_width = width
            push_width = first_point_width if first_point_width is not None else width

            def _ensure(prop, ptype, w):
                if not commands.propertyExists(prop):
                    commands.newProperty(prop, ptype, w)

            for prop, ptype, w in (
                (".color", commands.FloatType, 4),
                (".width", commands.FloatType, 1),
                (".brush", commands.StringType, 1),
                (".uuid", commands.StringType, 1),
                (".points", commands.FloatType, 2),
                (".join", commands.IntType, 1),
                (".cap", commands.IntType, 1),
                (".splat", commands.IntType, 1),
                (".mode", commands.IntType, 1),
                (".debug", commands.IntType, 1),
                (".smoothingWidth", commands.FloatType, 1),
                (".startFrame", commands.IntType, 1),
                (".duration", commands.IntType, 1),
                # Stamp-brush properties (only take effect for brush names other than
                # "circle"/"gauss" — see PaintIPNode::compilePenComponent). Not yet
                # exposed in this UI; written here so future sliders/pickers have a
                # ready-made property to set.
                (".hardness", commands.FloatType, 1),
                (".tipTexture", commands.StringType, 1),
                (".blendMode", commands.IntType, 1),
            ):
                _ensure(f"{n}{prop}", ptype, w)

            # mode: 0=OverMode, 1=EraseMode, 2=ScaleMode (burn/dodge use ScaleMode)
            if erase_mode:
                stroke_mode = 1
            elif blend_mode in ("additive", "darken"):
                stroke_mode = 2  # ScaleMode
            else:
                stroke_mode = 0  # OverMode

            # All properties read by _get_paint_start must be written before .points,
            # because setting .points fires graph-state-change which immediately tries
            # to build the PAINT_START payload for the LiveReview package.
            _ensure(f"{n}.softDeleted", commands.IntType, 1)
            commands.setFloatProperty(f"{n}.color", color, True)
            commands.setFloatProperty(f"{n}.width", [push_width], True)
            commands.setStringProperty(f"{n}.brush", [brush], True)
            commands.setIntProperty(f"{n}.mode", [stroke_mode], True)
            commands.setIntProperty(f"{n}.startFrame", [frame], True)
            commands.setIntProperty(f"{n}.duration", [1], True)
            commands.setStringProperty(f"{n}.uuid", [stroke_uuid], True)
            commands.setIntProperty(f"{n}.softDeleted", [0], True)
            commands.setFloatProperty(f"{n}.points", [first_point.x, first_point.y], True)
            commands.setIntProperty(f"{n}.join", [1], True)  # RoundJoin
            commands.setIntProperty(f"{n}.cap", [2], True)  # RoundCap
            commands.setIntProperty(f"{n}.splat", [1 if brush == "gauss" else 0], True)
            commands.setIntProperty(f"{n}.debug", [0], True)
            commands.setFloatProperty(f"{n}.smoothingWidth", [1.0], True)
            commands.setFloatProperty(f"{n}.hardness", [100.0], True)
            commands.setStringProperty(f"{n}.tipTexture", [""], True)
            commands.setIntProperty(f"{n}.blendMode", [2 if blend_mode == "additive" else 0], True)

            self._frame_order_and_undo(paint_node, frame, n, stroke_uuid)
            commands.redraw()
            return n
        except Exception as e:
            import traceback as _tb

            print(f"[annotate_toolbar] _new_stroke error: {e}")
            _tb.print_exc()
            return None

    def _pen_push(self, event):
        if commands.isPlaying():
            commands.stop()
        self._begin_sync()
        paint_node, frame = self._find_paint_node()
        if paint_node is None:
            self._end_sync()
            event.reject()
            return
        name, pei = self._pointer_location(event)
        if not name:
            self._end_sync()
            event.reject()
            return
        self._current_source_name = name
        # Refresh cached stroke width so _pressure_width() uses the current
        # zoom's _screen_scale(), not the stale value from the previous stroke.
        t = (self._mode._size - _PEN_SLIDER_MIN) / (_PEN_SLIDER_MAX - _PEN_SLIDER_MIN)
        scale = self._screen_scale() if getattr(self._mode, "_scale_brush", True) else 1.0
        self._pen_stroke_width = (_PEN_WIDTH_MIN + t * (_PEN_WIDTH_MAX - _PEN_WIDTH_MIN)) * scale
        tool = self._mode._tool
        if self._stylus_erasing:
            erase = True
            brush = getattr(self._mode, "_eraser_brush", "circle")
        else:
            erase = tool == TOOL_ERASER
            if tool == TOOL_AIRBRUSH:
                brush = "gauss"
            elif tool == TOOL_ERASER:
                brush = getattr(self._mode, "_eraser_brush", "circle")
            else:
                brush = "circle"
        stroke = self._new_stroke(paint_node, frame, pei, brush, erase, self._pressure_width(event))
        if stroke:
            self._pen_stroke = stroke
            self._pen_paint_node = paint_node
            self._pen_frame = frame
            commands.sendInternalEvent("set-current-annotate-mode-node", paint_node)
            # sync accumulation is still open — will be flushed at _pen_release
        else:
            self._end_sync()

    def _pen_drag(self, event):
        if not self._pen_stroke:
            return
        name, pei = self._pointer_location(event)
        if not name:
            return
        try:
            commands.insertFloatProperty(f"{self._pen_stroke}.points", [pei.x, pei.y])
            commands.insertFloatProperty(f"{self._pen_stroke}.width", [self._pressure_width(event)])
            commands.redraw()
            if not getattr(self._mode, "_sync_whole_strokes", True):
                self._end_sync(force=True)
                self._begin_sync()
        except Exception as e:
            print(f"[annotate_toolbar] _pen_drag error: {e}")

    def _pen_release(self, event):
        if not self._pen_stroke:
            return
        name, pei = self._pointer_location(event)
        if name and pei:
            try:
                commands.insertFloatProperty(f"{self._pen_stroke}.points", [pei.x, pei.y])
                commands.insertFloatProperty(f"{self._pen_stroke}.width", [self._pressure_width(event)])
            except Exception:
                pass
        # Commit to undo stack now that stroke is complete
        self._undo_stack.append((self._pen_paint_node, self._pen_frame, self._pen_stroke))
        self._redo_stack.clear()
        self._notify_buttons()
        commands.sendInternalEvent("annotate-stroke-released")
        self._pen_stroke = None
        self._pen_paint_node = None
        self._pen_frame = None
        commands.redraw()
        # End the whole-stroke accumulation started in _pen_push and flush to network.
        self._end_sync(force=True)

    # ------------------------------------------------------------------
    # Shift constraint (shapes only)
    # ------------------------------------------------------------------

    def _constrain(self, prefix, anchor, cur):
        dx = cur.x - anchor.x
        dy = cur.y - anchor.y
        if prefix in ("rect", "ellipse"):
            if self._constraint_angle is not None:
                cx = math.cos(self._constraint_angle)
                cy = math.sin(self._constraint_angle)
                proj = max(dx * cx + dy * cy, 0.0)
                return _Vec2(anchor.x + proj * cx, anchor.y + proj * cy)
            side = min(abs(dx), abs(dy))
            return _Vec2(
                anchor.x + (side if dx >= 0 else -side),
                anchor.y + (side if dy >= 0 else -side),
            )
        else:
            length = math.sqrt(dx * dx + dy * dy)
            if length < 1e-5:
                return cur
            angle = math.atan2(dy, dx)
            snapped = round(angle / (math.pi / 4)) * (math.pi / 4)
            return _Vec2(
                anchor.x + length * math.cos(snapped),
                anchor.y + length * math.sin(snapped),
            )

    # ------------------------------------------------------------------
    # Common shape push / release
    # ------------------------------------------------------------------

    def _do_push(self, pei, paint_node, frame):
        if commands.isPlaying():
            commands.stop()
            commands.setFrame(frame)
        prefix = _PREFIX[self._mode._tool]
        self._anchor = pei
        self._last_pei = pei
        self._shape_type = prefix
        self._current_node = paint_node
        self._current_frame = frame
        self._shape_active = True
        self._shift_transition = False
        commands.sendInternalEvent("set-current-annotate-mode-node", paint_node)
        # Open a sync accumulation block that stays open until _do_release so the
        # entire shape (push → drag → release) is sent as one batch to Live Review
        # participants instead of immediately broadcasting the initial zero-size shape.
        self._begin_sync()
        self._current_shape = self._new_shape(paint_node, frame, prefix, pei, pei)
        if self._current_shape:
            self._undo_stack.append((self._current_node, self._current_frame, self._current_shape))
            self._redo_stack.clear()
            self._notify_buttons()
        else:
            self._end_sync(force=True)

    def _do_release(self, pei):
        if pei is not None:
            self._update_shape(self._current_shape, self._shape_type, self._anchor, pei)
        self._shape_active = False
        self._current_shape = None
        commands.sendInternalEvent("annotate-shape-released")
        commands.redraw()
        self._end_sync(force=True)

    # ------------------------------------------------------------------
    # Pointer event handlers
    # ------------------------------------------------------------------

    def on_push(self, event):
        self._notify_draw_started()
        if self._mode._tool == TOOL_TEXT:
            # Commit any in-progress text, then record the new anchor.
            # The node is NOT created yet — it is deferred to the first keypress
            # so that clicking without typing leaves nothing behind.
            if self._text_active:
                self._commit_text()
            name, pei = self._pointer_location(event)
            if not name:
                return
            self._current_source_name = name
            paint_node, frame = self._find_paint_node()
            self._text_active = True
            self._text_buffer = ""
            self._text_node = None
            self._text_anchor = pei
            self._text_paint_node = paint_node
            self._text_frame = frame
            return

        if self._mode._tool in (TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER):
            self._pen_push(event)
            return

        if not self._is_shape_tool():
            event.reject()
            return
        if self._shift_transition:
            self._shift_transition = False
            return
        paint_node, frame = self._find_paint_node()
        if paint_node is None:
            event.reject()
            return
        name, pei = self._pointer_location(event)
        if not name:
            event.reject()
            return
        self._current_source_name = name
        self._constraint_angle = None
        self._do_push(pei, paint_node, frame)

    def on_drag(self, event):
        if self._mode._tool == TOOL_TEXT:
            return  # consume without action during text placement
        if self._mode._tool in (TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER):
            self._pen_drag(event)
            return
        if not self._is_shape_tool() or not self._shape_active:
            event.reject()
            return
        name, pei = self._pointer_location(event)
        if not name:
            return
        self._last_pei = pei
        self._update_shape(self._current_shape, self._shape_type, self._anchor, pei)

    def on_release(self, event):
        if self._mode._tool == TOOL_TEXT:
            return
        if self._mode._tool in (TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER):
            self._pen_release(event)
            return
        if not self._is_shape_tool() or not self._shape_active:
            event.reject()
            return
        if self._shift_transition:
            return
        name, pei = self._pointer_location(event)
        self._do_release(pei if name else None)

    def on_push_shift(self, event):
        self._notify_draw_started()
        if self._mode._tool == TOOL_TEXT:
            return
        if self._mode._tool in (TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER):
            self._pen_push(event)
            return
        if not self._is_shape_tool():
            event.reject()
            return
        if self._shift_transition:
            self._shift_transition = False
            return
        paint_node, frame = self._find_paint_node()
        if paint_node is None:
            event.reject()
            return
        name, pei = self._pointer_location(event)
        if not name:
            event.reject()
            return
        self._current_source_name = name
        self._constraint_angle = None
        self._do_push(pei, paint_node, frame)

    def on_drag_shift(self, event):
        if self._mode._tool == TOOL_TEXT:
            return
        if self._mode._tool in (TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER):
            self._pen_drag(event)
            return
        if not self._is_shape_tool() or not self._shape_active:
            event.reject()
            return
        name, pei = self._pointer_location(event)
        if not name:
            return
        self._last_pei = pei
        self._update_shape(
            self._current_shape, self._shape_type, self._anchor, self._constrain(self._shape_type, self._anchor, pei)
        )

    def on_release_shift(self, event):
        if self._mode._tool == TOOL_TEXT:
            return
        if self._mode._tool in (TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER):
            self._pen_release(event)
            return
        if not self._is_shape_tool() or not self._shape_active:
            event.reject()
            return
        if self._shift_transition:
            return
        name, pei = self._pointer_location(event)
        if name:
            self._do_release(self._constrain(self._shape_type, self._anchor, pei))
        else:
            self._do_release(None)

    def on_shift_down(self, event):
        if self._text_active:
            return  # don't interfere with text shift+letter input
        if self._shape_active and self._anchor and self._last_pei:
            dx = self._last_pei.x - self._anchor.x
            dy = self._last_pei.y - self._anchor.y
            self._constraint_angle = math.atan2(dy, dx)
        self._shift_transition = True

    def on_shift_up(self, event):
        if self._text_active:
            return
        self._constraint_angle = None
        if self._shape_active:
            self._shift_transition = True

    # ------------------------------------------------------------------
    # Text key handlers
    # ------------------------------------------------------------------

    def on_text_key(self, event):
        if not self._text_active:
            event.reject()
            return
        parts = event.name().split("--")
        ch = parts[-1]
        if len(ch) != 1:
            event.reject()
            return
        has_shift = "shift" in parts[:-1]
        if has_shift:
            char = ch.upper() if ch.isalpha() else _SHIFT_MAP.get(ch, ch)
        else:
            char = ch
        self._text_buffer += char
        self._ensure_text_node()
        self._update_text_display(cursor=True)

    def on_text_space(self, event):
        if not self._text_active:
            event.reject()
            return
        self._text_buffer += " "
        self._ensure_text_node()
        self._update_text_display(cursor=True)

    def on_text_backspace(self, event):
        if not self._text_active:
            event.reject()
            return
        if self._text_buffer:
            self._text_buffer = self._text_buffer[:-1]
        self._update_text_display(cursor=True)

    def on_text_commit(self, event):
        if not self._text_active:
            event.reject()
            return
        self._commit_text()

    def on_text_cancel(self, event):
        if not self._text_active:
            event.reject()
            return
        self._cancel_text()

    def on_key_up(self, event):
        # Consume key-up events during text input so they don't propagate
        if not self._text_active:
            event.reject()

    def on_frame_changed(self, event):
        if self._text_active:
            self._commit_text()
        event.reject()  # let normal frame-change handling continue

    # ------------------------------------------------------------------
    # Undo / redo / clear
    # ------------------------------------------------------------------

    def has_undo(self):
        return bool(self._undo_stack)

    def has_redo(self):
        return bool(self._redo_stack)

    # ------------------------------------------------------------------
    # Order-list helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _component_name(node_name):
        """Extract the component key from a full property path.

        e.g. "RVPaint_1.rect:3:42:host_1234"  →  "rect:3:42:host_1234"
        """
        return node_name.split(".", 1)[1] if "." in node_name else node_name

    def _remove_from_order(self, paint_node, frame, node_name):
        """Remove a component from the frame draw-order list."""
        if not paint_node:
            return
        order_prop = f"{paint_node}.frame:{frame}.order"
        if not commands.propertyExists(order_prop):
            return
        comp = self._component_name(node_name)
        current = list(commands.getStringProperty(order_prop))
        if comp in current:
            current.remove(comp)
            commands.setStringProperty(order_prop, current, True)

    def _restore_to_order(self, paint_node, frame, node_name):
        """Re-append a component to the frame draw-order list."""
        if not paint_node:
            return
        order_prop = f"{paint_node}.frame:{frame}.order"
        if not commands.propertyExists(order_prop):
            return
        comp = self._component_name(node_name)
        current = list(commands.getStringProperty(order_prop))
        if comp not in current:
            commands.insertStringProperty(order_prop, [comp])

    # ------------------------------------------------------------------
    # Undo / redo / clear
    # ------------------------------------------------------------------

    def undo(self):
        if not self._undo_stack:
            return
        paint_node, frame, node_name = self._undo_stack.pop()
        self._begin_sync()
        try:
            self._remove_from_order(paint_node, frame, node_name)
            commands.setIntProperty(f"{node_name}.softDeleted", [1], True)
            commands.redraw()
        except Exception as e:
            print(f"[annotate_toolbar] undo error: {e}")
        self._end_sync(force=True)
        self._redo_stack.append((paint_node, frame, node_name))
        self._notify_buttons()
        commands.sendInternalEvent("undo-paint", self._uuid_for(node_name))

    def redo(self):
        if not self._redo_stack:
            return
        paint_node, frame, node_name = self._redo_stack.pop()
        self._begin_sync()
        try:
            self._restore_to_order(paint_node, frame, node_name)
            commands.setIntProperty(f"{node_name}.softDeleted", [0], True)
            commands.redraw()
        except Exception as e:
            print(f"[annotate_toolbar] redo error: {e}")
        self._end_sync(force=True)
        self._undo_stack.append((paint_node, frame, node_name))
        self._notify_buttons()
        commands.sendInternalEvent("redo-paint", self._uuid_for(node_name))

    @staticmethod
    def _annotated_frames(paint_node):
        """Return the set of frame numbers that have an order property on paint_node.

        Scans actual properties rather than iterating a frame range so that
        annotations stored at source-space frame numbers outside the current
        timeline range are still found.
        """
        try:
            frames = set()
            for prop in commands.properties(paint_node):
                # prop is e.g. "RVPaint_1.frame:42.order"
                parts = prop.split(".")
                if len(parts) >= 3 and parts[2] == "order":
                    comp_parts = parts[1].split(":")
                    if len(comp_parts) == 2 and comp_parts[0] == "frame":
                        frames.add(int(comp_parts[1]))
            return frames
        except Exception:
            return set()

    def clear_frame(self):
        """Remove all visible nodes on the current frame from the draw order.

        Iterates every RVPaint node so that annotations received from remote
        clients (which land on the live-review annotation source group's node)
        are cleared along with locally drawn ones.
        """
        _, frame = self._find_paint_node()
        if frame is None:
            return
        all_paint_nodes = commands.nodesOfType("RVPaint")
        if not all_paint_nodes:
            return
        self._begin_sync()
        cleared = []
        cleared_uuids = []
        for paint_node in all_paint_nodes:
            order_prop = f"{paint_node}.frame:{frame}.order"
            if not commands.propertyExists(order_prop):
                continue
            components = list(commands.getStringProperty(order_prop))
            surviving = []
            node_cleared = False
            for comp in components:
                node_name = f"{paint_node}.{comp}"
                deleted_prop = f"{node_name}.softDeleted"
                try:
                    already_deleted = commands.propertyExists(deleted_prop) and commands.getIntProperty(deleted_prop)[0]
                    if not already_deleted:
                        commands.setIntProperty(deleted_prop, [1], True)
                        cleared.append((paint_node, frame, node_name))
                        uuid = self._uuid_for(node_name)
                        if uuid:
                            cleared_uuids.append(uuid)
                        node_cleared = True
                    else:
                        surviving.append(comp)
                except Exception:
                    surviving.append(comp)
            if node_cleared:
                commands.setStringProperty(order_prop, surviving, True)
        self._end_sync(force=True)
        if cleared:
            self._undo_stack.extend(cleared)
            self._redo_stack.clear()
            commands.redraw()
        self._notify_buttons()
        primary = all_paint_nodes[0]
        payload = "|".join(cleared_uuids) if cleared_uuids else f"{primary}:{frame}"
        commands.sendInternalEvent("clear-paint", payload)

    def clear_all_frames(self):
        """Soft-delete all visible nodes on every frame across all paint nodes.

        Iterates every RVPaint node and discovers annotated frames from actual
        properties (matching the old annotate_mode.mu behaviour) so that remote
        annotations and source-space frame numbers outside the timeline range
        are also cleared.
        """
        all_paint_nodes = commands.nodesOfType("RVPaint")
        if not all_paint_nodes:
            return
        self._begin_sync()
        any_cleared = False
        cleared_uuids = []
        for paint_node in all_paint_nodes:
            for frame in self._annotated_frames(paint_node):
                order_prop = f"{paint_node}.frame:{frame}.order"
                if not commands.propertyExists(order_prop):
                    continue
                components = list(commands.getStringProperty(order_prop))
                surviving = []
                frame_cleared = False
                for comp in components:
                    node_name = f"{paint_node}.{comp}"
                    deleted_prop = f"{node_name}.softDeleted"
                    try:
                        already_deleted = (
                            commands.propertyExists(deleted_prop) and commands.getIntProperty(deleted_prop)[0]
                        )
                        if not already_deleted:
                            commands.setIntProperty(deleted_prop, [1], True)
                            uuid = self._uuid_for(node_name)
                            if uuid:
                                cleared_uuids.append(uuid)
                            frame_cleared = True
                        else:
                            surviving.append(comp)
                    except Exception:
                        surviving.append(comp)
                if frame_cleared:
                    commands.setStringProperty(order_prop, surviving, True)
                    any_cleared = True
        self._end_sync(force=True)
        self._undo_stack.clear()
        self._redo_stack.clear()
        self._notify_buttons()
        if any_cleared:
            commands.redraw()
        payload = "|".join(cleared_uuids) if cleared_uuids else all_paint_nodes[0]
        commands.sendInternalEvent("clear-all-paint", payload)

    def _uuid_for(self, node_name):
        """Return the UUID stored on a paint node, or empty string if unavailable."""
        try:
            uuid_prop = f"{node_name}.uuid"
            if commands.propertyExists(uuid_prop):
                return commands.getStringProperty(uuid_prop)[0]
        except Exception:
            pass
        return ""

    def _notify_buttons(self):
        """Tell the mode to update undo/redo button enabled state."""
        try:
            self._mode._update_undo_redo_buttons()
        except Exception:
            pass

    def _notify_draw_started(self):
        """Tell the mode to close any open popups (e.g. colour picker)."""
        try:
            self._mode._hide_colour_picker()
        except Exception:
            pass

    # ------------------------------------------------------------------
    # Stylus eraser-end handlers
    # ------------------------------------------------------------------

    def on_stylus_eraser_push(self, event):
        """Physical eraser end of stylus: always draws with erase mode regardless of tool."""
        self._notify_draw_started()
        self._stylus_erasing = True
        self._pen_push(event)

    def on_stylus_eraser_drag(self, event):
        self._pen_drag(event)

    def on_stylus_eraser_release(self, event):
        self._pen_release(event)
        self._stylus_erasing = False

    # ------------------------------------------------------------------
    # Sync helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _begin_sync():
        """Signal sync.mu to start batching graph-state-change events (RV-to-RV sync)."""
        commands.sendInternalEvent("internal-sync-begin-accumulate")

    @staticmethod
    def _end_sync(force=False):
        """Signal sync.mu to flush the current batch; force=True sends immediately."""
        commands.sendInternalEvent("internal-sync-end-accumulate")
        if force:
            commands.sendInternalEvent("internal-sync-flush")


class _Vec2:
    __slots__ = ("x", "y")

    def __init__(self, x, y):
        self.x = x
        self.y = y
