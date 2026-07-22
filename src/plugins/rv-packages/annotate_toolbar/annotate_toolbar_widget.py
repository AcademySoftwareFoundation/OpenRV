# Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import os

try:
    from PySide2 import QtCore, QtWidgets, QtGui
except ImportError:
    from PySide6 import QtCore, QtWidgets, QtGui

from annotate_toolbar_colour_picker import ColourPickerSection


def _load_bundled_fonts():
    """Register any .ttf/.otf files from the fonts/ directory next to this file."""
    fonts_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fonts")
    if not os.path.isdir(fonts_dir):
        return
    db = QtGui.QFontDatabase()
    for root, _dirs, files in os.walk(fonts_dir):
        for fname in files:
            if fname.lower().endswith((".ttf", ".otf")):
                db.addApplicationFont(os.path.join(root, fname))


# Tool identifiers
TOOL_CURSOR = "cursor"
TOOL_PEN = "pen"
TOOL_AIRBRUSH = "airbrush"
TOOL_ERASER = "eraser"
TOOL_RECT = "rect"
TOOL_CIRCLE = "circle"
TOOL_ARROW = "arrow"
TOOL_LINE = "line"
TOOL_TEXT = "text"
TOOL_EYEDROPPER = "eyedropper"

# Secondary panel page indices
_PAGE_EMPTY = 0  # cursor, eyedropper
_PAGE_BRUSH = 1  # arrow, line
_PAGE_SHAPE = 2  # rect, circle
_PAGE_TEXT = 3  # text
_PAGE_PEN = 4  # pen and airbrush (size/opacity/blend mode)
_PAGE_ERASER = 5  # eraser (brush type combo + size/opacity)

_TOOL_PAGE = {
    TOOL_CURSOR: _PAGE_EMPTY,
    TOOL_EYEDROPPER: _PAGE_EMPTY,
    TOOL_PEN: _PAGE_PEN,
    TOOL_AIRBRUSH: _PAGE_PEN,
    TOOL_ERASER: _PAGE_ERASER,
    TOOL_ARROW: _PAGE_BRUSH,
    TOOL_LINE: _PAGE_BRUSH,
    TOOL_RECT: _PAGE_SHAPE,
    TOOL_CIRCLE: _PAGE_SHAPE,
    TOOL_TEXT: _PAGE_TEXT,
}

# Blend mode values passed to the mode/engine
COLOR_MOD_NORMAL = "normal"
COLOR_MOD_ADDITIVE = "additive"
COLOR_MOD_DARKEN = "darken"

_TOOL_LABEL = {
    TOOL_CURSOR: "↖",
    TOOL_PEN: "✏",
    TOOL_AIRBRUSH: "∴",
    TOOL_ERASER: "⌫",
    TOOL_RECT: "▭",
    TOOL_CIRCLE: "○",
    TOOL_ARROW: "↗",
    TOOL_LINE: "╱",
    TOOL_TEXT: "T",
    TOOL_EYEDROPPER: "⌖",
}

_TOOL_TOOLTIP = {
    TOOL_CURSOR: "Cursor",
    TOOL_PEN: "Pen",
    TOOL_AIRBRUSH: "Airbrush",
    TOOL_ERASER: "Eraser",
    TOOL_RECT: "Rectangle",
    TOOL_CIRCLE: "Circle",
    TOOL_ARROW: "Arrow",
    TOOL_LINE: "Line",
    TOOL_TEXT: "Text",
    TOOL_EYEDROPPER: "Eyedropper",
}

_FONT_SIZES = ("small", "medium", "large")
_FONT_SIZE_LABELS = {"small": "S", "medium": "M", "large": "L"}

# ---------------------------------------------------------------------------
# Stylesheets
# ---------------------------------------------------------------------------

_HEADER_SS = """
    QWidget#AnnotateToolbarHeader {
        background: #171717;
        border-bottom: 1px solid rgba(255, 255, 255, 51);
    }
    QToolButton#HeaderBtn {
        background: transparent;
        border: none;
        color: rgba(255, 255, 255, 153);
        font-size: 11px;
        border-radius: 2px;
        padding: 0px;
        min-width: 16px;
        min-height: 16px;
        max-width: 16px;
        max-height: 16px;
    }
    QToolButton#HeaderBtn:hover { color: #ffffff; background: rgba(255, 255, 255, 26); }
    QLabel#HeaderTitle {
        color: #ffffff;
        font-size: 12px;
        font-weight: bold;
        background: transparent;
    }
"""

_MENU_SS = """
    QMenu {
        background: #2D2D2D;
        border: 1px solid rgba(255, 255, 255, 64);
        border-radius: 4px;
        color: #F5F5F5;
        font-size: 12px;
        padding: 4px 0px;
    }
    QMenu::item {
        padding: 4px 16px;
        background: transparent;
    }
    QMenu::item:selected { background: rgba(255, 255, 255, 51); }
"""

_STRIP_SS = """
    QToolTip {
        color: #F5F5F5;
        background: #2D2D2D;
        border: 1px solid rgba(255, 255, 255, 64);
        border-radius: 2px;
        padding: 2px 4px;
    }
    QWidget#AnnotateToolStrip {
        background: #1f1f1f;
    }
    QToolButton {
        background: rgba(255, 255, 255, 26);
        border: none;
        color: #c8c8c8;
        font-size: 14px;
        border-radius: 2px;
        padding: 0px;
    }
    QToolButton:hover   { background: rgba(255, 255, 255, 51); }
    QToolButton:checked { background: rgba(56, 171, 223, 115); }
    QToolButton:focus   { border: 1px solid rgba(56, 171, 223, 89); }
    QToolButton:disabled { opacity: 0.4; }
    QToolButton[grouppos="first"] {
        border-top-left-radius: 2px; border-top-right-radius: 2px;
        border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;
    }
    QToolButton[grouppos="mid"] {
        border-radius: 0px;
    }
    QToolButton[grouppos="last"] {
        border-top-left-radius: 0px; border-top-right-radius: 0px;
        border-bottom-left-radius: 2px; border-bottom-right-radius: 2px;
    }
    QToolButton#ActionBtn { background: transparent; }
    QToolButton#ActionBtn:hover { background: rgba(255, 255, 255, 51); }
    QToolButton#ActionBtn:pressed { background: rgba(255, 255, 255, 64); }
"""

