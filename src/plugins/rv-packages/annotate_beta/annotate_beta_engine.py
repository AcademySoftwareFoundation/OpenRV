# Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import itertools
import math
from collections.abc import Iterable
from contextlib import contextmanager
from dataclasses import dataclass
from enum import IntFlag
from functools import partial
from typing import NamedTuple

from PySide6 import QtCore, QtGui
from rv import commands, extra_commands

import annotate_beta_constants as constants
import annotate_beta_history as history
import annotate_beta_paint as paint
from annotate_beta_types import PaintComponent, PaintFrame

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

# Shift bit of event.modifiers() (TwkApp ModifierEvent::Shift).
_SHIFT_MODIFIER = 1 << 0

# Font sizes are desired px at zoom=1 over this reference height. The WCS size
# is multiplied by _screen_scale() when the text is created, so:
#   - new text has the same initial screen size regardless of zoom
#   - existing text scales with the image as you zoom in (fixed WCS stored)
_FONT_SIZE_REFERENCE_HEIGHT = 1080.0


class TextStyle(IntFlag):
    """Bold/italic/underline bits of a text run. Values match PaintIPNode::TextRunFlags."""

    BOLD = 1
    ITALIC = 2
    UNDERLINE = 4


def _text_style(bold: bool, italic: bool, underline: bool) -> TextStyle:
    style = TextStyle(0)
    for flag, enabled in ((TextStyle.BOLD, bold), (TextStyle.ITALIC, italic), (TextStyle.UNDERLINE, underline)):
        if enabled:
            style |= flag
    return style


def _style_format(style: TextStyle, enabled: bool) -> QtGui.QTextCharFormat:
    """Char format that turns one style on or off."""
    char_format = QtGui.QTextCharFormat()
    if style == TextStyle.BOLD:
        char_format.setFontWeight(QtGui.QFont.Weight.Bold if enabled else QtGui.QFont.Weight.Normal)
    elif style == TextStyle.ITALIC:
        char_format.setFontItalic(enabled)
    else:
        char_format.setFontUnderline(enabled)
    return char_format


class _Vec2(NamedTuple):
    x: float
    y: float


