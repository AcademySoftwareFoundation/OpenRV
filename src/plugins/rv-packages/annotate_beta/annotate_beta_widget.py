# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import os

from PySide6 import QtCore, QtWidgets, QtGui

from annotate_beta_color_picker import ColorPickerSection


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

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _tool_button(tooltip, checkable=True, size=30):
    btn = QtWidgets.QToolButton()
    btn.setToolTip(tooltip)
    btn.setCheckable(checkable)
    btn.setFixedSize(size, size)
    btn.setProperty("tbstyle", "palette")
    return btn


def _separator(width=None):
    sep = QtWidgets.QWidget()
    sep.setObjectName("separator")
    sep.setFixedHeight(1)
    if width is not None:
        sep.setFixedWidth(width)
    return sep


class _StyledWidget(QtWidgets.QWidget):
    """QWidget subclass whose QSS background-color is actually painted.

    A plain, non-subclassed QWidget paints its stylesheet background for
    free, but custom QWidget subclasses need Qt.WA_StyledBackground set
    explicitly or the background-color rule is silently ignored.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(QtCore.Qt.WA_StyledBackground, True)


class _PanelSurface(_StyledWidget):
    """Styled widget painted with the panelSurface background color."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("panelSurface")


# ---------------------------------------------------------------------------
# Color swatch
# ---------------------------------------------------------------------------


class ColorSwatch(QtWidgets.QAbstractButton):
    """Square button showing the current annotation color.

    Clicking it emits swatch_clicked — the parent is responsible for
    showing/hiding the inline color picker.
    """

    swatch_clicked = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("colorSwatch")
        self._color = QtGui.QColor(255, 204, 0)
        self.setFixedSize(30, 30)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setToolTip("Color")

    def set_color(self, color):
        self._color = QtGui.QColor(color)
        self.update()

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing, False)
        p.setPen(QtGui.QPen(self.palette().color(self.foregroundRole()), 1))
        p.setBrush(self._color)
        p.drawRect(self.rect().adjusted(1, 1, -1, -1))

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.swatch_clicked.emit()


# ---------------------------------------------------------------------------
# Slider widget
# ---------------------------------------------------------------------------


