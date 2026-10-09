# Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import math
from contextlib import contextmanager
from dataclasses import dataclass
from functools import partial
from typing import NamedTuple

from rv import commands

import annotate_beta_constants as constants
import annotate_beta_paint as paint

_PREFIX = {
    constants.Tool.RECT: "rect",
    constants.Tool.CIRCLE: "ellipse",
    constants.Tool.ARROW: "arrow",
    constants.Tool.LINE: "line",
}

_SIZE_SCALE = 1.0 / 10000.0

# Pen/eraser width range matches the Mu annotate_mode penDrawMode (minSize/maxSize).
# Slider 1→100 maps linearly to 0.001→0.024 in normalized image-space units.
_PEN_WIDTH_MIN = 0.001
_PEN_WIDTH_MAX = 0.024

# Minimum spacing between recorded stroke points, as a fraction of stroke width.
# Matches the splat smoothing interval in PaintIPNode::compilePenComponent, so
# closer points only pile up as overlapping airbrush splats.
_PEN_MIN_POINT_SPACING = 0.25

_BORDER_WIDTH_MIN = 0.001

# Base WCS fractions for each font size tier (desired px at zoom=1 / 1080).
# Multiplied by _screen_scale() when the text is created, so:
#   - new text has the same initial screen size regardless of zoom
#   - existing text scales with the image as you zoom in (fixed WCS stored)
_FONT_SIZE_WCS_BASE = {
    constants.FontSize.SMALL: 24.0 / 1080.0,
    constants.FontSize.MEDIUM: 48.0 / 1080.0,
    constants.FontSize.LARGE: 72.0 / 1080.0,
}


class _Vec2(NamedTuple):
    x: float
    y: float


@dataclass
class ToolState:
    """Style remembered separately for each tool."""

    color: object  # QtGui.QColor
    size: int = constants.DEFAULT_SIZE
    opacity: int = constants.DEFAULT_OPACITY
    color_modifier: constants.ColorModifier = constants.ColorModifier.NORMAL
    filled: bool = False


@dataclass
class DrawSettings:
    """Drawing state owned by the mode and read by the engine at draw time."""

    tool_states: dict
    tool: constants.Tool = constants.Tool.PEN
    eraser_brush: constants.Brush = constants.Brush.CIRCLE
    font_family: str = constants.DEFAULT_FONT_FAMILY
    font_size: constants.FontSize = constants.FontSize.MEDIUM
    font_bold: bool = False
    font_italic: bool = False
    font_underline: bool = False
    store_on_source: bool = False
    auto_mark: bool = False
    sync_whole_strokes: bool = False  # True = batch stroke; False = send each point live
    scale_brush: bool = True

    @property
    def tool_state(self):
        return self.tool_states[self.tool]