_PANEL_SS = """
    QWidget#AnnotateSecondaryPanel, QWidget#AnnotateSecondaryPanel * {
        background: #171717;
    }
    QLabel {
        color: #707070;
        font-size: 10px;
        background: transparent;
    }
    QCheckBox {
        color: #F5F5F5;
        font-size: 12px;
        spacing: 5px;
    }
    QCheckBox::indicator {
        width: 12px;
        height: 12px;
        border: 1px solid rgba(255, 255, 255, 128);
        border-radius: 0px;
        background: #535353;
    }
    QCheckBox::indicator:hover {
        border-color: rgba(255, 255, 255, 200);
    }
    QCheckBox::indicator:focus {
        border-color: #38ABDF;
    }
    QCheckBox::indicator:checked {
        border-color: #FFFFFF;
        background: #FFFFFF;
    }
    QCheckBox::indicator:checked:hover {
        border-color: rgba(255, 255, 255, 230);
    }
    QCheckBox::indicator:checked:focus {
        border-color: #38ABDF;
    }
    QWidget#AnnotateSecondaryPanel QToolButton {
        background: rgba(255, 255, 255, 26);
        border: none;
        color: #c8c8c8;
        font-size: 11px;
        border-radius: 2px;
        padding: 1px 4px;
        min-width: 18px;
        min-height: 18px;
    }
    QWidget#AnnotateSecondaryPanel QToolButton:hover    { background: rgba(255, 255, 255, 51); }
    QWidget#AnnotateSecondaryPanel QToolButton:checked  { background: rgba(56, 171, 223, 115); }
    QWidget#AnnotateSecondaryPanel QToolButton:focus    { border: 1px solid rgba(56, 171, 223, 89); }
    QWidget#AnnotateSecondaryPanel QToolButton:disabled { opacity: 0.4; }
    QWidget#AnnotateSecondaryPanel QToolButton[grouppos="first"] {
        border-top-left-radius: 2px; border-top-right-radius: 2px;
        border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;
    }
    QWidget#AnnotateSecondaryPanel QToolButton[grouppos="mid"]  { border-radius: 0px; }
    QWidget#AnnotateSecondaryPanel QToolButton[grouppos="last"] {
        border-top-left-radius: 0px; border-top-right-radius: 0px;
        border-bottom-left-radius: 2px; border-bottom-right-radius: 2px;
    }
    QWidget#AnnotateSecondaryPanel QComboBox#EraserBrushCombo {
        background: #171717;
    }
    QWidget#AnnotateSecondaryPanel QComboBox {
        background: rgba(255, 255, 255, 26);
        border: none;
        border-bottom: 1px solid transparent;
        border-radius: 2px;
        color: #F5F5F5;
        font-size: 12px;
        font-weight: bold;
        padding: 0px 4px;
        min-height: 32px;
        max-height: 32px;
    }
    QWidget#AnnotateSecondaryPanel QComboBox:hover {
        border-bottom: 1px solid rgba(255, 255, 255, 200);
    }
    QWidget#AnnotateSecondaryPanel QComboBox:focus,
    QWidget#AnnotateSecondaryPanel QComboBox:on {
        border-bottom: 1px solid #38ABDF;
    }
    QWidget#AnnotateSecondaryPanel QComboBox::drop-down {
        border: none;
        background: transparent;
        width: 16px;
        subcontrol-origin: padding;
        subcontrol-position: right center;
    }
    QWidget#AnnotateSecondaryPanel QComboBox::down-arrow {
        width: 8px;
        height: 5px;
    }
    QComboBox QAbstractItemView {
        background: #171717;
        border: 1px solid rgba(255, 255, 255, 51);
        border-radius: 4px;
        color: #F5F5F5;
        font-size: 12px;
        font-weight: normal;
        selection-background-color: rgba(255, 255, 255, 51);
        selection-color: #F5F5F5;
        padding: 8px 0px;
        outline: 0px;
    }
    QLineEdit#SliderValue {
        background: rgba(255, 255, 255, 26);
        border: none;
        border-bottom: 1px solid transparent;
        border-radius: 2px;
        color: #F5F5F5;
        font-size: 12px;
        padding: 0px 4px;
        min-height: 22px;
        max-height: 22px;
        selection-background-color: rgba(56, 171, 223, 51);
        selection-color: #F5F5F5;
    }
    QLineEdit#SliderValue:hover {
        border-bottom: 1px solid rgba(255, 255, 255, 200);
    }
    QLineEdit#SliderValue:focus {
        border-bottom: 1px solid #38ABDF;
    }
    QLineEdit#SliderValue:disabled {
        opacity: 0.4;
    }
"""


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _tool_button(label, tooltip, checkable=True, size=30):
    btn = QtWidgets.QToolButton()
    btn.setText(label)
    btn.setToolTip(tooltip)
    btn.setCheckable(checkable)
    btn.setFixedSize(size, size)
    return btn


def _separator(width=None):
    sep = QtWidgets.QWidget()
    sep.setFixedHeight(1)
    if width is not None:
        sep.setFixedWidth(width)
    sep.setStyleSheet("QWidget { background: rgba(255, 255, 255, 64); }")
    return sep


# ---------------------------------------------------------------------------
# Header bar
# ---------------------------------------------------------------------------


class _ToolbarHeader(QtWidgets.QWidget):
    """Compact header bar: [×] [□]  (spacer)  Draw

    Signals:
        close_requested -- user clicked ×
        dock_requested  -- user clicked □
    """

    close_requested = QtCore.Signal()
    dock_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("AnnotateToolbarHeader")
        self.setFixedHeight(24)
        self.setStyleSheet(_HEADER_SS)

        lay = QtWidgets.QHBoxLayout(self)
        lay.setContentsMargins(4, 4, 8, 4)
        lay.setSpacing(4)

        close_btn = QtWidgets.QToolButton()
        close_btn.setObjectName("HeaderBtn")
        close_btn.setText("×")
        close_btn.setToolTip("Close")
        close_btn.setFixedSize(16, 16)
        close_btn.clicked.connect(self.close_requested)
        lay.addWidget(close_btn)

        dock_btn = QtWidgets.QToolButton()
        dock_btn.setObjectName("HeaderBtn")
        dock_btn.setText("□")
        dock_btn.setToolTip("Dock / Undock")
        dock_btn.setFixedSize(16, 16)
        dock_btn.clicked.connect(self.dock_requested)
        lay.addWidget(dock_btn)

        lay.addStretch()

        title = QtWidgets.QLabel("Draw")
        title.setObjectName("HeaderTitle")
        lay.addWidget(title)


# ---------------------------------------------------------------------------
# Colour swatch
# ---------------------------------------------------------------------------


class ColourSwatch(QtWidgets.QAbstractButton):
    """Square button showing the current annotation colour.

    Clicking it emits swatch_clicked — the parent is responsible for
    showing/hiding the inline colour picker.
    """

    swatch_clicked = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._colour = QtGui.QColor(255, 220, 0)
        self.setFixedSize(30, 30)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setToolTip("Colour")

    def get_colour(self):
        return QtGui.QColor(self._colour)

    def set_colour(self, colour):
        self._colour = QtGui.QColor(colour)
        self.update()

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing, False)
        p.setPen(QtGui.QPen(QtGui.QColor("#DCDCDC"), 1))
        p.setBrush(self._colour)
        p.drawRect(self.rect().adjusted(1, 1, -1, -1))

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.swatch_clicked.emit()