class _AnnotationSlider(QtWidgets.QSlider):
    """Vertical slider for the size and opacity controls."""

    _WIDTH = 32
    _MIN_HEIGHT = 110

    def __init__(self, min_val=0, max_val=100, default=50, parent=None):
        super().__init__(QtCore.Qt.Vertical, parent)
        self.setObjectName("annotationSlider")
        self.setRange(min_val, max_val)
        self.setValue(max(min_val, min(max_val, default)))
        self.setPageStep(max(1, (max_val - min_val) // 10))
        self.setFixedWidth(self._WIDTH)
        self.setMinimumHeight(self._MIN_HEIGHT)
        self.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Expanding)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

    _HANDLE_LENGTH = 18
    _GROOVE_MARGIN = 3

    def _value_at(self, y):
        """Value under a y position, as if the handle centre were dragged there."""
        span = max(1, self.height() - 2 * self._GROOVE_MARGIN - self._HANDLE_LENGTH)
        bottom = self._GROOVE_MARGIN + self._HANDLE_LENGTH // 2 + span

        return QtWidgets.QStyle.sliderValueFromPosition(self.minimum(), self.maximum(), bottom - int(y), span)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.setSliderDown(True)
            self.setSliderPosition(self._value_at(event.position().y()))
            event.accept()
        else:
            super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self.isSliderDown():
            self.setSliderPosition(self._value_at(event.position().y()))
            event.accept()
        else:
            super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self.isSliderDown():
            self.setSliderDown(False)
            event.accept()
        else:
            super().mouseReleaseEvent(event)


# ---------------------------------------------------------------------------
# Secondary panel option pages
# ---------------------------------------------------------------------------


class _ValueLineEdit(QtWidgets.QLineEdit):
    """QLineEdit that selects all text when it receives focus."""

    def focusInEvent(self, event):
        super().focusInEvent(event)
        QtCore.QTimer.singleShot(0, self.selectAll)


class _SliderSection(_PanelSurface):
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

        self._slider = _AnnotationSlider(min_val, max_val, default)
        self._slider.setToolTip(label)
        self._slider.valueChanged.connect(self._on_slider_changed)
        slider_row = QtWidgets.QHBoxLayout()
        slider_row.setContentsMargins(0, 0, 0, 0)
        slider_row.addStretch()
        slider_row.addWidget(self._slider)
        slider_row.addStretch()
        lay.addLayout(slider_row, 1)

        self._input = _ValueLineEdit(f"{default}{suffix}")
        self._input.setObjectName("sliderValue")
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

    def set_value(self, v):
        self._slider.setValue(v)
        if not self._input.hasFocus():
            self._input.setText(f"{v}{self._suffix}")


class _SizeOpacityPanel(_PanelSurface):
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

        lay.addSpacing(16)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(16)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addStretch()

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


def _load_icon(name):
    """Load a named SVG icon from the package's support files.

    Python files land in PlugIns/Python/ while package support files
    land in PlugIns/SupportFiles/annotate_beta/.
    """
    python_dir = os.path.dirname(os.path.abspath(__file__))
    support_dir = os.path.join(os.path.dirname(python_dir), "SupportFiles", "annotate_beta")
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


class _MenuToolButton(QtWidgets.QToolButton):
    """Tool button with a checkable drop-down menu."""

    selection_changed = QtCore.Signal(str)

    def __init__(self, tooltip, items, icon_size=None, parent=None):
        """items: sequence of (item_id, label, icon_name or None) triples."""
        super().__init__(parent)
        self.setObjectName("menuToolButton")
        self.setProperty("tbstyle", "palette")
        self.setToolTip(tooltip)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        self.setFixedHeight(30)
        self.setPopupMode(QtWidgets.QToolButton.InstantPopup)

        if icon_size is not None:
            self.setIconSize(QtCore.QSize(icon_size, icon_size))

        self._menu = QtWidgets.QMenu(self)
        group = QtGui.QActionGroup(self._menu)
        group.setExclusive(True)
        for item_id, label, icon_name in items:
            action = self._menu.addAction(_load_icon(icon_name), label) if icon_name else self._menu.addAction(label)
            action.setCheckable(True)
            action.setData(item_id)
            group.addAction(action)
        self._menu.triggered.connect(self._on_triggered)
        self.setMenu(self._menu)

    def _on_triggered(self, action):
        item_id = action.data()
        self.set_selection(item_id)
        self.selection_changed.emit(item_id)

    def _action_for(self, item_id):
        return next((a for a in self._menu.actions() if a.data() == item_id), None)

    def has_item(self, item_id):
        return self._action_for(item_id) is not None

    def selection(self):
        action = next((a for a in self._menu.actions() if a.isChecked()), None)
        return action.data() if action is not None else None

    def set_selection(self, item_id):
        """Tick item_id in the menu and show it on the button face."""
        for action in self._menu.actions():
            checked = action.data() == item_id
            action.setChecked(checked)
            if not checked:
                continue
            if action.icon().isNull():
                self.setText(action.text())
            else:
                self.setIcon(action.icon())

    def set_item_enabled(self, item_id, enabled):
        action = self._action_for(item_id)
        if action is not None:
            action.setEnabled(enabled)


class _EraserPanel(_PanelSurface):
    """Brush-type dropdown + size/opacity sliders for the eraser tool."""

    eraser_brush_changed = QtCore.Signal(str)  # "circle" or "gauss"
    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)

    _BRUSHES = (
        ("circle", "Circle (Hard)", "erase_circle"),
        ("gauss", "Gauss (Soft)", "erase_gauss"),
    )

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(0)

        self._brush_btn = _MenuToolButton("Brush Type", self._BRUSHES, icon_size=20)
        self._brush_btn.set_selection("circle")
        self._brush_btn.selection_changed.connect(self.eraser_brush_changed)
        lay.addWidget(self._brush_btn)

        lay.addSpacing(16)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(16)

        self._size = _SliderSection("Size", 1, 100, 32)
        self._size.value_changed.connect(self.size_changed)
        lay.addWidget(self._size)

        lay.addSpacing(16)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(16)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addStretch()

    def set_eraser_brush(self, brush):
        if self._brush_btn.has_item(brush):
            self._brush_btn.set_selection(brush)

    def set_soft_erase_enabled(self, enabled):
        """Enable or disable the Gauss (soft) eraser brush option."""
        self._brush_btn.set_item_enabled("gauss", enabled)
        if not enabled and self._brush_btn.selection() == "gauss":
            self._brush_btn.set_selection("circle")
            self.eraser_brush_changed.emit("circle")

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