@dataclass(frozen=True)
class AvailableActions:
    can_undo: bool = False
    can_redo: bool = False
    can_clear_frame: bool = False
    can_clear_all_frames: bool = False


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
    font_size: int = constants.DEFAULT_FONT_SIZE
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
    # Pixel size of the editing document's font. Layout scales with the font size, so the
    # document maps to the rendered text by _TEXT_EDIT_PIXEL_SIZE / font size per WCS unit.
    _TEXT_EDIT_PIXEL_SIZE = 100

    def __init__(self, settings, on_history_changed, on_draw_started, on_eyedropper):
        self._settings = settings
        self._on_history_changed = on_history_changed
        self._on_draw_started = on_draw_started
        self._on_eyedropper = on_eyedropper

        self.preferred_paint_node = ""  # Paint node set by external packages via set-current-annotate-mode-node
        self._visible_frames: set[PaintFrame] = set()

        # Shape drag state
        self._anchor = None
        self._last_image_point = None
        self._current_shape: PaintComponent | None = None
        self._shape_type = None
        self._shape_active = False
        self._constraint_angle = None

        # Text editing state
        self._text_active = False
        self._text_document: QtGui.QTextDocument | None = None
        self._text_cursor: QtGui.QTextCursor | None = None
        self._text_origin: _Vec2 | None = None  # image-space anchor of the active text box
        self._text_font_size = 0.0  # WCS font size of the active text box
        self._text_depth_limit: float | None = None  # document pixels from the first baseline to the image bottom
        self._text_selecting = False  # True while the pointer drags a selection
        self._text_base_style = TextStyle(0)  # toolbar style of the active text box
        self._text_component: PaintComponent | None = None
        self._text_registered = False

        # Pen/eraser stroke state
        self._pen_stroke: PaintComponent | None = None  # set on push, cleared on release
        self._pen_stroke_width = 0.0  # base width for the active stroke, scaled by pressure per point
        self._pen_last_point = None  # last point recorded into the active stroke
        # True while the physical eraser end of a Wacom stylus is in use; forces erase
        # mode regardless of the selected tool.
        self._stylus_erasing = False

        # Source image node name captured on each pointer-push event.
        # Used by _screen_scale() so widths stored in image space remain
        # screen-constant across different zoom levels and image aspect ratios.
        self._current_source_name = None

        self._history = history.UndoHistory()

    # ------------------------------------------------------------------
    # Event table setup
    # ------------------------------------------------------------------

    def setup_event_table(self, mode):
        mode.defineEventTable(constants.EVENT_TABLE_NAME, self.bindings)
        mode.defineEventTableRegex(constants.EVENT_TABLE_NAME, self.regex_bindings)

    @property
    def pointer_bindings(self):
        """Pointer and stylus bindings for the mode's global table.

        They live in the global table rather than the pushed EVENT_TABLE_NAME so that
        widgets with a bounding box (timeline, timeline magnifier) get pointer
        events inside their area first.
        """
        push_constrained = partial(self.on_push, constrained=True)
        drag_constrained = partial(self.on_drag, constrained=True)
        release_constrained = partial(self.on_release, constrained=True)
        return [
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
        ]

    @property
    def bindings(self):
        commit = partial(self.on_text_commit, reject=False)
        commit_and_propagate = partial(self.on_text_commit, reject=True)
        insert_new_line = partial(self._append_text, text="\n")
        select_to = partial(self.on_text_move, keep_anchor=True)
        return [
            # Shape shift-constraint tracking
            ("key-down--shift--shift", self.on_shift_down, "Constrain shape"),
            ("key-up--shift", self.on_shift_up, "Release constraint"),
            # Text editing — explicit keys
            ("key-down--backspace", self.on_text_backspace, "Delete previous char"),
            ("key-down--delete", self.on_text_delete, "Delete next char"),
            ("key-down--left", partial(self.on_text_move, move=QtGui.QTextCursor.MoveOperation.Left), "Cursor left"),
            ("key-down--right", partial(self.on_text_move, move=QtGui.QTextCursor.MoveOperation.Right), "Cursor right"),
            ("key-down--up", partial(self.on_text_move, move=QtGui.QTextCursor.MoveOperation.Up), "Cursor up"),
            ("key-down--down", partial(self.on_text_move, move=QtGui.QTextCursor.MoveOperation.Down), "Cursor down"),
            (
                "key-down--home",
                partial(self.on_text_move, move=QtGui.QTextCursor.MoveOperation.StartOfLine),
                "Cursor to line start",
            ),
            (
                "key-down--end",
                partial(self.on_text_move, move=QtGui.QTextCursor.MoveOperation.EndOfLine),
                "Cursor to line end",
            ),
            ("key-down--shift--left", partial(select_to, move=QtGui.QTextCursor.MoveOperation.Left), "Select left"),
            ("key-down--shift--right", partial(select_to, move=QtGui.QTextCursor.MoveOperation.Right), "Select right"),
            ("key-down--shift--up", partial(select_to, move=QtGui.QTextCursor.MoveOperation.Up), "Select up"),
            ("key-down--shift--down", partial(select_to, move=QtGui.QTextCursor.MoveOperation.Down), "Select down"),
            (
                "key-down--shift--home",
                partial(select_to, move=QtGui.QTextCursor.MoveOperation.StartOfLine),
                "Select to line start",
            ),
            (
                "key-down--shift--end",
                partial(select_to, move=QtGui.QTextCursor.MoveOperation.EndOfLine),
                "Select to line end",
            ),
            ("key-down--control--b", partial(self.on_text_style, style=TextStyle.BOLD), "Toggle bold text"),
            ("key-down--control--i", partial(self.on_text_style, style=TextStyle.ITALIC), "Toggle italic text"),
            ("key-down--control--u", partial(self.on_text_style, style=TextStyle.UNDERLINE), "Toggle underlined text"),
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
            ("before-node-delete", commit_and_propagate, "Commit text"),
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
        return self._settings.font_size / _FONT_SIZE_REFERENCE_HEIGHT * self._screen_scale()

    def _new_text_document(self, line_width: float) -> QtGui.QTextDocument:
        """Editing document for a new text box, with the renderer's font and wrap settings.

        line_width is in document pixels, 0 for no wrapping.
        """
        font = QtGui.QFont()
        if self._settings.font_family:
            font.setFamily(self._settings.font_family)
        font.setPixelSize(self._TEXT_EDIT_PIXEL_SIZE)
        font.setBold(self._settings.font_bold)
        font.setItalic(self._settings.font_italic)
        font.setUnderline(self._settings.font_underline)
        font.setKerning(False)
        font.setHintingPreference(QtGui.QFont.HintingPreference.PreferVerticalHinting)
        document = QtGui.QTextDocument()
        document.setDocumentMargin(0)
        document.setDefaultFont(font)
        if line_width > 0:
            document.setTextWidth(line_width)
        return document

    def _border_width(self):
        scale = self._screen_scale() if self._settings.scale_brush else 1.0
        return max(_BORDER_WIDTH_MIN, self._settings.tool_state.size * _SIZE_SCALE * scale)

    # ------------------------------------------------------------------
    # Paint node resolution
    # ------------------------------------------------------------------

    @staticmethod
    def _is_in_source_group(node: str) -> bool:
        group = commands.nodeGroup(node)
        return bool(group) and commands.nodeType(group) == "RVSourceGroup"

    def _find_paint_frame(self) -> PaintFrame | None:
        try:
            infos = commands.metaEvaluate(commands.frame(), commands.viewNode())
        except Exception:  # raised when the view node name is invalid
            return None

        # If an external package has nominated a specific paint node (via
        # set-current-annotate-mode-node), prefer it so that RV-drawn
        # annotations land on the annotation source group's node rather than the local pipeline node.
        if self.preferred_paint_node:
            for info in infos:
                if info.get("node") == self.preferred_paint_node:
                    return PaintFrame(self.preferred_paint_node, info["frame"])
            # Preferred node isn't part of the current view (e.g. it was set
            # while viewing a different view/layout) — fall through to normal
            # resolution below rather than trusting a stale override. Matches
            # legacy annotate_mode.mu's updateCurrentNode(), which never uses
            # _userSelectedNode unless it's found in the current metaEvaluate.

        paint_infos = [info for info in infos if info.get("nodeType") == "RVPaint"]
        if not paint_infos:
            return None

        # Only draw on a source when exactly one is visible. With several (e.g. layout), use the view's own paint node.
        if self._settings.store_on_source:
            source_infos = [info for info in paint_infos if self._is_in_source_group(info["node"])]
            if len(source_infos) == 1:
                return PaintFrame(source_infos[0]["node"], source_infos[0]["frame"])

        return PaintFrame(paint_infos[0]["node"], paint_infos[0]["frame"])

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
        # Update RV's shared pointer state
        try:
            extra_commands.recordPixelInfo(event)
        except Exception:
            pass  # A failure here must not interrupt the stroke

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
        self._history.push_create(self._text_component)
        self._text_registered = True
        self._on_history_changed()

    def _text(self) -> str:
        return self._text_document.toPlainText()

    def _update_text_display(self):
        cursor = self._text_cursor
        selection = (cursor.selectionStart(), cursor.selectionEnd()) if cursor.hasSelection() else None
        paint.set_text(self._text_component, self._text(), cursor.position(), selection, self._text_runs())

    def _style_of(self, char_format: QtGui.QTextCharFormat) -> TextStyle:
        """Style of a char format, falling back to the box's base style for what it doesn't set."""
        base = self._text_base_style
        bold = (
            char_format.fontWeight() >= QtGui.QFont.Weight.Bold
            if char_format.hasProperty(QtGui.QTextFormat.Property.FontWeight)
            else TextStyle.BOLD in base
        )
        italic = (
            char_format.fontItalic()
            if char_format.hasProperty(QtGui.QTextFormat.Property.FontItalic)
            else TextStyle.ITALIC in base
        )
        underline = (
            char_format.fontUnderline()
            if char_format.hasProperty(QtGui.QTextFormat.Property.TextUnderlineStyle)
            else TextStyle.UNDERLINE in base
        )
        return _text_style(bold, italic, underline)

    def _char_styles(self) -> list[TextStyle | None]:
        """Style of each character of the text, None for line breaks."""
        PARAGRAPH_SEPARATOR = "\u2029"
        document = self._text_document
        cursor = QtGui.QTextCursor(document)
        styles = []
        for position in range(document.characterCount() - 1):
            if document.characterAt(position) == PARAGRAPH_SEPARATOR:
                styles.append(None)
                continue
            # charFormat() is the format of the character before the cursor.
            cursor.setPosition(position + 1)
            styles.append(self._style_of(cursor.charFormat()))
        return styles

    def _text_runs(self) -> list[int]:
        """[start, length, flags] for each range whose style differs from the base style."""
        base = self._text_base_style
        runs = []
        position = 0
        for style, characters in itertools.groupby(base if style is None else style for style in self._char_styles()):
            length = len(list(characters))
            if style != base:
                runs += [position, length, int(style)]
            position += length
        return runs

    def _text_position_at(self, image_point, inside_only):
        """Cursor position under image_point, or None when inside_only and the point is outside the box."""
        layout = self._text_document.documentLayout()
        first_line = self._text_document.firstBlock().layout().lineAt(0)
        pixels_per_unit = self._TEXT_EDIT_PIXEL_SIZE / self._text_font_size
        # The renderer puts the first baseline at the box origin, with y going up in image space.
        x = (image_point.x - self._text_origin.x) * pixels_per_unit
        y = first_line.ascent() - (image_point.y - self._text_origin.y) * pixels_per_unit
        if inside_only:
            size = layout.documentSize()
            margin = first_line.height() / 2
            # documentSize() spans the whole wrap width, idealWidth() only the widest line.
            inside_horizontally = -margin <= x <= self._text_document.idealWidth() + margin
            inside_vertically = 0 <= y <= size.height()
            if not (inside_horizontally and inside_vertically):
                return None
        return layout.hitTest(QtCore.QPointF(x, y), QtCore.Qt.HitTestAccuracy.FuzzyHit)

    def _reset_text(self):
        self._text_active = False
        self._text_component = None
        self._text_document = None
        self._text_cursor = None
        self._text_origin = None
        self._text_depth_limit = None
        self._text_selecting = False
        self._text_registered = False

    def _commit_text(self):
        # Nothing typed: clean up the placeholder node rather than leaving an empty annotation
        if not self._text().strip():
            self._cancel_text()
            return
        paint.finish_text(self._text_component, self._text())
        self._reset_text()
        commands.sendInternalEvent("annotate-text-committed")

    def _cancel_text(self):
        paint.finish_text(self._text_component, self._text())
        paint.soft_delete(self._text_component)
        commands.redraw()

        if self._text_registered and self._history.discard_latest_create(self._text_component):
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
        paint_frame = self._find_paint_frame()
        if paint_frame is None:
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
            if tool == constants.Tool.BRUSH:
                brush = constants.Brush.GAUSS
            elif tool == constants.Tool.ERASER:
                brush = self._settings.eraser_brush
            else:
                brush = constants.Brush.CIRCLE
        self._pen_stroke = paint.new_stroke(
            paint_frame,
            image_point,
            self._pressure_width(event),
            brush,
            erase,
            self._settings.tool_state,
        )
        self._auto_mark_frame()
        self._pen_last_point = image_point
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
        self._history.push_create(self._pen_stroke)
        self._on_history_changed()
        commands.sendInternalEvent("annotate-stroke-released")
        self._pen_stroke = None
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

    def _do_push(self, image_point, paint_frame: PaintFrame):
        if commands.isPlaying():
            commands.stop()
            commands.setFrame(paint_frame.frame)
        prefix = _PREFIX[self._settings.tool]
        self._anchor = image_point
        self._last_image_point = image_point
        self._shape_type = prefix
        self._shape_active = True
        # Open a sync accumulation block that stays open until _do_release so the
        # entire shape (push → drag → release) is sent as one batch to remote
        # clients instead of immediately broadcasting the initial zero-size shape.
        self._begin_sync()
        self._current_shape = paint.new_shape(
            paint_frame,
            prefix,
            image_point,
            image_point,
            self._settings.tool_state,
            self._border_width(),
        )
        self._auto_mark_frame()
        self._history.push_create(self._current_shape)
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
        tool = self._settings.tool
        if tool == constants.Tool.EYEDROPPER and not constrained:
            self._on_eyedropper(event)
            return
        if tool not in constants.DRAWING_TOOLS:
            event.reject()
            return
        self._on_draw_started()
        if tool == constants.Tool.TEXT:
            if not constrained and not self._place_text_cursor(event):
                self._start_text(event)
            return
        if tool in constants.BRUSH_TOOLS:
            self._pen_push(event)
            return
        if self._shape_active:
            return  # synthetic push after a Shift change mid-drag; keep the current shape
        paint_frame = self._find_paint_frame()
        if paint_frame is None:
            event.reject()
            return
        source_name, image_point = self._pointer_location(event)
        if not source_name:
            event.reject()
            return
        self._current_source_name = source_name
        self._constraint_angle = None
        self._do_push(image_point, paint_frame)

    def on_drag(self, event, constrained=False):
        tool = self._settings.tool
        if tool == constants.Tool.TEXT:
            self._drag_text_selection(event)
            return
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
            self._text_selecting = False
            return
        if tool in constants.BRUSH_TOOLS:
            self._pen_release(event)
            return
        if tool not in constants.SHAPE_TOOLS or not self._shape_active:
            event.reject()
            return
        # On a modifier change mid-drag, QTTranslator::sendMouseEvent sends a synthetic
        # release named with the old modifiers but carrying the live ones. A real release
        # always matches, so a Shift mismatch means the drag continues.
        if constrained != bool(event.modifiers() & _SHIFT_MODIFIER):
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
        paint_frame = self._find_paint_frame()
        if paint_frame is None:
            event.reject()
            return
        self._text_active = True
        self._text_origin = image_point
        self._text_font_size = self._font_size()
        pixels_per_unit = self._TEXT_EDIT_PIXEL_SIZE / self._text_font_size
        wrap_width = 0.0
        edges = self._image_edges(source_name)
        if edges:
            right_edge, bottom_edge = edges
            wrap_width = max(0.0, right_edge - image_point.x)
            self._text_depth_limit = (image_point.y - bottom_edge) * pixels_per_unit
        self._text_document = self._new_text_document(wrap_width * pixels_per_unit)
        self._text_cursor = QtGui.QTextCursor(self._text_document)
        self._text_base_style = _text_style(
            self._settings.font_bold, self._settings.font_italic, self._settings.font_underline
        )
        with self._sync_batch(force=False):
            self._text_component = paint.new_text_node(
                paint_frame, image_point, self._settings, self._text_font_size, wrap_width
            )

    @staticmethod
    def _image_edges(source_name) -> tuple[float, float] | None:
        """Image-space (right x, bottom y) of the image, or None when they are unknown."""
        # imageGeometry() returns the on-screen corners in the viewport space eventToImageSpace() takes.
        try:
            corners = [
                commands.eventToImageSpace(source_name, corner, True) for corner in commands.imageGeometry(source_name)
            ]
            return max(corner[0] for corner in corners), min(corner[1] for corner in corners)
        except Exception:
            return None

    def _insert_text(self, text) -> bool:
        """Insert text at the caret, unless it starts a line whose baseline is below the image bottom."""
        document, cursor = self._text_document, self._text_cursor
        height_before = document.size().height()
        anchor, position, pending_format = cursor.anchor(), cursor.position(), cursor.charFormat()
        # One edit block per insertion so undo() reverts only this one.
        cursor.beginEditBlock()
        cursor.insertText(text)
        cursor.endEditBlock()
        if self._text_fits_in_image(height_before):
            return True
        document.undo(cursor)
        cursor.setPosition(anchor)
        cursor.setPosition(position, QtGui.QTextCursor.MoveMode.KeepAnchor)
        if not cursor.hasSelection():
            cursor.setCharFormat(pending_format)
        return False

    def _text_fits_in_image(self, height_before) -> bool:
        if self._text_depth_limit is None:
            return True
        document = self._text_document
        # Edits that don't add lines are always allowed, so a box started at the bottom edge keeps one line.
        if document.size().height() <= height_before:
            return True
        first_line_ascent = document.firstBlock().layout().lineAt(0).ascent()
        last_block = document.lastBlock()
        last_line = last_block.layout().lineAt(last_block.layout().lineCount() - 1)
        last_baseline = document.documentLayout().blockBoundingRect(last_block).y() + last_line.y() + last_line.ascent()
        # Compare baselines rather than line boxes: the descent and line gap of the last line may be clipped.
        return last_baseline - first_line_ascent <= self._text_depth_limit

    def _place_text_cursor(self, event) -> bool:
        """Move the caret to a push inside the active text box and start a drag selection."""
        if not self._text_active:
            return False
        _, image_point = self._pointer_location(event)
        position = self._text_position_at(image_point, inside_only=True) if image_point else None
        if position is None:
            return False
        self._text_cursor.setPosition(position)
        self._text_selecting = True
        self._update_text_display()
        return True

    def _drag_text_selection(self, event):
        if not self._text_selecting:
            return
        _, image_point = self._pointer_location(event)
        if image_point is None:
            return
        self._text_cursor.setPosition(
            self._text_position_at(image_point, inside_only=False), QtGui.QTextCursor.MoveMode.KeepAnchor
        )
        self._update_text_display()

    def on_shift_down(self, event):
        if self._text_active:
            return  # don't interfere with text shift+letter input
        if self._shape_active and self._anchor and self._last_image_point:
            dx = self._last_image_point.x - self._anchor.x
            dy = self._last_image_point.y - self._anchor.y
            self._constraint_angle = math.atan2(dy, dx)

    def on_shift_up(self, event):
        if self._text_active:
            return
        self._constraint_angle = None

    # ------------------------------------------------------------------
    # Text key handlers
    # ------------------------------------------------------------------

    def on_text_key(self, event):
        self._append_text(event, event.name().split("--")[-1])

    def _append_text(self, event, text):
        if not self._text_active:
            event.reject()
            return
        if not self._insert_text(text):
            return
        self._register_text_undo()
        self._update_text_display()

    def on_text_backspace(self, event):
        if not self._text_active:
            event.reject()
            return
        self._text_cursor.deletePreviousChar()
        self._update_text_display()

    def on_text_delete(self, event):
        if not self._text_active:
            event.reject()
            return
        self._text_cursor.deleteChar()
        self._update_text_display()

    def on_text_style(self, event, style):
        """Toggle style on the selection, or for the next characters typed when nothing is selected."""
        if not self._text_active:
            event.reject()
            return
        cursor = self._text_cursor
        if cursor.hasSelection():
            selected = self._char_styles()[cursor.selectionStart() : cursor.selectionEnd()]
            enable = not all(style in character for character in selected if character is not None)
        else:
            enable = style not in self._style_of(cursor.charFormat())
        cursor.mergeCharFormat(_style_format(style, enable))
        self._update_text_display()

    def on_text_move(self, event, move, keep_anchor=False):
        if not self._text_active:
            event.reject()
            return
        # The document lays out lazily, and Up/Down/Home/End need line geometry.
        self._text_document.size()
        self._text_cursor.movePosition(
            move, QtGui.QTextCursor.MoveMode.KeepAnchor if keep_anchor else QtGui.QTextCursor.MoveMode.MoveAnchor
        )
        self._update_text_display()

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

    def available_actions(self) -> AvailableActions:
        if commands.isPlaying():
            return AvailableActions()
        paint_frame = self._find_paint_frame()
        return AvailableActions(
            can_undo=self._history.can_undo(paint_frame),
            can_redo=self._history.can_redo(paint_frame),
            can_clear_frame=paint_frame in self._visible_frames,
            can_clear_all_frames=bool(self._visible_frames),
        )

    def sync_paint_nodes(self):
        self._history.retain_paint_nodes(set(commands.nodesOfType("RVPaint")))
        self._visible_frames = {frame for frame in paint.all_annotated_frames() if paint.has_visible_components(frame)}

    def update_visible_frame(self, property_name: str) -> bool:
        paint_frame = paint.order_property_frame(property_name)

        if paint_frame is None:
            return False

        if paint.has_visible_components(paint_frame):
            self._visible_frames.add(paint_frame)
        else:
            self._visible_frames.discard(paint_frame)

        return True

    def clear_annotate_history(self):
        self._reset_text()
        self._history.clear()
        self._on_history_changed()

    def discard_cleared_frame_history(self, order_property: str):
        paint_frame = paint.order_property_frame(order_property)
        self._history.discard_cleared_frame(paint_frame)
        self._on_history_changed()

    def discard_cleared_all_frames_history(self):
        self._history.clear()
        self._on_history_changed()

    def undo(self):
        self.commit_text_if_active()
        action = self._history.undo(self._find_paint_frame())
        if action is None:
            return
        self._apply_history_action(action, hide=action.type == history.PaintActionType.CREATE, event="undo-paint")

    def redo(self):
        self.commit_text_if_active()
        action = self._history.redo(self._find_paint_frame())
        if action is None:
            return
        self._apply_history_action(action, hide=action.type != history.PaintActionType.CREATE, event="redo-paint")

    def _apply_history_action(self, action: history.PaintAction, hide: bool, event: str):
        update = paint.soft_delete if hide else paint.restore
        with self._sync_batch():
            for component in action.components:
                update(component)

        commands.redraw()
        self._on_history_changed()
        self._send_paint_event(event, action.components)

    def _send_paint_event(self, event: str, components: Iterable[PaintComponent]):
        uuids = paint.uuids_for(components)
        if uuids:
            commands.sendInternalEvent(event, "|".join(uuids))

    def clear_frame(self):
        """Soft-delete every visible component on the current paint frame."""
        self.commit_text_if_active()
        paint_frame = self._find_paint_frame()
        if paint_frame is None:
            return
        with self._sync_batch():
            cleared = paint.soft_delete_frame(paint_frame)
        if not cleared:
            return

        self._history.push_clear_all(paint_frame, cleared)
        commands.redraw()
        self._on_history_changed()
        self._send_paint_event("clear-paint", cleared)
        commands.sendInternalEvent("annotate-clear-frame", paint.frame_order_property(paint_frame))

    def clear_all_frames(self):
        """Soft-delete every visible component on every frame of every paint node."""
        self.commit_text_if_active()
        cleared = []
        with self._sync_batch():
            for paint_frame in paint.all_annotated_frames():
                cleared.extend(paint.soft_delete_frame(paint_frame))
        if not cleared:
            return

        self._history.push_clear_all_frames(cleared)
        commands.redraw()
        self._on_history_changed()
        self._send_paint_event("clear-all-paint", cleared)
        commands.sendInternalEvent("annotate-clear-all-frames")

    # ------------------------------------------------------------------
    # Stylus eraser-end handlers
    # ------------------------------------------------------------------

    def on_stylus_eraser_push(self, event):
        """Physical eraser end of stylus: erases with any drawing tool selected."""
        if self._settings.tool not in constants.DRAWING_TOOLS:
            event.reject()
            return
        self._on_draw_started()
        self._stylus_erasing = True
        self._pen_push(event)
        if not self._pen_stroke:
            self._stylus_erasing = False

    def on_stylus_eraser_drag(self, event):
        if not self._stylus_erasing:
            event.reject()
            return
        self._pen_drag(event)

    def on_stylus_eraser_release(self, event):
        if not self._stylus_erasing:
            event.reject()
            return
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