# ---------------------------------------------------------------------------
# Custom slider widget (Figma Weave 3.0 design)
# ---------------------------------------------------------------------------


class _CustomSlider(QtWidgets.QWidget):
    """Vertical slider matching the Figma Weave 3.0 spec.

    Rail:   1px wide, rgba(255,255,255,0.5) — above handle
    Fill:   3px wide, #38ABDF              — below handle
    Handle: 18px dark circle (#171717) with 10px blue dot (#38ABDF) inside
    Focus:  1px dark gap + 3px blue ring around inner dot
    Disabled: opacity 0.5
    """

    valueChanged = QtCore.Signal(int)

    _RAIL_W = 1
    _FILL_W = 3
    _OUTER_R = 9  # 18px diameter outer handle circle
    _INNER_R = 5  # 10px diameter inner dot
    _PAD = 12  # padding so handle doesn't clip (must be >= _OUTER_R=9)

    _COL_RAIL = QtGui.QColor(255, 255, 255, 128)
    _COL_FILL = QtGui.QColor("#38ABDF")
    _COL_OUTER = QtGui.QColor("#171717")
    _COL_GAP = QtGui.QColor("#262626")

    def __init__(self, min_val=0, max_val=100, default=50, parent=None):
        super().__init__(parent)
        self._min = min_val
        self._max = max_val
        self._value = max(min_val, min(max_val, default))
        self._pressed = False
        self.setFixedWidth(32)  # Figma: 32px wide after -90deg rotation
        self.setMinimumHeight(110)
        self.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Expanding)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

    def _handle_y(self):
        top, bot = self._PAD, self.height() - self._PAD
        t = (self._value - self._min) / max(1, self._max - self._min)
        return int(bot - t * (bot - top))

    def _y_to_value(self, y):
        top, bot = self._PAD, self.height() - self._PAD
        span = bot - top
        if span <= 0:
            return self._min
        t = max(0.0, min(1.0, 1.0 - (y - top) / span))
        return int(round(self._min + t * (self._max - self._min)))

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing)
        p.setPen(QtCore.Qt.NoPen)

        if not self.isEnabled():
            p.setOpacity(0.5)

        cx = self.width() / 2.0
        top = float(self._PAD)
        bot = float(self.height() - self._PAD)
        hy = float(self._handle_y())

        # Rail above handle (semi-transparent white, 1px)
        rh = self._RAIL_W / 2.0
        p.setBrush(self._COL_RAIL)
        p.drawRoundedRect(QtCore.QRectF(cx - rh, top, self._RAIL_W, hy - top), rh, rh)

        # Fill below handle (blue, 3px)
        fh = self._FILL_W / 2.0
        p.setBrush(self._COL_FILL)
        p.drawRoundedRect(QtCore.QRectF(cx - fh, hy, self._FILL_W, bot - hy), fh, fh)

        # Focus ring when pressed or keyboard-focused
        if self._pressed or self.hasFocus():
            p.setBrush(self._COL_FILL)
            p.drawEllipse(QtCore.QPointF(cx, hy), self._INNER_R + 1 + 3, self._INNER_R + 1 + 3)
            p.setBrush(self._COL_GAP)
            p.drawEllipse(QtCore.QPointF(cx, hy), self._INNER_R + 1, self._INNER_R + 1)

        # Handle outer dark circle
        p.setBrush(self._COL_OUTER)
        p.drawEllipse(QtCore.QPointF(cx, hy), self._OUTER_R, self._OUTER_R)

        # Handle inner blue dot
        p.setBrush(self._COL_FILL)
        p.drawEllipse(QtCore.QPointF(cx, hy), self._INNER_R, self._INNER_R)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._pressed = True
            self.setFocus()
            self._set_from_y(event.y())

    def mouseMoveEvent(self, event):
        if self._pressed:
            self._set_from_y(event.y())

    def mouseReleaseEvent(self, event):
        self._pressed = False
        self.update()

    def focusInEvent(self, event):
        self.update()

    def focusOutEvent(self, event):
        self.update()

    def keyPressEvent(self, event):
        key = event.key()
        step = max(1, (self._max - self._min) // 10)
        if key == QtCore.Qt.Key_Up:
            self._set_value(self._value + 1)
        elif key == QtCore.Qt.Key_Down:
            self._set_value(self._value - 1)
        elif key == QtCore.Qt.Key_PageUp:
            self._set_value(self._value + step)
        elif key == QtCore.Qt.Key_PageDown:
            self._set_value(self._value - step)
        else:
            super().keyPressEvent(event)

    def wheelEvent(self, event):
        delta = event.angleDelta().y()
        self._set_value(self._value + (1 if delta > 0 else -1))

    def _set_from_y(self, y):
        self._set_value(self._y_to_value(y))

    def _set_value(self, v):
        v = max(self._min, min(self._max, v))
        if v != self._value:
            self._value = v
            self.valueChanged.emit(v)
            self.update()

    def value(self):
        return self._value

    def setValue(self, v):
        self._set_value(v)

    def setRange(self, mn, mx):
        self._min = mn
        self._max = mx
        self._set_value(max(mn, min(mx, self._value)))


# ---------------------------------------------------------------------------
# Secondary panel option pages
# ---------------------------------------------------------------------------


class _ValueLineEdit(QtWidgets.QLineEdit):
    """QLineEdit that selects all text when it receives focus."""

    def focusInEvent(self, event):
        super().focusInEvent(event)
        QtCore.QTimer.singleShot(0, self.selectAll)


class _SliderSection(QtWidgets.QWidget):
    """Vertical slider + editable value input."""

    value_changed = QtCore.Signal(int)

    def __init__(self, label, min_val, max_val, default, suffix="", parent=None):
        super().__init__(parent)
        self._suffix = suffix
        self._min = min_val
        self._max = max_val

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(4)

        self._slider = _CustomSlider(min_val, max_val, default)
        self._slider.setToolTip(label)
        self._slider.valueChanged.connect(self._on_slider_changed)
        slider_row = QtWidgets.QHBoxLayout()
        slider_row.setContentsMargins(0, 0, 0, 0)
        slider_row.addStretch()
        slider_row.addWidget(self._slider)
        slider_row.addStretch()
        lay.addLayout(slider_row, 1)

        self._input = _ValueLineEdit(f"{default}{suffix}")
        self._input.setObjectName("SliderValue")
        self._input.setToolTip(label)
        self._input.setAlignment(QtCore.Qt.AlignCenter)
        self._input.setFixedWidth(48)
        self._input.editingFinished.connect(self._on_input_committed)
        lay.addWidget(self._input, alignment=QtCore.Qt.AlignHCenter)

        # Fixed after layout is set up — prevents sections in less-constrained panels
        # (no blend buttons) from claiming leftover space. Preferred has GrowFlag set
        # and quietly absorbs extra space; Fixed (no flags) takes exactly sizeHint.
        self.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)

    def _on_slider_changed(self, v):
        if not self._input.hasFocus():
            self._input.setText(f"{v}{self._suffix}")
        self.value_changed.emit(v)

    def _on_input_committed(self):
        text = self._input.text().strip()
        if self._suffix and text.endswith(self._suffix):
            text = text[: -len(self._suffix)].strip()
        try:
            v = max(self._min, min(self._max, int(round(float(text)))))
            self._slider.setValue(v)
            self._input.setText(f"{v}{self._suffix}")
            self.value_changed.emit(v)
        except (ValueError, TypeError):
            self._input.setText(f"{self._slider.value()}{self._suffix}")

    def get_value(self):
        return self._slider.value()

    def set_value(self, v):
        self._slider.setValue(v)
        if not self._input.hasFocus():
            self._input.setText(f"{v}{self._suffix}")