class _PenPanel(_PanelSurface):
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

        lay.addSpacing(16)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(16)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addSpacing(16)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(16)

        # Blend mode buttons — grouped with 1px gaps, connected border-radius
        self._blend_grp = QtWidgets.QButtonGroup(self)
        self._blend_btns = {}
        btn_container = _PanelSurface()
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
            btn.setToolTip(tip)
            btn.setProperty("tbstyle", "palette")
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

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


class _ShapeOptionsPanel(_PanelSurface):
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
        lay.addWidget(self._filled)
        lay.addSpacing(8)

        self._size = _SliderSection("Size", 1, 100, 32)
        self._size.value_changed.connect(self.size_changed)
        lay.addWidget(self._size)

        lay.addSpacing(16)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(16)

        self._opacity = _SliderSection("Opacity", 0, 100, 50, suffix="%")
        self._opacity.value_changed.connect(self.opacity_changed)
        lay.addWidget(self._opacity)

        lay.addStretch()

    def set_filled(self, v):
        self._filled.blockSignals(True)
        self._filled.setChecked(v)
        self._filled.blockSignals(False)

    def set_size(self, v):
        self._size.set_value(v)

    def set_opacity(self, v):
        self._opacity.set_value(v)


class _FontNameDelegate(QtWidgets.QStyledItemDelegate):
    """Renders each font name in its own typeface, ticking the current one."""

    def __init__(self, combo, parent=None):
        super().__init__(parent)
        self._combo = combo

    def initStyleOption(self, option, index):
        super().initStyleOption(option, index)
        option.features |= QtWidgets.QStyleOptionViewItem.HasDecoration
        option.decorationSize = QtCore.QSize(12, 12)

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

    def paint(self, painter, option, index):
        super().paint(painter, option, index)
        if index.data() != self._combo.currentText():
            return
        style = QtWidgets.QApplication.style()
        style_option = QtWidgets.QStyleOptionViewItem(option)
        self.initStyleOption(style_option, index)
        style_option.rect = style.subElementRect(QtWidgets.QStyle.SE_ItemViewItemDecoration, style_option, None)
        style.drawPrimitive(QtWidgets.QStyle.PE_IndicatorMenuCheckMark, style_option, painter, None)

    def sizeHint(self, option, index):
        size = super().sizeHint(option, index)
        size.setHeight(option.fontMetrics.height() + 8)
        return size