class AnnotateDrawEngine:
    def __init__(self, settings, on_history_changed, on_draw_started):
        self._settings = settings
        self._on_history_changed = on_history_changed
        self._on_draw_started = on_draw_started

        # Paint node set via set-current-annotate-mode-node; see _find_paint_node.
        self.preferred_paint_node = ""

        # Shape drag state
        self._anchor = None
        self._last_image_point = None
        self._current_shape = None
        self._shape_type = None
        self._shape_active = False
        self._shift_transition = False
        self._constraint_angle = None

        # Text editing state
        self._text_active = False
        self._text_buffer = ""
        self._text_node = None
        self._text_paint_node = None
        self._text_frame = None
        self._text_registered = False

        # Pen/eraser stroke state
        self._pen_stroke = None  # current stroke node name (set on push, cleared on release)
        self._pen_paint_node = None
        self._pen_frame = None
        self._pen_stroke_width = 0.0  # base width for the active stroke, scaled by pressure per point
        self._pen_last_point = None  # last point recorded into the active stroke
        # True while the physical eraser end of a Wacom stylus is in use; forces erase
        # mode regardless of the selected tool.
        self._stylus_erasing = False

        # Source image node name captured on each pointer-push event.
        # Used by _screen_scale() so widths stored in image space remain
        # screen-constant across different zoom levels and image aspect ratios.
        self._current_source_name = None

        # Undo/redo stacks — each entry: (paint_node, frame, node_name)
        # node_name is the full prop path prefix, e.g. "RVPaint_1.rect:3:42:host_123"
        self._undo_stack = []
        self._redo_stack = []

    # ------------------------------------------------------------------
    # Event table setup
    # ------------------------------------------------------------------

    def setup_event_table(self, mode):
        mode.defineEventTable(constants.EVENT_TABLE_NAME, self.bindings)
        mode.defineEventTableRegex(constants.EVENT_TABLE_NAME, self.regex_bindings)

    @property
    def bindings(self):
        push_constrained = partial(self.on_push, constrained=True)
        drag_constrained = partial(self.on_drag, constrained=True)
        release_constrained = partial(self.on_release, constrained=True)
        commit = partial(self.on_text_commit, reject=False)
        commit_and_propagate = partial(self.on_text_commit, reject=True)
        insert_new_line = partial(self._append_text, text="\n")
        return [
            # Pointer events (shapes, strokes and text placement)
            ("pointer-1--push", self.on_push, "Start shape/text"),
            ("pointer-1--drag", self.on_drag, "Update shape"),
            ("pointer-1--release", self.on_release, "Commit shape"),
            ("pointer-1--shift--push", push_constrained, "Start shape (constrained)"),
            ("pointer-1--shift--drag", drag_constrained, "Update shape (constrained)"),
            ("pointer-1--shift--release", release_constrained, "Commit shape (constrained)"),
            ("stylus-pen--push", self.on_push, "Start shape/text (stylus)"),
            ("stylus-pen--drag", self.on_drag, "Update shape (stylus)"),
            ("stylus-pen--release", self.on_release, "Commit shape (stylus)"),
            ("stylus-pen--shift--push", push_constrained, "Start shape (stylus, constrained)"),
            ("stylus-pen--shift--drag", drag_constrained, "Update shape (stylus, constrained)"),
            ("stylus-pen--shift--release", release_constrained, "Commit shape (stylus, constrained)"),
            ("stylus-eraser--push", self.on_stylus_eraser_push, "Start (stylus eraser end)"),
            ("stylus-eraser--drag", self.on_stylus_eraser_drag, "Draw (stylus eraser end)"),
            ("stylus-eraser--release", self.on_stylus_eraser_release, "Commit (stylus eraser end)"),
            # Shape shift-constraint tracking
            ("key-down--shift--shift", self.on_shift_down, "Constrain shape"),
            ("key-up--shift", self.on_shift_up, "Release constraint"),
            # Text editing — explicit keys
            ("key-down--backspace", self.on_text_backspace, "Delete char"),
            ("key-down--delete", self.on_text_backspace, "Delete char"),
            ("key-down--space", partial(self._append_text, text=" "), "Insert space"),
            ("key-down--shift--enter", insert_new_line, "Insert a new line"),
            ("key-down--shift--keypad-enter", insert_new_line, "Insert a new line"),
            ("key-down--return", commit, "Commit text"),
            ("key-down--enter", commit, "Commit text"),
            ("key-down--keypad-enter", commit, "Commit text"),
            ("key-down--escape", self.on_text_cancel, "Cancel text"),
            # Session events
            ("frame-changed", commit_and_propagate, "Commit text"),
            ("before-session-write", commit_and_propagate, "Commit text"),
            ("before-session-write-copy", commit_and_propagate, "Commit text"),
            ("before-play-start", commit_and_propagate, "Commit text"),
            ("before-session-read", commit_and_propagate, "Commit text"),
            ("before-graph-view-change", commit_and_propagate, "Commit text"),
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

    def commit_text_if_active(self):
        if self._text_active:
            self._commit_text()

    # ------------------------------------------------------------------
    # State helpers
    # ------------------------------------------------------------------

    def _screen_scale(self):
        """Scale factor that makes image-space widths screen-constant.

        Returns the view height divided by the image height in event pixels, so
        doubling the zoom halves the result and the rendered width stays the same.
        Because it is based on the displayed image height, it also gives the same
        screen-pixel width for any image aspect ratio.
        """
        source_name = self._current_source_name
        if not source_name:
            return 1.0
        view_height = commands.viewSize()[1]
        event_origin = commands.imageToEventSpace(source_name, (0.0, 0.0), True)
        event_offset = commands.imageToEventSpace(source_name, (0.0, 0.01), True)
        pixels_per_unit = abs(event_offset[1] - event_origin[1]) / 0.01
        return view_height / max(pixels_per_unit, 0.1)

    def _font_size(self):
        base = _FONT_SIZE_WCS_BASE.get(self._settings.font_size, _FONT_SIZE_WCS_BASE[constants.FontSize.MEDIUM])
        return base * self._screen_scale()

    def _border_width(self):
        scale = self._screen_scale() if self._settings.scale_brush else 1.0
        return max(_BORDER_WIDTH_MIN, self._settings.tool_state.size * _SIZE_SCALE * scale)

    # ------------------------------------------------------------------
    # Paint node resolution
    # ------------------------------------------------------------------

    def _find_paint_node(self):
        frame = commands.frame()
        try:
            infos = commands.metaEvaluate(frame, commands.viewNode())
        except Exception:  # raised when the view node name is invalid
            return None, None

        # If an external package has nominated a specific paint node (via
        # set-current-annotate-mode-node), prefer it so that RV-drawn
        # annotations land on the annotation source group's node rather than the local pipeline node.
        if self.preferred_paint_node:
            for info in infos:
                if info.get("node") == self.preferred_paint_node:
                    return self.preferred_paint_node, info["frame"]
            # Preferred node isn't part of the current view (e.g. it was set
            # while viewing a different view/layout) — fall through to normal
            # resolution below rather than trusting a stale override. Matches
            # legacy annotate_mode.mu's updateCurrentNode(), which never uses
            # _userSelectedNode unless it's found in the current metaEvaluate.

        if self._settings.store_on_source:
            for info in infos:
                if info.get("nodeType") == "RVPaint":
                    node = info["node"]
                    group = commands.nodeGroup(node)
                    if group and commands.nodeType(group) == "RVSourceGroup":
                        return node, info["frame"]

        for info in infos:
            if info.get("nodeType") == "RVPaint":
                return info["node"], info["frame"]
        return None, frame

    def _auto_mark_frame(self):
        if self._settings.auto_mark:
            commands.markFrame(commands.frame(), True)

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
        pointer = event.pointer()
        device_pixel_ratio = commands.devicePixelRatio()
        device_pointer = (pointer[0] * device_pixel_ratio, pointer[1] * device_pixel_ratio)

        image_infos = commands.imagesAtPixel(pointer, "annotate")
        if not image_infos:
            return "", None

        for info in reversed(image_infos):
            source_name = info.get("name", "")
            if not source_name:
                continue
            try:
                image_point = commands.eventToImageSpace(source_name, device_pointer, True)
                return source_name, _Vec2(image_point[0], image_point[1])
            except Exception:
                continue

        return "", None

    # ------------------------------------------------------------------
    # Text editing state
    # ------------------------------------------------------------------

    def _register_text_undo(self):
        if self._text_registered:
            return

        self._auto_mark_frame()
        self._undo_stack.append((self._text_paint_node, self._text_frame, self._text_node))
        self._redo_stack.clear()
        self._text_registered = True
        self._on_history_changed()

    def _update_text_display(self, cursor=True):
        paint.set_text(self._text_node, self._text_buffer + "|" if cursor else self._text_buffer)

    def _reset_text(self):
        self._text_active = False
        self._text_node = None
        self._text_buffer = ""
        self._text_paint_node = None
        self._text_frame = None
        self._text_registered = False

    def _commit_text(self):
        # Nothing typed — clean up the placeholder node rather than leaving an empty annotation
        if not self._text_buffer.strip():
            self._cancel_text()
            return
        self._update_text_display(cursor=False)
        self._reset_text()
        commands.sendInternalEvent("annotate-text-committed")

    def _cancel_text(self):
        self._update_text_display(cursor=False)
        paint.soft_delete(self._text_paint_node, self._text_frame, self._text_node)
        commands.redraw()

        if self._text_registered and self._undo_stack:
            _, _, last_node = self._undo_stack[-1]
            if last_node == self._text_node:
                self._undo_stack.pop()
                self._on_history_changed()

        self._reset_text()

    # ------------------------------------------------------------------
    # Pen/eraser strokes
    # ------------------------------------------------------------------

    def _pressure_width(self, event):
        """Return pen width scaled by the event pressure."""
        pressure = max(0.01, min(1.0, event.pressure()))
        return self._pen_stroke_width * pressure

    def _pen_point_too_close(self, image_point):
        """Return True if image_point is closer to the last recorded stroke point than the minimum spacing."""
        if self._pen_last_point is None:
            return False
        distance = math.hypot(image_point.x - self._pen_last_point.x, image_point.y - self._pen_last_point.y)
        return distance < _PEN_MIN_POINT_SPACING * self._pen_stroke_width

    def _pen_push(self, event):
        if commands.isPlaying():
            commands.stop()
        self._begin_sync()
        paint_node, frame = self._find_paint_node()
        if paint_node is None:
            self._end_sync()
            event.reject()
            return
        source_name, image_point = self._pointer_location(event)
        if not source_name:
            self._end_sync()
            event.reject()
            return
        self._current_source_name = source_name
        # Refresh cached stroke width so _pressure_width() uses the current
        # zoom's _screen_scale(), not the stale value from the previous stroke.
        size_fraction = (self._settings.tool_state.size - constants.SIZE_MIN) / (
            constants.SIZE_MAX - constants.SIZE_MIN
        )
        scale = self._screen_scale() if self._settings.scale_brush else 1.0
        self._pen_stroke_width = (_PEN_WIDTH_MIN + size_fraction * (_PEN_WIDTH_MAX - _PEN_WIDTH_MIN)) * scale
        tool = self._settings.tool
        if self._stylus_erasing:
            erase = True
            brush = self._settings.eraser_brush
        else:
            erase = tool == constants.Tool.ERASER
            if tool == constants.Tool.AIRBRUSH:
                brush = constants.Brush.GAUSS
            elif tool == constants.Tool.ERASER:
                brush = self._settings.eraser_brush
            else:
                brush = constants.Brush.CIRCLE
        self._pen_stroke = paint.new_stroke(
            paint_node, frame, image_point, self._pressure_width(event), brush, erase, self._settings.tool_state
        )
        self._auto_mark_frame()
        self._pen_paint_node = paint_node
        self._pen_frame = frame
        self._pen_last_point = image_point
        commands.sendInternalEvent("set-current-annotate-mode-node", paint_node)
        # sync accumulation is still open — will be flushed at _pen_release

    def _pen_drag(self, event):
        if not self._pen_stroke:
            return
        source_name, image_point = self._pointer_location(event)
        if not source_name or self._pen_point_too_close(image_point):
            return
        paint.append_stroke_point(self._pen_stroke, image_point, self._pressure_width(event))
        self._pen_last_point = image_point
        commands.redraw()
        if not self._settings.sync_whole_strokes:
            self._end_sync(force=True)
            self._begin_sync()

    def _pen_release(self, event):
        if not self._pen_stroke:
            return
        source_name, image_point = self._pointer_location(event)
        if source_name and image_point and not self._pen_point_too_close(image_point):
            paint.append_stroke_point(self._pen_stroke, image_point, self._pressure_width(event))
        self._undo_stack.append((self._pen_paint_node, self._pen_frame, self._pen_stroke))
        self._redo_stack.clear()
        self._on_history_changed()
        commands.sendInternalEvent("annotate-stroke-released")
        self._pen_stroke = None
        self._pen_paint_node = None
        self._pen_frame = None
        self._pen_last_point = None
        commands.redraw()
        # End the whole-stroke accumulation started in _pen_push and flush to network.
        self._end_sync(force=True)

    # ------------------------------------------------------------------
    # Shift constraint (shapes only)
    # ------------------------------------------------------------------

    def _constrain(self, prefix, anchor, current):
        dx = current.x - anchor.x
        dy = current.y - anchor.y
        if prefix in paint.BOX_PREFIXES:
            if self._constraint_angle is not None:
                direction_x = math.cos(self._constraint_angle)
                direction_y = math.sin(self._constraint_angle)
                projection = max(dx * direction_x + dy * direction_y, 0.0)
                return _Vec2(anchor.x + projection * direction_x, anchor.y + projection * direction_y)
            side = min(abs(dx), abs(dy))
            return _Vec2(
                anchor.x + (side if dx >= 0 else -side),
                anchor.y + (side if dy >= 0 else -side),
            )
        else:
            length = math.sqrt(dx * dx + dy * dy)
            if length < 1e-5:
                return current
            angle = math.atan2(dy, dx)
            snapped = round(angle / (math.pi / 4)) * (math.pi / 4)
            return _Vec2(
                anchor.x + length * math.cos(snapped),
                anchor.y + length * math.sin(snapped),
            )

    # ------------------------------------------------------------------
    # Common shape push / release
    # ------------------------------------------------------------------

    def _do_push(self, image_point, paint_node, frame):
        if commands.isPlaying():
            commands.stop()
            commands.setFrame(frame)
        prefix = _PREFIX[self._settings.tool]
        self._anchor = image_point
        self._last_image_point = image_point
        self._shape_type = prefix
        self._shape_active = True
        self._shift_transition = False
        commands.sendInternalEvent("set-current-annotate-mode-node", paint_node)
        # Open a sync accumulation block that stays open until _do_release so the
        # entire shape (push → drag → release) is sent as one batch to remote
        # clients instead of immediately broadcasting the initial zero-size shape.
        self._begin_sync()
        self._current_shape = paint.new_shape(
            paint_node, frame, prefix, image_point, image_point, self._settings.tool_state, self._border_width()
        )
        self._auto_mark_frame()
        self._undo_stack.append((paint_node, frame, self._current_shape))
        self._redo_stack.clear()
        self._on_history_changed()

    def _do_release(self, image_point):
        if image_point is not None:
            paint.update_shape(self._current_shape, self._shape_type, self._anchor, image_point)
        self._shape_active = False
        self._current_shape = None
        commands.sendInternalEvent("annotate-shape-released")
        commands.redraw()
        self._end_sync(force=True)

    # ------------------------------------------------------------------
    # Pointer event handlers
    # ------------------------------------------------------------------

    def on_push(self, event, constrained=False):
        self._on_draw_started()
        tool = self._settings.tool
        if tool == constants.Tool.TEXT:
            if not constrained:
                self._start_text(event)
            return
        if tool in constants.BRUSH_TOOLS:
            self._pen_push(event)
            return
        if tool not in constants.SHAPE_TOOLS:
            event.reject()
            return
        if self._shift_transition:
            self._shift_transition = False
            return
        paint_node, frame = self._find_paint_node()
        if paint_node is None:
            event.reject()
            return
        source_name, image_point = self._pointer_location(event)
        if not source_name:
            event.reject()
            return
        self._current_source_name = source_name
        self._constraint_angle = None
        self._do_push(image_point, paint_node, frame)

    def on_drag(self, event, constrained=False):
        tool = self._settings.tool
        if tool == constants.Tool.TEXT:
            return  # consume without action during text placement
        if tool in constants.BRUSH_TOOLS:
            self._pen_drag(event)
            return
        if tool not in constants.SHAPE_TOOLS or not self._shape_active:
            event.reject()
            return
        source_name, image_point = self._pointer_location(event)
        if not source_name:
            return
        self._last_image_point = image_point
        if constrained:
            image_point = self._constrain(self._shape_type, self._anchor, image_point)
        paint.update_shape(self._current_shape, self._shape_type, self._anchor, image_point)

    def on_release(self, event, constrained=False):
        tool = self._settings.tool
        if tool == constants.Tool.TEXT:
            return
        if tool in constants.BRUSH_TOOLS:
            self._pen_release(event)
            return
        if tool not in constants.SHAPE_TOOLS or not self._shape_active:
            event.reject()
            return
        if self._shift_transition:
            return
        source_name, image_point = self._pointer_location(event)
        if not source_name:
            image_point = None
        elif constrained:
            image_point = self._constrain(self._shape_type, self._anchor, image_point)
        self._do_release(image_point)

    def _start_text(self, event):
        if self._text_active:
            self._commit_text()
        source_name, image_point = self._pointer_location(event)
        if not source_name:
            event.reject()
            return
        self._current_source_name = source_name
        paint_node, frame = self._find_paint_node()
        if paint_node is None:
            event.reject()
            return
        self._text_active = True
        self._text_buffer = ""
        self._text_paint_node = paint_node
        self._text_frame = frame
        with self._sync_batch(force=False):
            self._text_node = paint.new_text_node(paint_node, frame, image_point, self._settings, self._font_size())

    def on_shift_down(self, event):
        if self._text_active:
            return  # don't interfere with text shift+letter input
        if self._shape_active and self._anchor and self._last_image_point:
            dx = self._last_image_point.x - self._anchor.x
            dy = self._last_image_point.y - self._anchor.y
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
        self._append_text(event, event.name().split("--")[-1])

    def _append_text(self, event, text):
        if not self._text_active:
            event.reject()
            return
        self._text_buffer += text
        self._register_text_undo()
        self._update_text_display(cursor=True)

    def on_text_backspace(self, event):
        if not self._text_active:
            event.reject()
            return
        if self._text_buffer:
            self._text_buffer = self._text_buffer[:-1]
        self._update_text_display(cursor=True)

    def on_text_commit(self, event, reject):
        if not self._text_active:
            event.reject()
            return

        if reject:
            event.reject()

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

    # ------------------------------------------------------------------
    # Undo / redo / clear
    # ------------------------------------------------------------------

    def has_undo(self):
        return bool(self._undo_stack)

    def has_redo(self):
        return bool(self._redo_stack)

    def clear_annotate_history(self):
        self._reset_text()
        self._undo_stack.clear()
        self._redo_stack.clear()
        self._on_history_changed()

    def undo(self):
        self.commit_text_if_active()
        if not self._undo_stack:
            return
        paint_node, frame, node_name = self._undo_stack.pop()
        with self._sync_batch():
            try:
                paint.soft_delete(paint_node, frame, node_name)
                commands.redraw()
            except Exception as error:
                print(f"[annotate_beta] undo error: {error}")
        self._redo_stack.append((paint_node, frame, node_name))
        self._on_history_changed()
        commands.sendInternalEvent("undo-paint", paint.uuid_for(node_name))

    def redo(self):
        self.commit_text_if_active()
        if not self._redo_stack:
            return
        paint_node, frame, node_name = self._redo_stack.pop()
        with self._sync_batch():
            try:
                paint.restore(paint_node, frame, node_name)
                commands.redraw()
            except Exception as error:
                print(f"[annotate_beta] redo error: {error}")
        self._undo_stack.append((paint_node, frame, node_name))
        self._on_history_changed()
        commands.sendInternalEvent("redo-paint", paint.uuid_for(node_name))

    def clear_frame(self):
        """Remove all visible nodes on the current frame from the draw order.

        Iterates every RVPaint node so that annotations received from remote
        clients (which land on the annotation source group's node)
        are cleared along with locally drawn ones.
        """
        self.commit_text_if_active()
        _, frame = self._find_paint_node()
        if frame is None:
            return
        all_paint_nodes = commands.nodesOfType("RVPaint")
        if not all_paint_nodes:
            return
        with self._sync_batch():
            cleared = [
                (paint_node, frame, node_name)
                for paint_node in all_paint_nodes
                for node_name in paint.soft_delete_frame(paint_node, frame)
            ]
        if cleared:
            self._undo_stack.extend(cleared)
            self._redo_stack.clear()
            commands.redraw()
        self._on_history_changed()
        cleared_uuids = [shape_uuid for _, _, node_name in cleared if (shape_uuid := paint.uuid_for(node_name))]
        payload = "|".join(cleared_uuids) if cleared_uuids else f"{all_paint_nodes[0]}:{frame}"
        commands.sendInternalEvent("clear-paint", payload)

    def clear_all_frames(self):
        """Soft-delete all visible nodes on every frame across all paint nodes.

        Iterates every RVPaint node and discovers annotated frames from actual
        properties (matching the old annotate_mode.mu behaviour) so that remote
        annotations and source-space frame numbers outside the timeline range
        are also cleared.
        """
        self.commit_text_if_active()
        all_paint_nodes = commands.nodesOfType("RVPaint")
        if not all_paint_nodes:
            return
        with self._sync_batch():
            cleared = [
                node_name
                for paint_node in all_paint_nodes
                for frame in paint.annotated_frames(paint_node)
                for node_name in paint.soft_delete_frame(paint_node, frame)
            ]
        self._undo_stack.clear()
        self._redo_stack.clear()
        self._on_history_changed()
        if cleared:
            commands.redraw()
        cleared_uuids = [shape_uuid for node_name in cleared if (shape_uuid := paint.uuid_for(node_name))]
        payload = "|".join(cleared_uuids) if cleared_uuids else all_paint_nodes[0]
        commands.sendInternalEvent("clear-all-paint", payload)

    # ------------------------------------------------------------------
    # Stylus eraser-end handlers
    # ------------------------------------------------------------------

    def on_stylus_eraser_push(self, event):
        """Physical eraser end of stylus: always draws with erase mode regardless of tool."""
        self._on_draw_started()
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

    @contextmanager
    def _sync_batch(self, force=True):
        """Batch every graph change made inside the block into one sync update."""
        self._begin_sync()
        try:
            yield
        finally:
            self._end_sync(force=force)

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