class _SizeOpacityPanel(QtWidgets.QWidget):
    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(0)

        self._size = _SliderSection("Size", 1, 100, 32)
        self._size.value_changed.connect(self.size_changed)
        lay.addWidget(self._size)

        lay.addSpacing(8)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(8)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addStretch()

    def get_size(self):
        return self._size.get_value()

    def get_opacity(self):
        return self._opacity.get_value()

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


def _load_icon(name):
    """Load a named SVG icon from the package's support files.

    Python files land in PlugIns/Python/ while package support files
    land in PlugIns/SupportFiles/annotate_toolbar/.
    """
    python_dir = os.path.dirname(os.path.abspath(__file__))
    support_dir = os.path.join(os.path.dirname(python_dir), "SupportFiles", "annotate_toolbar")
    path = os.path.join(support_dir, f"icon_{name}.svg")
    if os.path.exists(path):
        return QtGui.QIcon(path)
    return QtGui.QIcon()


def _apply_icon(btn, name, size=16):
    """Set an SVG icon on a button; falls back to keeping existing text."""
    icon = _load_icon(name)
    if not icon.isNull():
        btn.setIcon(icon)
        btn.setIconSize(QtCore.QSize(size, size))
        btn.setText("")


def _load_png_icon(filename):
    """Load a PNG icon by filename from the package's support files."""
    python_dir = os.path.dirname(os.path.abspath(__file__))
    support_dir = os.path.join(os.path.dirname(python_dir), "SupportFiles", "annotate_toolbar")
    path = os.path.join(support_dir, filename)
    if os.path.exists(path):
        return QtGui.QIcon(path)
    # Fallback: same directory as this file (development layout)
    path = os.path.join(python_dir, filename)
    if os.path.exists(path):
        return QtGui.QIcon(path)
    return QtGui.QIcon()


def _apply_combo_style(combo):
    """Apply per-widget combo QSS with runtime-resolved caret arrow images."""
    python_dir = os.path.dirname(os.path.abspath(__file__))
    support_dir = os.path.join(os.path.dirname(python_dir), "SupportFiles", "annotate_toolbar")
    caret = os.path.join(support_dir, "icon_caret_down.svg").replace("\\", "/")
    if os.path.exists(caret):
        combo.setStyleSheet(f"""
            QComboBox::drop-down  {{ border: none; background: transparent; width: 16px;
                                     subcontrol-origin: padding; subcontrol-position: right center; }}
            QComboBox::down-arrow {{ image: url("{caret}"); width: 8px; height: 5px; }}
        """)


def _apply_check_icon(checkbox):
    """Apply Figma-spec checkbox stylesheet with runtime checkmark image path."""
    python_dir = os.path.dirname(os.path.abspath(__file__))
    support_dir = os.path.join(os.path.dirname(python_dir), "SupportFiles", "annotate_toolbar")
    path = os.path.join(support_dir, "icon_check.svg").replace("\\", "/")
    img = f'image: url("{path}");' if os.path.exists(path) else ""
    checkbox.setStyleSheet(f"""
        QCheckBox {{
            color: #F5F5F5;
            font-size: 12px;
            spacing: 5px;
        }}
        QCheckBox::indicator {{
            width: 12px; height: 12px;
            border: 1px solid rgba(255, 255, 255, 128);
            border-radius: 0px;
            background: #535353;
        }}
        QCheckBox::indicator:hover  {{ border-color: rgba(255, 255, 255, 200); }}
        QCheckBox::indicator:focus  {{ border-color: #38ABDF; }}
        QCheckBox::indicator:checked {{
            border-color: #FFFFFF; background: #FFFFFF; {img}
        }}
        QCheckBox::indicator:checked:hover {{
            border-color: rgba(255, 255, 255, 230); background: #FFFFFF; {img}
        }}
        QCheckBox::indicator:checked:focus {{
            border-color: #38ABDF; background: #FFFFFF; {img}
        }}
    """)