class _FontComboBox(QtWidgets.QComboBox):
    """Combo box whose closed field always shows "Aa" in the selected font."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.currentTextChanged.connect(self.sync_display_font)

    def sync_display_font(self, family):
        f = self.font()
        f.setFamily(family)
        self.setFont(f)

    def paintEvent(self, event):
        painter = QtWidgets.QStylePainter(self)
        opt = QtWidgets.QStyleOptionComboBox()
        self.initStyleOption(opt)
        opt.currentText = "Aa"
        painter.drawComplexControl(QtWidgets.QStyle.CC_ComboBox, opt)
        painter.drawControl(QtWidgets.QStyle.CE_ComboBoxLabel, opt)


class _TextOptionsPanel(_PanelSurface):
    font_family_changed = QtCore.Signal(str)
    font_size_changed = QtCore.Signal(str)
    font_bold_changed = QtCore.Signal(bool)
    font_italic_changed = QtCore.Signal(bool)
    font_underline_changed = QtCore.Signal(bool)

    _SIZES = (
        ("small", "S", None),
        ("medium", "M", None),
        ("large", "L", None),
    )

    def __init__(self, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(8)

        self._font_combo = _FontComboBox()
        self._font_combo.setToolTip("Font")
        self._font_combo.setItemDelegate(_FontNameDelegate(self._font_combo, self._font_combo))
        for name in QtGui.QFontDatabase.families():
            if not name.startswith(".") and QtGui.QFontDatabase.isSmoothlyScalable(name):
                self._font_combo.addItem(name)
        self._font_combo.setMaxVisibleItems(10)
        self._font_combo.currentTextChanged.connect(self.font_family_changed)
        self._font_combo.view().setMinimumWidth(160)
        lay.addWidget(self._font_combo)

        self._size_btn = _MenuToolButton("Size", self._SIZES)
        self._size_btn.set_selection("medium")
        self._size_btn.selection_changed.connect(self.font_size_changed)
        lay.addWidget(self._size_btn)

        lay.addSpacing(5)
        lay.addWidget(_separator(30), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(5)

        # B / I / U style toggles
        self._style_btns = {}
        for key, label, signal in (
            ("bold", "Bold", self.font_bold_changed),
            ("italic", "Italic", self.font_italic_changed),
            ("underline", "Underline", self.font_underline_changed),
        ):
            btn = _tool_button(label)
            _apply_icon(btn, key)
            btn.toggled.connect(signal)
            lay.addWidget(btn, alignment=QtCore.Qt.AlignHCenter)
            self._style_btns[key] = btn

        lay.addStretch()

    def set_font_family(self, name):
        idx = self._font_combo.findText(name)
        if idx >= 0:
            self._font_combo.blockSignals(True)
            self._font_combo.setCurrentIndex(idx)
            self._font_combo.sync_display_font(name)
            self._font_combo.blockSignals(False)

    def set_font_size(self, size):
        self._size_btn.set_selection(size if self._size_btn.has_item(size) else "medium")

    def _set_style(self, key, v):
        btn = self._style_btns[key]
        btn.blockSignals(True)
        btn.setChecked(v)
        btn.blockSignals(False)

    def set_bold(self, v):
        self._set_style("bold", v)

    def set_italic(self, v):
        self._set_style("italic", v)

    def set_underline(self, v):
        self._set_style("underline", v)


# ---------------------------------------------------------------------------
# Floating color picker popup
# ---------------------------------------------------------------------------


class ColorPickerPopup(QtWidgets.QFrame):
    """Floating color picker that appears to the right of the toolbar."""

    color_changed = QtCore.Signal(QtGui.QColor)

    def __init__(self, parent=None):
        super().__init__(parent, QtCore.Qt.Tool | QtCore.Qt.FramelessWindowHint)
        self.setObjectName("annotationBetaColorPopup")
        self.setAttribute(QtCore.Qt.WA_ShowWithoutActivating)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        self._picker = ColorPickerSection()
        self._picker.color_changed.connect(self.color_changed)
        lay.addWidget(self._picker)
        self.adjustSize()

    def show_near(self, anchor_widget):
        """Position and show the popup to the right of anchor_widget."""
        pos = anchor_widget.mapToGlobal(QtCore.QPoint(anchor_widget.width() + 6, 0))
        self.move(pos)
        self.show()
        self.raise_()

    def set_color(self, c):
        self._picker.set_color(c)


# ---------------------------------------------------------------------------
# Secondary panel
# ---------------------------------------------------------------------------


class AnnotateSecondaryPanel(_StyledWidget):
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
        self.setObjectName("secondaryPanel")
        self.setFixedWidth(80)

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(4, 8, 4, 8)
        lay.setSpacing(0)

        self._stack = QtWidgets.QStackedWidget()
        lay.addWidget(self._stack)

        # Page 0 — empty (cursor / eyedropper)
        self._stack.addWidget(_PanelSurface())

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

    def set_size(self, v):
        self._brush_panel.set_size(v)
        self._shape_panel.set_size(v)
        self._pen_panel.set_size(v)
        self._eraser_panel.set_size(v)

    def set_opacity(self, v):
        self._brush_panel.set_opacity(v)
        self._shape_panel.set_opacity(v)
        self._pen_panel.set_opacity(v)
        self._eraser_panel.set_opacity(v)

    def set_filled(self, v):
        self._shape_panel.set_filled(v)

    def set_color_modifier(self, mode):
        self._pen_panel.set_color_modifier(mode)

    def set_blend_mode_enabled(self, mode, enabled):
        self._pen_panel.set_blend_mode_enabled(mode, enabled)

    def set_eraser_brush(self, brush):
        self._eraser_panel.set_eraser_brush(brush)

    def set_soft_erase_enabled(self, enabled):
        self._eraser_panel.set_soft_erase_enabled(enabled)

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


class AnnotateToolStrip(_StyledWidget):
    """Narrow vertical column of annotation tool buttons."""

    tool_changed = QtCore.Signal(str)
    undo_requested = QtCore.Signal()
    redo_requested = QtCore.Signal()
    clear_requested = QtCore.Signal()
    clear_all_requested = QtCore.Signal()
    swatch_toggle_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("toolStrip")
        self.setFixedWidth(50)

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(8, 8, 8, 8)
        lay.setSpacing(0)

        self._group = QtWidgets.QButtonGroup(self)
        self._buttons = {}

        def _add_tool(tool, grouppos="solo"):
            btn = _tool_button(_TOOL_TOOLTIP[tool])
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

        lay.addSpacing(2)

        # Drawing group: pen + airbrush + eraser
        _group_of([TOOL_PEN, TOOL_AIRBRUSH, TOOL_ERASER])

        lay.addSpacing(2)

        # Shapes group: rect + circle + arrow + line
        _group_of([TOOL_RECT, TOOL_CIRCLE, TOOL_ARROW, TOOL_LINE])

        lay.addSpacing(2)

        # Text and eyedropper — standalone
        _add_tool(TOOL_TEXT)
        lay.addSpacing(2)
        _add_tool(TOOL_EYEDROPPER)

        # Fixed gap + divider + swatch (NOT floating — swatch follows tools)
        lay.addSpacing(10)
        lay.addWidget(_separator(15), alignment=QtCore.Qt.AlignHCenter)
        lay.addSpacing(10)

        self._swatch = ColorSwatch()
        lay.addWidget(self._swatch, alignment=QtCore.Qt.AlignHCenter)
        self._swatch.swatch_clicked.connect(self.swatch_toggle_requested)

        # All remaining space goes here, pushing actions to the bottom
        lay.addStretch()

        self._undo_btn = _tool_button("Undo", checkable=False)
        self._undo_btn.setObjectName("actionButton")
        _apply_icon(self._undo_btn, "undo")
        self._undo_btn.setEnabled(False)
        self._undo_btn.clicked.connect(self.undo_requested)
        lay.addWidget(self._undo_btn)

        lay.addSpacing(1)

        self._redo_btn = _tool_button("Redo", checkable=False)
        self._redo_btn.setObjectName("actionButton")
        _apply_icon(self._redo_btn, "redo")
        self._redo_btn.setEnabled(False)
        self._redo_btn.clicked.connect(self.redo_requested)
        lay.addWidget(self._redo_btn)

        lay.addSpacing(1)

        self._clear_btn = _tool_button("Clear Frame", checkable=False)
        self._clear_btn.setObjectName("actionButton")
        _apply_icon(self._clear_btn, "clear")
        self._clear_btn.clicked.connect(self._on_clear_clicked)
        lay.addWidget(self._clear_btn)

        self._group.buttonClicked.connect(self._on_tool_clicked)
        self._buttons[TOOL_PEN].setChecked(True)

    def _on_clear_clicked(self):
        menu = QtWidgets.QMenu(self)
        menu.addAction("Clear Frame", self.clear_requested.emit)
        menu.addAction("Clear All Frames on Timeline", self._on_clear_all_confirmed)
        pos = self._clear_btn.mapToGlobal(QtCore.QPoint(self._clear_btn.width() + 2, 0))
        menu.exec(pos)

    def _on_clear_all_confirmed(self):
        dlg = QtWidgets.QMessageBox(self)
        dlg.setWindowTitle("Clear Annotations")
        dlg.setText("Clear all annotations from the current timeline?")
        dlg.setStandardButtons(QtWidgets.QMessageBox.Cancel | QtWidgets.QMessageBox.Ok)
        dlg.setDefaultButton(QtWidgets.QMessageBox.Cancel)
        if dlg.exec() == QtWidgets.QMessageBox.Ok:
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

    def set_color(self, color):
        """Update the swatch color display."""
        self._swatch.set_color(color)


# ---------------------------------------------------------------------------
# Top-level widget and dock
# ---------------------------------------------------------------------------


class AnnotateToolbarWidget(QtWidgets.QWidget):
    """Tool strip + secondary panel.  All signals bubble up from children."""

    tool_changed = QtCore.Signal(str)
    color_changed = QtCore.Signal(QtGui.QColor)
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
    swatch_toggle_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("annotationBeta")

        lay = QtWidgets.QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(0)

        self._strip = AnnotateToolStrip()
        lay.addWidget(self._strip)

        self._panel = AnnotateSecondaryPanel()
        lay.addWidget(self._panel)

        # Floating color picker popup (no parent — truly floating)
        self._picker_popup = ColorPickerPopup()
        self._picker_popup.color_changed.connect(self._on_color_changed)

        # Wire strip signals
        self._strip.tool_changed.connect(self._on_tool_changed)
        self._strip.undo_requested.connect(self.undo_requested)
        self._strip.redo_requested.connect(self.redo_requested)
        self._strip.clear_requested.connect(self.clear_requested)
        self._strip.clear_all_requested.connect(self.clear_all_requested)
        self._strip.swatch_toggle_requested.connect(self._on_swatch_toggle)

        # Wire panel signals (no color_changed — that comes from the popup now)
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

    def _on_color_changed(self, color):
        self._strip.set_color(color)
        self.color_changed.emit(color)

    def set_color(self, color):
        """Programmatically set the active color (e.g. on tool switch)."""
        self._strip.set_color(color)
        self._picker_popup.set_color(color)

    def set_size(self, v):
        self._panel.set_size(v)

    def set_opacity(self, v):
        self._panel.set_opacity(v)

    def set_filled(self, v):
        self._panel.set_filled(v)

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

    close_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__("Draw", parent)
        self.setObjectName("annotationBetaDock")
        self.setAllowedAreas(QtCore.Qt.LeftDockWidgetArea | QtCore.Qt.RightDockWidgetArea)
        # Show tooltips even when this window is not the active window (e.g. when floating
        # or when RV's main viewport has focus).
        self.setAttribute(QtCore.Qt.WA_AlwaysShowToolTips)
        self._widget = AnnotateToolbarWidget()
        self.setWidget(self._widget)

    @property
    def toolbar_widget(self):
        return self._widget

    def closeEvent(self, event):
        super().closeEvent(event)
        if event.isAccepted():
            self.close_requested.emit()