class _EraserPanel(QtWidgets.QWidget):
    """Brush-type combo + size/opacity sliders for the eraser tool."""

    eraser_brush_changed = QtCore.Signal(str)  # "circle" or "gauss"
    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)

    _BRUSHES = [
        ("circle", "erase_circle", "Circle (Hard)"),
        ("gauss", "erase_gauss", "Gauss (Soft)"),
    ]

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(0)

        self._brush_combo = QtWidgets.QComboBox()
        self._brush_combo.setObjectName("EraserBrushCombo")
        self._brush_combo.setToolTip("Brush Type")
        self._brush_combo.setIconSize(QtCore.QSize(20, 20))
        for brush_id, icon_name, _label in self._BRUSHES:
            icon = _load_icon(icon_name)
            self._brush_combo.addItem(icon, "", brush_id)
        self._brush_combo.currentIndexChanged.connect(self._on_brush_changed)
        _apply_combo_style(self._brush_combo)
        lay.addWidget(self._brush_combo)

        lay.addSpacing(8)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(8)

        self._size = _SliderSection("Size", 1, 100, 32)
        self._size.value_changed.connect(self.size_changed)
        lay.addWidget(self._size)

        lay.addSpacing(8)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(8)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addStretch()

    def _on_brush_changed(self, index):
        brush = self._brush_combo.itemData(index)
        if brush:
            self.eraser_brush_changed.emit(brush)

    def get_eraser_brush(self):
        return self._brush_combo.currentData() or "circle"

    def set_eraser_brush(self, brush):
        for i in range(self._brush_combo.count()):
            if self._brush_combo.itemData(i) == brush:
                self._brush_combo.setCurrentIndex(i)
                return

    def set_soft_erase_enabled(self, enabled):
        """Enable or disable the Gauss (soft) eraser brush option."""
        model = self._brush_combo.model()
        for i in range(self._brush_combo.count()):
            if self._brush_combo.itemData(i) == "gauss":
                item = model.item(i)
                if item is None:
                    break
                if enabled:
                    item.setFlags(item.flags() | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
                else:
                    item.setFlags(item.flags() & ~(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable))
                    if self._brush_combo.currentIndex() == i:
                        self._brush_combo.setCurrentIndex(0)
                break

    def get_size(self):
        return self._size.get_value()

    def get_opacity(self):
        return self._opacity.get_value()

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


class _PenPanel(QtWidgets.QWidget):
    """Size + opacity sliders plus Normal / Darken / Additive blend mode buttons."""

    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)
    color_modifier_changed = QtCore.Signal(str)  # COLOR_MOD_NORMAL / COLOR_MOD_ADDITIVE / COLOR_MOD_DARKEN

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(0)

        self._size = _SliderSection("Size", 1, 100, 32)
        self._size.value_changed.connect(self.size_changed)
        lay.addWidget(self._size)

        lay.addSpacing(8)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(8)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addSpacing(8)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(8)

        # Blend mode buttons — grouped with 1px gaps, connected border-radius
        self._blend_grp = QtWidgets.QButtonGroup(self)
        self._blend_btns = {}
        btn_container = QtWidgets.QWidget()
        btn_lay = QtWidgets.QVBoxLayout(btn_container)
        btn_lay.setContentsMargins(0, 0, 0, 0)
        btn_lay.setSpacing(0)
        _blend_positions = ["first", "mid", "last"]
        for i, (key, icon_name, tip) in enumerate(
            [
                (COLOR_MOD_NORMAL, "normal", "Normal"),
                (COLOR_MOD_DARKEN, "burn", "Burn"),
                (COLOR_MOD_ADDITIVE, "dodge", "Dodge"),
            ]
        ):
            btn = QtWidgets.QToolButton()
            btn.setText({"normal": "◈", "burn": "🔥", "dodge": "🔍"}[icon_name])
            btn.setToolTip(tip)
            btn.setProperty("grouppos", _blend_positions[i])
            _apply_icon(btn, icon_name)
            btn.setCheckable(True)
            btn.setFixedSize(30, 30)
            self._blend_grp.addButton(btn)
            self._blend_btns[key] = btn
            btn_lay.addWidget(btn, alignment=QtCore.Qt.AlignHCenter)
            if i < 2:
                btn_lay.addSpacing(1)
        self._blend_btns[COLOR_MOD_NORMAL].setChecked(True)
        self._blend_grp.buttonClicked.connect(self._on_blend_clicked)
        lay.addWidget(btn_container, alignment=QtCore.Qt.AlignHCenter)
        lay.addStretch()

    def _on_blend_clicked(self, btn):
        for key, b in self._blend_btns.items():
            if b is btn:
                self.color_modifier_changed.emit(key)
                return

    def get_color_modifier(self):
        for key, btn in self._blend_btns.items():
            if btn.isChecked():
                return key
        return COLOR_MOD_NORMAL

    def set_color_modifier(self, mode):
        btn = self._blend_btns.get(mode)
        if btn:
            btn.setChecked(True)

    def set_blend_mode_enabled(self, mode, enabled):
        """Enable or disable a blend mode button (e.g. burn/dodge) by its key."""
        btn = self._blend_btns.get(mode)
        if btn:
            btn.setEnabled(enabled)
            if not enabled and btn.isChecked():
                self._blend_btns[COLOR_MOD_NORMAL].setChecked(True)
                self.color_modifier_changed.emit(COLOR_MOD_NORMAL)

    def get_size(self):
        return self._size.get_value()

    def get_opacity(self):
        return self._opacity.get_value()

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


class _ShapeOptionsPanel(QtWidgets.QWidget):
    filled_changed = QtCore.Signal(bool)
    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(0)

        self._filled = QtWidgets.QCheckBox("Filled")
        self._filled.setChecked(False)
        self._filled.toggled.connect(self.filled_changed)
        _apply_check_icon(self._filled)
        lay.addWidget(self._filled)
        lay.addSpacing(8)

        self._size = _SliderSection("Size", 1, 100, 32)
        self._size.value_changed.connect(self.size_changed)
        lay.addWidget(self._size)

        lay.addSpacing(8)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(8)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addStretch()

    def get_filled(self):
        return self._filled.isChecked()

    def get_size(self):
        return self._size.get_value()

    def get_opacity(self):
        return self._opacity.get_value()

    def set_filled(self, v):
        self._filled.setChecked(v)

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


class _FontNameDelegate(QtWidgets.QStyledItemDelegate):
    """Renders each font name item in its own typeface."""

    def initStyleOption(self, option, index):
        super().initStyleOption(option, index)
        font_name = index.data()
        if font_name:
            f = QtGui.QFont(font_name)
            pt = option.font.pointSize()
            if pt > 0:
                f.setPointSize(pt)
            else:
                px = option.font.pixelSize()
                if px > 0:
                    f.setPixelSize(px)
            option.font = f


class _TextOptionsPanel(QtWidgets.QWidget):
    font_family_changed = QtCore.Signal(str)
    font_size_changed = QtCore.Signal(str)  # "small" | "medium" | "large"
    font_bold_changed = QtCore.Signal(bool)
    font_italic_changed = QtCore.Signal(bool)
    font_underline_changed = QtCore.Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(8)

        # Font family — system fonts + any fonts bundled in annotation-platform.
        # Filter to smoothly scalable fonts only; bitmap fonts trigger Qt bearing warnings.
        _load_bundled_fonts()
        _db = QtGui.QFontDatabase()
        self._font_combo = QtWidgets.QComboBox()
        self._font_combo.setToolTip("Font")
        self._font_combo.setItemDelegate(_FontNameDelegate(self._font_combo))
        for name in _db.families():
            if not name.startswith(".") and _db.isSmoothlyScalable(name):
                self._font_combo.addItem(name)
        self._font_combo.currentTextChanged.connect(self.font_family_changed)
        self._font_combo.view().setMinimumWidth(160)
        _apply_combo_style(self._font_combo)
        lay.addWidget(self._font_combo)

        # S / M / L size combo
        self._size_combo = QtWidgets.QComboBox()
        self._size_combo.setToolTip("Size")
        for label in ("S", "M", "L"):
            self._size_combo.addItem(label)
        self._size_combo.setCurrentText("M")
        self._size_combo.currentTextChanged.connect(
            lambda t: self.font_size_changed.emit({"S": "small", "M": "medium", "L": "large"}[t])
        )
        self._size_combo.view().setMinimumWidth(60)
        _apply_combo_style(self._size_combo)
        lay.addWidget(self._size_combo)

        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)

        # B / I / U style toggles — full-width, stacked vertically
        def _style_btn(label, tooltip):
            btn = QtWidgets.QToolButton()
            btn.setText(label)
            btn.setCheckable(True)
            btn.setToolTip(tooltip)
            btn.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
            btn.setFixedHeight(30)
            return btn

        self._bold_btn = _style_btn("B", "Bold")
        font_b = self._bold_btn.font()
        font_b.setBold(True)
        self._bold_btn.setFont(font_b)
        _apply_icon(self._bold_btn, "bold")
        self._bold_btn.toggled.connect(self.font_bold_changed)
        lay.addWidget(self._bold_btn)

        self._italic_btn = _style_btn("I", "Italic")
        font_i = self._italic_btn.font()
        font_i.setItalic(True)
        self._italic_btn.setFont(font_i)
        _apply_icon(self._italic_btn, "italic")
        self._italic_btn.toggled.connect(self.font_italic_changed)
        lay.addWidget(self._italic_btn)

        self._underline_btn = _style_btn("U", "Underline")
        font_u = self._underline_btn.font()
        font_u.setUnderline(True)
        self._underline_btn.setFont(font_u)
        _apply_icon(self._underline_btn, "underline")
        self._underline_btn.toggled.connect(self.font_underline_changed)
        lay.addWidget(self._underline_btn)

        lay.addStretch()

    def get_font_family(self):
        return self._font_combo.currentText()

    def get_font_size(self):
        return {"S": "small", "M": "medium", "L": "large"}.get(self._size_combo.currentText(), "medium")

    def get_bold(self):
        return self._bold_btn.isChecked()

    def get_italic(self):
        return self._italic_btn.isChecked()

    def get_underline(self):
        return self._underline_btn.isChecked()

    def set_font_family(self, name):
        idx = self._font_combo.findText(name)
        if idx >= 0:
            self._font_combo.blockSignals(True)
            self._font_combo.setCurrentIndex(idx)
            self._font_combo.blockSignals(False)

    def set_font_size(self, size):
        label = {"small": "S", "medium": "M", "large": "L"}.get(size, "M")
        self._size_combo.blockSignals(True)
        self._size_combo.setCurrentText(label)
        self._size_combo.blockSignals(False)

    def set_bold(self, v):
        self._bold_btn.blockSignals(True)
        self._bold_btn.setChecked(v)
        self._bold_btn.blockSignals(False)

    def set_italic(self, v):
        self._italic_btn.blockSignals(True)
        self._italic_btn.setChecked(v)
        self._italic_btn.blockSignals(False)

    def set_underline(self, v):
        self._underline_btn.blockSignals(True)
        self._underline_btn.setChecked(v)
        self._underline_btn.blockSignals(False)


# ---------------------------------------------------------------------------
# Floating colour picker popup
# ---------------------------------------------------------------------------


class ColourPickerPopup(QtWidgets.QFrame):
    """Floating colour picker that appears to the right of the toolbar."""

    colour_changed = QtCore.Signal(QtGui.QColor)

    def __init__(self, parent=None):
        super().__init__(parent, QtCore.Qt.Tool | QtCore.Qt.FramelessWindowHint)
        self.setAttribute(QtCore.Qt.WA_ShowWithoutActivating)
        self.setStyleSheet("QFrame { background: #1a1a1a; border: 1px solid #333333; border-radius: 4px; }")
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        self._picker = ColourPickerSection()
        self._picker.colour_changed.connect(self.colour_changed)
        lay.addWidget(self._picker)
        self.adjustSize()

    def show_near(self, anchor_widget):
        """Position and show the popup to the right of anchor_widget."""
        pos = anchor_widget.mapToGlobal(QtCore.QPoint(anchor_widget.width() + 6, 0))
        self.move(pos)
        self.show()
        self.raise_()

    def set_colour(self, c):
        self._picker.set_colour(c)

    def get_colour(self):
        return self._picker._colour


# ---------------------------------------------------------------------------
# Secondary panel
# ---------------------------------------------------------------------------


class AnnotateSecondaryPanel(QtWidgets.QWidget):
    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)
    filled_changed = QtCore.Signal(bool)
    color_modifier_changed = QtCore.Signal(str)
    eraser_brush_changed = QtCore.Signal(str)
    font_family_changed = QtCore.Signal(str)
    font_size_changed = QtCore.Signal(str)
    font_bold_changed = QtCore.Signal(bool)
    font_italic_changed = QtCore.Signal(bool)
    font_underline_changed = QtCore.Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("AnnotateSecondaryPanel")
        self.setFixedWidth(80)

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(4, 8, 4, 8)
        lay.setSpacing(0)

        self._stack = QtWidgets.QStackedWidget()
        lay.addWidget(self._stack)

        # Page 0 — empty (cursor / eyedropper)
        self._stack.addWidget(QtWidgets.QWidget())

        # Page 1 — brush tools (arrow, line)
        self._brush_panel = _SizeOpacityPanel()
        self._brush_panel.size_changed.connect(self.size_changed)
        self._brush_panel.opacity_changed.connect(self.opacity_changed)
        self._stack.addWidget(self._brush_panel)

        # Page 2 — shape tools (rect, circle)
        self._shape_panel = _ShapeOptionsPanel()
        self._shape_panel.filled_changed.connect(self.filled_changed)
        self._shape_panel.size_changed.connect(self.size_changed)
        self._shape_panel.opacity_changed.connect(self.opacity_changed)
        self._stack.addWidget(self._shape_panel)

        # Page 3 — text tool
        self._text_panel = _TextOptionsPanel()
        self._text_panel.font_family_changed.connect(self.font_family_changed)
        self._text_panel.font_size_changed.connect(self.font_size_changed)
        self._text_panel.font_bold_changed.connect(self.font_bold_changed)
        self._text_panel.font_italic_changed.connect(self.font_italic_changed)
        self._text_panel.font_underline_changed.connect(self.font_underline_changed)
        self._stack.addWidget(self._text_panel)

        # Page 4 — pen and airbrush (size + opacity + blend mode buttons)
        self._pen_panel = _PenPanel()
        self._pen_panel.size_changed.connect(self.size_changed)
        self._pen_panel.opacity_changed.connect(self.opacity_changed)
        self._pen_panel.color_modifier_changed.connect(self.color_modifier_changed)
        self._stack.addWidget(self._pen_panel)

        # Page 5 — eraser (brush type combo + size + opacity)
        self._eraser_panel = _EraserPanel()
        self._eraser_panel.eraser_brush_changed.connect(self.eraser_brush_changed)
        self._eraser_panel.size_changed.connect(self.size_changed)
        self._eraser_panel.opacity_changed.connect(self.opacity_changed)
        self._stack.addWidget(self._eraser_panel)

    def set_page_for_tool(self, tool):
        self._stack.setCurrentIndex(_TOOL_PAGE.get(tool, _PAGE_EMPTY))

    def get_size(self):
        p = self._stack.currentIndex()
        if p == _PAGE_BRUSH:
            return self._brush_panel.get_size()
        if p == _PAGE_SHAPE:
            return self._shape_panel.get_size()
        if p == _PAGE_PEN:
            return self._pen_panel.get_size()
        if p == _PAGE_ERASER:
            return self._eraser_panel.get_size()
        return 32

    def set_size(self, v):
        self._brush_panel.set_size(v)
        self._shape_panel.set_size(v)
        self._pen_panel.set_size(v)
        self._eraser_panel.set_size(v)

    def get_opacity(self):
        p = self._stack.currentIndex()
        if p == _PAGE_BRUSH:
            return self._brush_panel.get_opacity()
        if p == _PAGE_SHAPE:
            return self._shape_panel.get_opacity()
        if p == _PAGE_PEN:
            return self._pen_panel.get_opacity()
        if p == _PAGE_ERASER:
            return self._eraser_panel.get_opacity()
        return 50

    def set_opacity(self, v):
        self._brush_panel.set_opacity(v)
        self._shape_panel.set_opacity(v)
        self._pen_panel.set_opacity(v)
        self._eraser_panel.set_opacity(v)

    def get_color_modifier(self):
        return self._pen_panel.get_color_modifier()

    def set_color_modifier(self, mode):
        self._pen_panel.set_color_modifier(mode)

    def set_blend_mode_enabled(self, mode, enabled):
        self._pen_panel.set_blend_mode_enabled(mode, enabled)

    def get_eraser_brush(self):
        return self._eraser_panel.get_eraser_brush()

    def set_eraser_brush(self, brush):
        self._eraser_panel.set_eraser_brush(brush)

    def set_soft_erase_enabled(self, enabled):
        self._eraser_panel.set_soft_erase_enabled(enabled)

    def get_filled(self):
        return self._shape_panel.get_filled()

    def get_font_family(self):
        return self._text_panel.get_font_family()

    def get_font_size(self):
        return self._text_panel.get_font_size()

    def get_bold(self):
        return self._text_panel.get_bold()

    def get_italic(self):
        return self._text_panel.get_italic()

    def get_underline(self):
        return self._text_panel.get_underline()

    def set_font_family(self, name):
        self._text_panel.set_font_family(name)

    def set_font_size(self, size):
        self._text_panel.set_font_size(size)

    def set_bold(self, v):
        self._text_panel.set_bold(v)

    def set_italic(self, v):
        self._text_panel.set_italic(v)

    def set_underline(self, v):
        self._text_panel.set_underline(v)


# ---------------------------------------------------------------------------
# Tool strip
# ---------------------------------------------------------------------------


class AnnotateToolStrip(QtWidgets.QWidget):
    """Narrow vertical column of annotation tool buttons."""

    tool_changed = QtCore.Signal(str)
    undo_requested = QtCore.Signal()
    redo_requested = QtCore.Signal()
    clear_requested = QtCore.Signal()
    clear_all_requested = QtCore.Signal()
    swatch_toggle_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("AnnotateToolStrip")
        self.setFixedWidth(30)

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(0, 8, 0, 8)
        lay.setSpacing(0)

        self._group = QtWidgets.QButtonGroup(self)
        self._buttons = {}

        def _add_tool(tool, grouppos="solo"):
            btn = _tool_button(_TOOL_LABEL[tool], _TOOL_TOOLTIP[tool])
            _apply_icon(btn, tool)
            if grouppos != "solo":
                btn.setProperty("grouppos", grouppos)
            self._buttons[tool] = btn
            self._group.addButton(btn)
            lay.addWidget(btn)

        def _group_of(tools):
            """Add a visually-connected group of tool buttons (1px gap between them)."""
            for i, tool in enumerate(tools):
                if i == 0:
                    pos = "first"
                elif i == len(tools) - 1:
                    pos = "last"
                else:
                    pos = "mid"
                _add_tool(tool, grouppos=pos)
                if i < len(tools) - 1:
                    lay.addSpacing(1)

        # Cursor — standalone
        _add_tool(TOOL_CURSOR)

        lay.addSpacing(1)

        # Drawing group: pen + airbrush + eraser
        _group_of([TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER])

        lay.addSpacing(1)

        # Shapes group: rect + circle + arrow + line
        _group_of([TOOL_RECT, TOOL_CIRCLE, TOOL_ARROW, TOOL_LINE])

        lay.addSpacing(1)

        # Text and eyedropper — standalone
        _add_tool(TOOL_TEXT)
        lay.addSpacing(1)
        _add_tool(TOOL_EYEDROPPER)

        # Fixed gap + divider + swatch (NOT floating — swatch follows tools)
        lay.addSpacing(4)
        lay.addWidget(_separator())
        lay.addSpacing(4)

        self._swatch = ColourSwatch()
        lay.addWidget(self._swatch, alignment=QtCore.Qt.AlignHCenter)
        self._swatch.swatch_clicked.connect(self.swatch_toggle_requested)

        # All remaining space goes here, pushing actions to the bottom
        lay.addStretch()

        lay.addWidget(_separator())
        lay.addSpacing(1)

        self._undo_btn = _tool_button("↩", "Undo", checkable=False)
        self._undo_btn.setObjectName("ActionBtn")
        _apply_icon(self._undo_btn, "undo")
        self._undo_btn.setEnabled(False)
        self._undo_btn.clicked.connect(self.undo_requested)
        lay.addWidget(self._undo_btn)

        lay.addSpacing(1)

        self._redo_btn = _tool_button("↪", "Redo", checkable=False)
        self._redo_btn.setObjectName("ActionBtn")
        _apply_icon(self._redo_btn, "redo")
        self._redo_btn.setEnabled(False)
        self._redo_btn.clicked.connect(self.redo_requested)
        lay.addWidget(self._redo_btn)

        lay.addSpacing(1)

        self._clear_btn = _tool_button("⊗", "Clear Frame", checkable=False)
        self._clear_btn.setObjectName("ActionBtn")
        _apply_icon(self._clear_btn, "clear")
        self._clear_btn.clicked.connect(self._on_clear_clicked)
        lay.addWidget(self._clear_btn)

        self._group.buttonClicked.connect(self._on_tool_clicked)
        self._buttons[TOOL_PEN].setChecked(True)

    def _on_clear_clicked(self):
        menu = QtWidgets.QMenu(self)
        menu.setStyleSheet(_MENU_SS)
        menu.addAction("Clear Frame", self.clear_requested.emit)
        menu.addAction("Clear All Frames on Timeline", self._on_clear_all_confirmed)
        pos = self._clear_btn.mapToGlobal(QtCore.QPoint(self._clear_btn.width() + 2, 0))
        menu.exec_(pos)

    def _on_clear_all_confirmed(self):
        dlg = QtWidgets.QMessageBox(self)
        dlg.setWindowTitle("Clear Annotations")
        dlg.setText("Clear all annotations from the current timeline?")
        dlg.setStandardButtons(QtWidgets.QMessageBox.Cancel | QtWidgets.QMessageBox.Ok)
        dlg.setDefaultButton(QtWidgets.QMessageBox.Cancel)
        if dlg.exec_() == QtWidgets.QMessageBox.Ok:
            self.clear_all_requested.emit()

    def _on_tool_clicked(self, btn):
        for tool, b in self._buttons.items():
            if b is btn:
                self.tool_changed.emit(tool)
                return

    def set_active_tool(self, tool):
        btn = self._buttons.get(tool)
        if btn:
            btn.setChecked(True)

    def set_tool_enabled(self, tool, enabled):
        """Enable or disable a tool button. If the tool is active when disabled, switches to pen."""
        btn = self._buttons.get(tool)
        if btn:
            btn.setEnabled(enabled)
            if not enabled and btn.isChecked():
                self._buttons[TOOL_PEN].setChecked(True)
                self.tool_changed.emit(TOOL_PEN)

    def set_undo_enabled(self, enabled):
        self._undo_btn.setEnabled(enabled)

    def set_redo_enabled(self, enabled):
        self._redo_btn.setEnabled(enabled)

    def set_colour(self, colour):
        """Update the swatch colour display."""
        self._swatch.set_colour(colour)


# ---------------------------------------------------------------------------
# Top-level widget and dock
# ---------------------------------------------------------------------------


class AnnotateToolbarWidget(QtWidgets.QWidget):
    """Combined header + strip + secondary panel.  All signals bubble up from children."""

    tool_changed = QtCore.Signal(str)
    colour_changed = QtCore.Signal(QtGui.QColor)
    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)
    filled_changed = QtCore.Signal(bool)
    color_modifier_changed = QtCore.Signal(str)
    eraser_brush_changed = QtCore.Signal(str)
    font_family_changed = QtCore.Signal(str)
    font_size_changed = QtCore.Signal(str)
    font_bold_changed = QtCore.Signal(bool)
    font_italic_changed = QtCore.Signal(bool)
    font_underline_changed = QtCore.Signal(bool)
    undo_requested = QtCore.Signal()
    redo_requested = QtCore.Signal()
    clear_requested = QtCore.Signal()
    clear_all_requested = QtCore.Signal()
    close_requested = QtCore.Signal()
    dock_requested = QtCore.Signal()
    swatch_toggle_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setStyleSheet(_STRIP_SS + _PANEL_SS)

        outer_lay = QtWidgets.QVBoxLayout(self)
        outer_lay.setContentsMargins(0, 0, 0, 0)
        outer_lay.setSpacing(0)

        # Header bar
        self._header = _ToolbarHeader()
        outer_lay.addWidget(self._header)

        # Strip + panel row
        body = QtWidgets.QWidget()
        lay = QtWidgets.QHBoxLayout(body)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(0)

        self._strip = AnnotateToolStrip()
        lay.addWidget(self._strip)

        self._panel = AnnotateSecondaryPanel()
        lay.addWidget(self._panel)

        outer_lay.addWidget(body)

        # Floating colour picker popup (no parent — truly floating)
        self._picker_popup = ColourPickerPopup()
        self._picker_popup.colour_changed.connect(self._on_colour_changed)

        # Wire header signals
        self._header.close_requested.connect(self.close_requested)
        self._header.dock_requested.connect(self.dock_requested)

        # Wire strip signals
        self._strip.tool_changed.connect(self._on_tool_changed)
        self._strip.undo_requested.connect(self.undo_requested)
        self._strip.redo_requested.connect(self.redo_requested)
        self._strip.clear_requested.connect(self.clear_requested)
        self._strip.clear_all_requested.connect(self.clear_all_requested)
        self._strip.swatch_toggle_requested.connect(self._on_swatch_toggle)

        # Wire panel signals (no colour_changed — that comes from the popup now)
        self._panel.size_changed.connect(self.size_changed)
        self._panel.opacity_changed.connect(self.opacity_changed)
        self._panel.filled_changed.connect(self.filled_changed)
        self._panel.color_modifier_changed.connect(self.color_modifier_changed)
        self._panel.eraser_brush_changed.connect(self.eraser_brush_changed)
        self._panel.font_family_changed.connect(self.font_family_changed)
        self._panel.font_size_changed.connect(self.font_size_changed)
        self._panel.font_bold_changed.connect(self.font_bold_changed)
        self._panel.font_italic_changed.connect(self.font_italic_changed)
        self._panel.font_underline_changed.connect(self.font_underline_changed)

        # Set initial page
        self._panel.set_page_for_tool(TOOL_PEN)

    def _on_tool_changed(self, tool):
        self._panel.set_page_for_tool(tool)
        self.tool_changed.emit(tool)

    def _on_swatch_toggle(self):
        if self._picker_popup.isVisible():
            self._picker_popup.hide()
        else:
            self._picker_popup.show_near(self._strip._swatch)
        self.swatch_toggle_requested.emit()

    def hide_popups(self):
        self._picker_popup.hide()

    def _on_colour_changed(self, colour):
        self._strip.set_colour(colour)
        self.colour_changed.emit(colour)

    def set_colour(self, colour):
        """Programmatically set the active colour (e.g. on tool switch)."""
        self._strip.set_colour(colour)
        self._picker_popup.set_colour(colour)

    def set_size(self, v):
        self._panel.set_size(v)

    def set_opacity(self, v):
        self._panel.set_opacity(v)

    # Passthrough accessors so the mode can read current UI state
    @property
    def panel(self):
        return self._panel

    @property
    def strip(self):
        return self._strip

    def set_undo_enabled(self, v):
        self._strip.set_undo_enabled(v)

    def set_redo_enabled(self, v):
        self._strip.set_redo_enabled(v)

    def set_tool_enabled(self, tool, enabled):
        self._strip.set_tool_enabled(tool, enabled)

    def set_blend_mode_enabled(self, mode, enabled):
        self._panel.set_blend_mode_enabled(mode, enabled)

    def set_soft_erase_enabled(self, enabled):
        self._panel.set_soft_erase_enabled(enabled)


class AnnotateToolbarDockWidget(QtWidgets.QDockWidget):
    """QDockWidget wrapper for the annotation toolbar."""

    def __init__(self, parent=None):
        super().__init__("Annotations", parent)
        self.setFeatures(QtWidgets.QDockWidget.DockWidgetMovable | QtWidgets.QDockWidget.DockWidgetFloatable)
        self.setAllowedAreas(QtCore.Qt.LeftDockWidgetArea | QtCore.Qt.RightDockWidgetArea)
        # Show tooltips even when this window is not the active window (e.g. when floating
        # or when RV's main viewport has focus).
        self.setAttribute(QtCore.Qt.WA_AlwaysShowToolTips)
        self._widget = AnnotateToolbarWidget()
        self.setWidget(self._widget)

    @property
    def toolbar_widget(self):
        return self._widget
