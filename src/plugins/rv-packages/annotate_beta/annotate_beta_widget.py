# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import collections
import math
import os
import sys
import time

from dataclasses import dataclass
from enum import Enum, StrEnum, auto
from functools import partial

from PySide6 import QtCore, QtWidgets, QtGui

from rv import commands

from annotate_beta_color_picker import ColorPickerPopup
import annotate_beta_constants as constants


class _Page(Enum):
    """Secondary panel option page shown for a tool."""

    SIZE_OPACITY = auto()
    SHAPE = auto()
    TEXT = auto()
    BLEND = auto()
    BRUSH = auto()
    ERASER = auto()


@dataclass(frozen=True)
class _ToolInfo:
    page: _Page | None  # None: no option page (cursor, eyedropper)
    tooltip: str


_TOOLS = {
    constants.Tool.CURSOR: _ToolInfo(page=None, tooltip="Cursor"),
    constants.Tool.EYEDROPPER: _ToolInfo(page=None, tooltip="Eyedropper"),
    constants.Tool.PEN: _ToolInfo(page=_Page.BLEND, tooltip="Pen"),
    constants.Tool.BRUSH: _ToolInfo(page=_Page.BRUSH, tooltip="Brush"),
    constants.Tool.ERASER: _ToolInfo(page=_Page.ERASER, tooltip="Eraser"),
    constants.Tool.RECT: _ToolInfo(page=_Page.SHAPE, tooltip="Rectangle"),
    constants.Tool.CIRCLE: _ToolInfo(page=_Page.SHAPE, tooltip="Circle"),
    constants.Tool.ARROW: _ToolInfo(page=_Page.SIZE_OPACITY, tooltip="Arrow"),
    constants.Tool.LINE: _ToolInfo(page=_Page.SIZE_OPACITY, tooltip="Line"),
    constants.Tool.TEXT: _ToolInfo(page=_Page.TEXT, tooltip="Text"),
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _tool_button(tooltip: str = "", checkable: bool = True, icon_name: str | None = None):
    button = QtWidgets.QToolButton()
    button.setToolTip(tooltip)
    button.setCheckable(checkable)
    button.setFixedSize(30, 30)
    button.setIconSize(QtCore.QSize(16, 16))
    button.setProperty("tbstyle", "palette")
    if icon_name:
        button.setIcon(_load_icon(icon_name))
    return button


def _separator(width):
    separator = QtWidgets.QWidget()
    separator.setObjectName("separator")
    separator.setFixedHeight(1)
    separator.setFixedWidth(width)
    return separator


def _insert_divider(layout, index, spacing=16):
    layout.insertSpacing(index, spacing)
    layout.insertWidget(index + 1, _separator(30), alignment=QtCore.Qt.AlignHCenter)
    layout.insertSpacing(index + 2, spacing)


def _add_divider(layout, spacing=16):
    _insert_divider(layout, layout.count(), spacing)


def _group_positions(count):
    """QSS "grouppos" values for a run of visually connected buttons."""
    return ["first", *["mid"] * (count - 2), "last"]


class _StyledWidget(QtWidgets.QWidget):
    """QWidget whose QSS background-color is actually painted."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(QtCore.Qt.WA_StyledBackground, True)


# ---------------------------------------------------------------------------
# Color swatch
# ---------------------------------------------------------------------------


class ColorSwatch(QtWidgets.QAbstractButton):
    """Square button showing the current annotation color.

    AnnotateToolbarWidget shows/hides the color picker popup when it is clicked.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("colorSwatch")
        self._color = QtGui.QColor(constants.DEFAULT_COLOR)
        self.setFixedSize(30, 30)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setToolTip("Color")

    def set_color(self, color):
        self._color = QtGui.QColor(color)
        self.update()

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, False)
        painter.setPen(QtGui.QPen(self.palette().color(self.foregroundRole()), 1))
        painter.setBrush(self._color)
        painter.drawRect(self.rect().adjusted(1, 1, -1, -1))


# ---------------------------------------------------------------------------
# Slider widget
# ---------------------------------------------------------------------------


class _AnnotationSlider(QtWidgets.QSlider):
    """Slider for the size, opacity and brush settings controls."""

    _HANDLE_LENGTH = 18

    def __init__(self, minimum=0, maximum=100, default=50, orientation=QtCore.Qt.Vertical, parent=None):
        super().__init__(orientation, parent)
        self.setObjectName("annotationSlider")
        self.setRange(minimum, maximum)
        self.setValue(max(minimum, min(maximum, default)))
        self.setPageStep(max(1, (maximum - minimum) // 10))
        if orientation == QtCore.Qt.Vertical:
            self.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Expanding)
        else:
            self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

    def _value_at(self, point):
        if self.orientation() == QtCore.Qt.Vertical:
            span = max(1, self.height() - self._HANDLE_LENGTH)
            position = self._HANDLE_LENGTH // 2 + span - int(point.y())
        else:
            span = max(1, self.width() - self._HANDLE_LENGTH)
            position = int(point.x()) - self._HANDLE_LENGTH // 2

        return QtWidgets.QStyle.sliderValueFromPosition(self.minimum(), self.maximum(), position, span)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.setSliderDown(True)
            self.setSliderPosition(self._value_at(event.position()))
            event.accept()
        else:
            super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self.isSliderDown():
            self.setSliderPosition(self._value_at(event.position()))
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


class _SliderSection(QtWidgets.QWidget):
    """Slider + editable value input: stacked when vertical, labelled row when horizontal."""

    value_changed = QtCore.Signal(int)

    def __init__(self, label, minimum, maximum, default, suffix="", orientation=QtCore.Qt.Vertical, parent=None):
        super().__init__(parent)
        self._suffix = suffix
        self._minimum = minimum
        self._maximum = maximum

        self._slider = _AnnotationSlider(minimum, maximum, default, orientation)
        self._slider.setToolTip(label)
        self._slider.valueChanged.connect(self._on_slider_changed)

        self._input = _ValueLineEdit(f"{default}{suffix}")
        self._input.setObjectName("sliderValue")
        self._input.setToolTip(label)
        self._input.setFixedWidth(48)
        self._input.editingFinished.connect(self._on_input_committed)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        if orientation == QtCore.Qt.Vertical:
            layout.setSpacing(4)
            slider_row = QtWidgets.QHBoxLayout()
            slider_row.setContentsMargins(0, 0, 0, 0)
            slider_row.addStretch()
            slider_row.addWidget(self._slider)
            slider_row.addStretch()
            layout.addLayout(slider_row, 1)
            layout.addWidget(self._input, alignment=QtCore.Qt.AlignHCenter)
        else:
            layout.setSpacing(0)
            title = QtWidgets.QLabel(label)
            title.setObjectName("sliderSectionTitle")
            layout.addWidget(title)
            row = QtWidgets.QHBoxLayout()
            row.setContentsMargins(0, 0, 0, 0)
            row.setSpacing(4)
            row.addWidget(self._slider, 1)
            row.addWidget(self._input)
            layout.addLayout(row)

        # Prevents sections in less-constrained panels (no blend buttons) from
        # claiming leftover space. Preferred has GrowFlag set
        # and quietly absorbs extra space; Fixed (no flags) takes exactly sizeHint.
        self.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)

    def _on_slider_changed(self, value):
        if not self._input.hasFocus():
            self._input.setText(f"{value}{self._suffix}")
        self.value_changed.emit(value)

    def _on_input_committed(self):
        text = self._input.text().strip()
        if self._suffix and text.endswith(self._suffix):
            text = text[: -len(self._suffix)].strip()
        try:
            value = max(self._minimum, min(self._maximum, int(round(float(text)))))
        except (ValueError, OverflowError):
            self._input.setText(f"{self._slider.value()}{self._suffix}")
            return
        self._slider.setValue(value)
        self._input.setText(f"{value}{self._suffix}")

    def set_value(self, value):
        with QtCore.QSignalBlocker(self._slider):
            self._slider.setValue(value)
        if not self._input.hasFocus():
            self._input.setText(f"{value}{self._suffix}")


class _SliderPanel(QtWidgets.QWidget):
    """Option page with size and opacity sliders; subclasses add their own controls around them."""

    size_changed = QtCore.Signal(int)
    opacity_changed = QtCore.Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._layout = QtWidgets.QVBoxLayout(self)
        self._layout.setContentsMargins(8, 8, 8, 8)
        self._layout.setSpacing(0)

        self._size = _SliderSection("Size", constants.SIZE_MIN, constants.SIZE_MAX, constants.DEFAULT_SIZE)
        self._size.value_changed.connect(self.size_changed)
        self._opacity = _SliderSection(
            "Opacity", constants.OPACITY_MIN, constants.OPACITY_MAX, constants.DEFAULT_OPACITY, suffix="%"
        )
        self._opacity.value_changed.connect(self.opacity_changed)

    def _add_sliders(self):
        self._layout.addWidget(self._size)
        _add_divider(self._layout)
        self._layout.addWidget(self._opacity)

    def set_size(self, value):
        self._size.set_value(value)

    def set_opacity(self, value):
        self._opacity.set_value(value)


class _SizeOpacityPanel(_SliderPanel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._add_sliders()
        self._layout.addStretch()


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


class _PopupToolButton(QtWidgets.QToolButton):
    """Tool button that opens its menu on click."""

    def __init__(self, tooltip, parent=None):
        super().__init__(parent)
        self.setObjectName("menuToolButton")
        self.setProperty("tbstyle", "palette")
        self.setToolTip(tooltip)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        self.setFixedHeight(30)
        self.setPopupMode(QtWidgets.QToolButton.InstantPopup)


class _MenuToolButton(_PopupToolButton):
    """Tool button with a checkable drop-down menu."""

    selection_changed = QtCore.Signal(str)

    def __init__(self, tooltip, items, icon_size=None, parent=None):
        super().__init__(tooltip, parent)

        if icon_size is not None:
            self.setIconSize(QtCore.QSize(icon_size, icon_size))

        self._selection = None
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
        if item_id == self._selection:
            return
        self.set_selection(item_id)
        self.selection_changed.emit(item_id)

    def _action_for(self, item_id):
        return next((action for action in self._menu.actions() if action.data() == item_id), None)

    def has_item(self, item_id):
        return self._action_for(item_id) is not None

    def selection(self):
        return self._selection

    def set_selection(self, item_id):
        action = self._action_for(item_id)
        if action is None:
            return

        self._selection = item_id
        action.setChecked(True)
        if action.icon().isNull():
            self.setText(action.text())
        else:
            self.setIcon(action.icon())

    def set_item_enabled(self, item_id, enabled):
        action = self._action_for(item_id)
        if action is not None:
            action.setEnabled(enabled)


class _EraserPanel(_SliderPanel):
    """Brush-type dropdown + size/opacity sliders for the eraser tool."""

    eraser_brush_changed = QtCore.Signal(str)  # Brush value

    _BRUSHES = (
        (constants.Brush.CIRCLE, "Circle (Hard)", "erase_circle"),
        (constants.Brush.GAUSS, "Gauss (Soft)", "erase_gauss"),
    )

    def __init__(self, parent=None):
        super().__init__(parent)
        self._brush_button = _MenuToolButton("Brush Type", self._BRUSHES, icon_size=20)
        self._brush_button.set_selection(constants.Brush.CIRCLE)
        self._brush_button.selection_changed.connect(self.eraser_brush_changed)
        self._layout.addWidget(self._brush_button)
        _add_divider(self._layout)
        self._add_sliders()
        self._layout.addStretch()

    def set_eraser_brush(self, brush):
        self._brush_button.set_selection(brush)

    def set_soft_erase_enabled(self, enabled):
        """Enable or disable the Gauss (soft) eraser brush option."""
        self._brush_button.set_item_enabled(constants.Brush.GAUSS, enabled)
        if not enabled and self._brush_button.selection() == constants.Brush.GAUSS:
            self._brush_button.set_selection(constants.Brush.CIRCLE)
            self.eraser_brush_changed.emit(constants.Brush.CIRCLE)


class _BlendModeGroup(QtWidgets.QWidget):
    """Normal / Burn / Dodge blend mode buttons."""

    color_modifier_changed = QtCore.Signal(str)  # ColorModifier value

    _BLEND_MODES = (
        (constants.ColorModifier.NORMAL, "normal", "Normal"),
        (constants.ColorModifier.DARKEN, "burn", "Burn"),
        (constants.ColorModifier.ADDITIVE, "dodge", "Dodge"),
    )

    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(1)

        self._group = QtWidgets.QButtonGroup(self)
        self._buttons = {}
        positions = _group_positions(len(self._BLEND_MODES))
        for (key, icon_name, tooltip), position in zip(self._BLEND_MODES, positions, strict=True):
            button = _tool_button(tooltip, icon_name=icon_name)
            button.setProperty("grouppos", position)
            button.clicked.connect(partial(self.color_modifier_changed.emit, key))
            self._group.addButton(button)
            self._buttons[key] = button
            layout.addWidget(button, alignment=QtCore.Qt.AlignHCenter)
        self._buttons[constants.ColorModifier.NORMAL].setChecked(True)

    def set_color_modifier(self, mode):
        button = self._buttons.get(mode)
        if button:
            button.setChecked(True)

    def set_blend_mode_enabled(self, mode, enabled):
        """Enable or disable a blend mode button (e.g. burn/dodge) by its key."""
        button = self._buttons.get(mode)
        if button:
            button.setEnabled(enabled)
            if not enabled and button.isChecked():
                self._buttons[constants.ColorModifier.NORMAL].setChecked(True)
                self.color_modifier_changed.emit(constants.ColorModifier.NORMAL)


class _PenPanel(_SliderPanel):
    """Size and opacity sliders, and the blend mode buttons."""

    color_modifier_changed = QtCore.Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._add_sliders()
        _add_divider(self._layout)
        self._blend_modes = _BlendModeGroup()
        self._blend_modes.color_modifier_changed.connect(self.color_modifier_changed)
        self._layout.addWidget(self._blend_modes)
        self._layout.addStretch()

    def set_color_modifier(self, mode):
        self._blend_modes.set_color_modifier(mode)

    def set_blend_mode_enabled(self, mode, enabled):
        self._blend_modes.set_blend_mode_enabled(mode, enabled)


class _BrushSettingGroup(StrEnum):
    BRUSH = "BRUSH"
    VARIATION = "VARIATION"


@dataclass(frozen=True)
class _BrushSettingInfo:
    group: _BrushSettingGroup
    label: str


_BRUSH_SETTINGS = {
    constants.BrushSetting.HARDNESS: _BrushSettingInfo(_BrushSettingGroup.BRUSH, "Hardness"),
    constants.BrushSetting.SPACING: _BrushSettingInfo(_BrushSettingGroup.BRUSH, "Spacing"),
    constants.BrushSetting.SIZE_VARIATION: _BrushSettingInfo(_BrushSettingGroup.VARIATION, "Size"),
    constants.BrushSetting.OPACITY_VARIATION: _BrushSettingInfo(_BrushSettingGroup.VARIATION, "Opacity"),
    constants.BrushSetting.ROTATION_VARIATION: _BrushSettingInfo(_BrushSettingGroup.VARIATION, "Rotation"),
    constants.BrushSetting.SPACING_VARIATION: _BrushSettingInfo(_BrushSettingGroup.VARIATION, "Spacing"),
}


class _BrushSettings(QtWidgets.QWidget):
    """Titled group of brush settings sliders shown in the brush settings dropdown."""

    def __init__(self, group, parent=None):
        super().__init__(parent)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(8, 4, 8, 4)
        layout.setSpacing(1)

        title = QtWidgets.QLabel(group)
        title.setObjectName("brushSettingsGroupTitle")
        layout.addWidget(title)
        layout.addSpacing(12)

        self._sections = {}
        for setting, info in _BRUSH_SETTINGS.items():
            if info.group is not group:
                continue
            section = _SliderSection(
                info.label,
                constants.BRUSH_SETTING_MIN,
                constants.BRUSH_SETTING_MAX,
                constants.DEFAULT_BRUSH_SETTINGS[setting],
                suffix="%",
                orientation=QtCore.Qt.Horizontal,
            )
            layout.addWidget(section)
            self._sections[setting] = section

    def reset(self):
        for setting, section in self._sections.items():
            section.set_value(constants.DEFAULT_BRUSH_SETTINGS[setting])


class _BrushSettingsButton(_PopupToolButton):
    """Tool button whose drop-down holds the brush settings sliders and a reset action."""

    def __init__(self, parent=None):
        super().__init__("Brush settings", parent)
        self.setIcon(_load_icon("brush_settings"))
        self.setIconSize(QtCore.QSize(16, 16))

        menu = QtWidgets.QMenu(self)
        menu.setObjectName("brushSettingsMenu")
        self._groups = []
        for group in _BrushSettingGroup:
            settings = _BrushSettings(group)
            settings_action = QtWidgets.QWidgetAction(menu)
            settings_action.setDefaultWidget(settings)
            menu.addAction(settings_action)
            menu.addSeparator()
            self._groups.append(settings)
        menu.addAction("Reset all to default", self._reset)
        self.setMenu(menu)

    def _reset(self):
        for settings in self._groups:
            settings.reset()


class _WidePopupComboBox(QtWidgets.QComboBox):
    """Combo box whose popup is as wide as its widest item, even when the closed field is narrow."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.view().setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)

    def showPopup(self):
        view = self.view()
        scrollbar_width = view.verticalScrollBar().sizeHint().width()
        view.setMinimumWidth(view.sizeHintForColumn(0) + scrollbar_width + 2 * view.frameWidth())
        super().showPopup()


class _BrushTypeComboBox(_WidePopupComboBox):
    """Combo box whose closed field shows the selected brush's image instead of its name."""

    _STAMP_SIZE = 10
    _STROKE_STEP = _STAMP_SIZE // 4
    _STROKE_AMPLITUDE = _STAMP_SIZE // 2
    _STROKE_COUNT = 40

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setToolTip("Brush types")
        self._icon = _load_icon("erase_gauss")  # placeholder icon
        self.setItemDelegate(_BrushItemDelegate(self))
        preview = self._paint_stroke_preview()
        for name in ["BrushNameHere"] * 20:  # placeholder brush items
            self.addItem(name)
            self.setItemData(self.count() - 1, preview, _BrushItemDelegate.PREVIEW_ROLE)

    def _paint_stroke_preview(self):
        device_pixel_ratio = self.devicePixelRatioF()
        width = (self._STROKE_COUNT - 1) * self._STROKE_STEP + self._STAMP_SIZE
        height = self._STAMP_SIZE + 2 * self._STROKE_AMPLITUDE
        pixmap = QtGui.QPixmap(round(width * device_pixel_ratio), round(height * device_pixel_ratio))
        pixmap.setDevicePixelRatio(device_pixel_ratio)
        pixmap.fill(QtCore.Qt.transparent)
        painter = QtGui.QPainter(pixmap)
        for i in range(self._STROKE_COUNT):
            x = i * self._STROKE_STEP
            y = self._STROKE_AMPLITUDE - round(
                self._STROKE_AMPLITUDE * math.sin(2 * math.pi * i / (self._STROKE_COUNT - 1))
            )
            self._icon.paint(painter, QtCore.QRect(x, y, self._STAMP_SIZE, self._STAMP_SIZE))
        painter.end()
        return pixmap

    def paintEvent(self, event):
        painter = QtWidgets.QStylePainter(self)
        style_option = QtWidgets.QStyleOptionComboBox()
        self.initStyleOption(style_option)
        style_option.currentText = ""
        style_option.currentIcon = self._icon
        painter.drawComplexControl(QtWidgets.QStyle.CC_ComboBox, style_option)
        painter.drawControl(QtWidgets.QStyle.CE_ComboBoxLabel, style_option)


class _BrushPanel(_PenPanel):
    """Brush type and settings dropdowns, size and opacity sliders, and the blend mode buttons."""

    def __init__(self, parent=None):
        super().__init__(parent)
        _insert_divider(self._layout, 0)
        self._layout.insertWidget(0, _BrushSettingsButton())
        self._layout.insertSpacing(0, 8)
        self._layout.insertWidget(0, _BrushTypeComboBox())


class _ShapeOptionsPanel(_SliderPanel):
    filled_changed = QtCore.Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._filled = QtWidgets.QCheckBox("Filled")
        self._filled.toggled.connect(self.filled_changed)
        self._layout.addWidget(self._filled)
        self._layout.addSpacing(8)
        self._add_sliders()
        self._layout.addStretch()

    def set_filled(self, value):
        with QtCore.QSignalBlocker(self._filled):
            self._filled.setChecked(value)


class _CheckedItemDelegate(QtWidgets.QStyledItemDelegate):
    """Ticks the current item of the combo box it belongs to."""

    def initStyleOption(self, option, index):
        super().initStyleOption(option, index)
        option.features |= QtWidgets.QStyleOptionViewItem.HasDecoration
        option.decorationSize = QtCore.QSize(12, 12)

    def paint(self, painter, option, index):
        super().paint(painter, option, index)
        if index.row() != self.parent().currentIndex():
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


class _BrushItemDelegate(_CheckedItemDelegate):
    """Draws the brush name with its preview stroke underneath."""

    PREVIEW_ROLE = QtCore.Qt.UserRole

    def paint(self, painter, option, index):
        style = QtWidgets.QApplication.style()
        style.drawPrimitive(QtWidgets.QStyle.PE_PanelItemViewItem, option, painter, option.widget)
        preview = index.data(self.PREVIEW_ROLE)
        size = preview.deviceIndependentSize().toSize()
        name_option = QtWidgets.QStyleOptionViewItem(option)
        name_option.rect.setHeight(option.rect.height() - size.height())
        super().paint(painter, name_option, index)
        self.initStyleOption(name_option, index)
        text_rect = style.subElementRect(QtWidgets.QStyle.SE_ItemViewItemText, name_option, None)
        x = text_rect.left() + (text_rect.width() - size.width()) // 2
        painter.drawPixmap(x, name_option.rect.bottom() + 1, preview)

    def sizeHint(self, option, index):
        size = super().sizeHint(option, index)
        preview_size = index.data(self.PREVIEW_ROLE).deviceIndependentSize().toSize()
        return QtCore.QSize(max(size.width(), preview_size.width()), size.height() + preview_size.height())


class _FontNameDelegate(_CheckedItemDelegate):
    """Renders each font name in its own typeface."""

    def initStyleOption(self, option, index):
        super().initStyleOption(option, index)

        font_name = index.data()
        if font_name:
            font = QtGui.QFont(font_name)
            point_size = option.font.pointSize()
            if point_size > 0:
                font.setPointSize(point_size)
            else:
                pixel_size = option.font.pixelSize()
                if pixel_size > 0:
                    font.setPixelSize(pixel_size)
            option.font = font


class _FontComboBox(_WidePopupComboBox):
    """Combo box whose closed field always shows "Aa" in the selected font."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.currentTextChanged.connect(self.sync_display_font)

    def sync_display_font(self, family):
        font = self.font()
        font.setFamily(family)
        self.setFont(font)

    def paintEvent(self, event):
        painter = QtWidgets.QStylePainter(self)
        style_option = QtWidgets.QStyleOptionComboBox()
        self.initStyleOption(style_option)
        style_option.currentText = "Aa"
        painter.drawComplexControl(QtWidgets.QStyle.CC_ComboBox, style_option)
        painter.drawControl(QtWidgets.QStyle.CE_ComboBoxLabel, style_option)


class _TextOptionsPanel(QtWidgets.QWidget):
    font_family_changed = QtCore.Signal(str)
    font_size_changed = QtCore.Signal(str)
    font_bold_changed = QtCore.Signal(bool)
    font_italic_changed = QtCore.Signal(bool)
    font_underline_changed = QtCore.Signal(bool)
    opacity_changed = QtCore.Signal(int)

    _SIZES = (
        (constants.FontSize.SMALL, "S", None),
        (constants.FontSize.MEDIUM, "M", None),
        (constants.FontSize.LARGE, "L", None),
    )

    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)

        self._pending_family = None
        self._font_queue = None
        self._font_timer = None

        self._font_combo = _FontComboBox()
        self._font_combo.setToolTip("Font")
        self._font_combo.setItemDelegate(_FontNameDelegate(self._font_combo))
        self._font_combo.setMaxVisibleItems(10)
        # A placeholder keeps the first loaded font from being auto-selected.
        self._font_combo.setPlaceholderText("Aa")
        self._font_combo.currentTextChanged.connect(self._on_font_selected)
        self._font_combo.currentTextChanged.connect(self.font_family_changed)
        layout.addWidget(self._font_combo)

        self._size_button = _MenuToolButton("Size", self._SIZES)
        self._size_button.set_selection(constants.FontSize.MEDIUM)
        self._size_button.selection_changed.connect(self.font_size_changed)
        layout.addWidget(self._size_button)

        _add_divider(layout, spacing=5)

        self._style_buttons = {}
        for icon_name, label, signal in (
            ("bold", "Bold", self.font_bold_changed),
            ("italic", "Italic", self.font_italic_changed),
            ("underline", "Underline", self.font_underline_changed),
        ):
            button = _tool_button(label, icon_name=icon_name)
            button.toggled.connect(signal)
            layout.addWidget(button, alignment=QtCore.Qt.AlignHCenter)
            self._style_buttons[icon_name] = button

        _add_divider(layout, spacing=5)

        self._opacity = _SliderSection(
            "Opacity", constants.OPACITY_MIN, constants.OPACITY_MAX, constants.DEFAULT_OPACITY, suffix="%"
        )
        self._opacity.value_changed.connect(self.opacity_changed)
        layout.addWidget(self._opacity)

        layout.addStretch()

    def start_loading_fonts(self):
        """Fill the font combo in time-boxed batches so the GUI stays responsive."""
        if self._font_queue is not None:
            return
        self._font_queue = collections.deque(
            name for name in QtGui.QFontDatabase.families() if not name.startswith(".")
        )
        self._font_timer = QtCore.QTimer(self)
        self._font_timer.timeout.connect(self._load_font_batch)
        self._font_timer.start(0)

    def _load_font_batch(self):
        # isSmoothlyScalable() is the slow call: it resolves every style of the family.
        deadline = time.perf_counter() + constants.FONT_BATCH_TIME_BUDGET_SECONDS
        combo = self._font_combo
        with QtCore.QSignalBlocker(combo):
            while self._font_queue and time.perf_counter() < deadline:
                name = self._font_queue.popleft()
                # Skip fonts without an 'hhea' table (e.g. macOS "GB18030 Bitmap"): Qt warns
                # it can't compute their bearings when laying out text.
                if QtGui.QFontDatabase.isSmoothlyScalable(name) and QtGui.QRawFont.fromFont(
                    QtGui.QFont(name)
                ).fontTable("hhea"):
                    combo.addItem(name)
                    if name == self._pending_family:
                        combo.setCurrentIndex(combo.count() - 1)
        if not self._font_queue:
            self._font_timer.stop()
            self._font_timer.deleteLater()
            self._font_timer = None

    def _on_font_selected(self, name):
        # Only user selections reach here: programmatic changes block signals.
        self._pending_family = name

    def set_font_family(self, name):
        self._pending_family = name
        index = self._font_combo.findText(name)
        if index >= 0:
            with QtCore.QSignalBlocker(self._font_combo):
                self._font_combo.setCurrentIndex(index)
        self._font_combo.sync_display_font(name)

    def set_font_size(self, size):
        self._size_button.set_selection(size if self._size_button.has_item(size) else constants.FontSize.MEDIUM)

    def _set_style(self, key, value):
        with QtCore.QSignalBlocker(self._style_buttons[key]):
            self._style_buttons[key].setChecked(value)

    def set_bold(self, value):
        self._set_style("bold", value)

    def set_italic(self, value):
        self._set_style("italic", value)

    def set_underline(self, value):
        self._set_style("underline", value)

    def set_opacity(self, value):
        self._opacity.set_value(value)


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

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(4, 8, 4, 8)
        layout.setSpacing(0)

        self._stack = QtWidgets.QStackedWidget()
        layout.addWidget(self._stack)

        self._size_opacity_panel = _SizeOpacityPanel()  # arrow, line
        self._shape_panel = _ShapeOptionsPanel()  # rect, circle
        self._text_panel = _TextOptionsPanel()
        self._pen_panel = _PenPanel()
        self._brush_panel = _BrushPanel()
        self._eraser_panel = _EraserPanel()
        self._pages = {
            _Page.SIZE_OPACITY: self._size_opacity_panel,
            _Page.SHAPE: self._shape_panel,
            _Page.TEXT: self._text_panel,
            _Page.BLEND: self._pen_panel,
            _Page.BRUSH: self._brush_panel,
            _Page.ERASER: self._eraser_panel,
        }
        for page in self._pages.values():
            self._stack.addWidget(page)

        self._slider_panels = (
            self._size_opacity_panel,
            self._shape_panel,
            self._pen_panel,
            self._brush_panel,
            self._eraser_panel,
        )
        for panel in self._slider_panels:
            panel.size_changed.connect(self.size_changed)
            panel.opacity_changed.connect(self.opacity_changed)
        self._blend_panels = (self._pen_panel, self._brush_panel)
        for panel in self._blend_panels:
            panel.color_modifier_changed.connect(partial(self._on_color_modifier_changed, panel))
        self._shape_panel.filled_changed.connect(self.filled_changed)
        self._eraser_panel.eraser_brush_changed.connect(self.eraser_brush_changed)
        self._text_panel.font_family_changed.connect(self.font_family_changed)
        self._text_panel.font_size_changed.connect(self.font_size_changed)
        self._text_panel.font_bold_changed.connect(self.font_bold_changed)
        self._text_panel.font_italic_changed.connect(self.font_italic_changed)
        self._text_panel.font_underline_changed.connect(self.font_underline_changed)
        self._text_panel.opacity_changed.connect(self.opacity_changed)

    def _on_color_modifier_changed(self, panel, mode):
        if self._stack.currentWidget() is panel:
            self.color_modifier_changed.emit(mode)

    def set_page_for_tool(self, tool):
        page = _TOOLS[tool].page
        self._stack.setVisible(page is not None)
        if page is not None:
            self._stack.setCurrentWidget(self._pages[page])

    def showEvent(self, event):
        super().showEvent(event)
        # Deferred so the dock paints before the font list starts loading.
        QtCore.QTimer.singleShot(0, self._text_panel.start_loading_fonts)

    def set_size(self, value):
        for panel in self._slider_panels:
            panel.set_size(value)

    def set_opacity(self, value):
        for panel in self._slider_panels:
            panel.set_opacity(value)
        self._text_panel.set_opacity(value)

    def set_filled(self, value):
        self._shape_panel.set_filled(value)

    def set_color_modifier(self, mode):
        for panel in self._blend_panels:
            panel.set_color_modifier(mode)

    def set_blend_mode_enabled(self, mode, enabled):
        for panel in self._blend_panels:
            panel.set_blend_mode_enabled(mode, enabled)

    def set_eraser_brush(self, brush):
        self._eraser_panel.set_eraser_brush(brush)

    def set_soft_erase_enabled(self, enabled):
        self._eraser_panel.set_soft_erase_enabled(enabled)

    def set_font_family(self, name):
        self._text_panel.set_font_family(name)

    def set_font_size(self, size):
        self._text_panel.set_font_size(size)

    def set_bold(self, value):
        self._text_panel.set_bold(value)

    def set_italic(self, value):
        self._text_panel.set_italic(value)

    def set_underline(self, value):
        self._text_panel.set_underline(value)


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

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("toolStrip")
        self.setFixedWidth(50)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(0)

        self._group = QtWidgets.QButtonGroup(self)
        self._buttons = {}

        def _add_tool(tool, grouppos=None):
            button = _tool_button(_TOOLS[tool].tooltip, icon_name=tool)
            if grouppos:
                button.setProperty("grouppos", grouppos)
            button.clicked.connect(partial(self.tool_changed.emit, tool))
            self._buttons[tool] = button
            self._group.addButton(button)
            layout.addWidget(button)

        def _group_of(tools):
            """Add a visually-connected group of tool buttons (1px gap between them)."""
            for index, (tool, group_position) in enumerate(zip(tools, _group_positions(len(tools)))):
                if index:
                    layout.addSpacing(1)
                _add_tool(tool, grouppos=group_position)

        _add_tool(constants.Tool.CURSOR)

        layout.addSpacing(2)

        _group_of([constants.Tool.PEN, constants.Tool.BRUSH, constants.Tool.ERASER])

        layout.addSpacing(2)

        _group_of([constants.Tool.RECT, constants.Tool.CIRCLE, constants.Tool.ARROW, constants.Tool.LINE])

        layout.addSpacing(2)

        _add_tool(constants.Tool.TEXT)
        layout.addSpacing(2)
        _add_tool(constants.Tool.EYEDROPPER)

        layout.addSpacing(10)
        layout.addWidget(_separator(15), alignment=QtCore.Qt.AlignHCenter)
        layout.addSpacing(10)

        self.swatch = ColorSwatch()
        layout.addWidget(self.swatch, alignment=QtCore.Qt.AlignHCenter)

        layout.addStretch()

        self._undo_action = QtGui.QAction(_load_icon("undo"), "Undo", self)
        self._undo_action.setShortcut(QtGui.QKeySequence.StandardKey.Undo)
        self._undo_action.setShortcutContext(QtCore.Qt.ApplicationShortcut)
        self._undo_action.setEnabled(False)
        self._undo_action.triggered.connect(self.undo_requested)

        self._undo_button = _tool_button(checkable=False)
        self._undo_button.setDefaultAction(self._undo_action)
        self._undo_button.setObjectName("actionButton")
        layout.addWidget(self._undo_button)

        layout.addSpacing(1)

        self._redo_action = QtGui.QAction(_load_icon("redo"), "Redo", self)
        if sys.platform == "win32":
            self._redo_action.setShortcuts([QtGui.QKeySequence("Ctrl+Y"), QtGui.QKeySequence("Ctrl+Shift+Z")])
        else:
            self._redo_action.setShortcut(QtGui.QKeySequence.StandardKey.Redo)
        self._redo_action.setShortcutContext(QtCore.Qt.ApplicationShortcut)
        self._redo_action.setEnabled(False)
        self._redo_action.triggered.connect(self.redo_requested)

        self._redo_button = _tool_button(checkable=False)
        self._redo_button.setDefaultAction(self._redo_action)
        self._redo_button.setObjectName("actionButton")
        layout.addWidget(self._redo_button)

        layout.addSpacing(1)

        self._clear_frame_action = QtGui.QAction("Clear Frame", self)
        self._clear_frame_action.triggered.connect(self.clear_requested)
        self._clear_all_frames_action = QtGui.QAction("Clear All Frames on Timeline", self)
        self._clear_all_frames_action.triggered.connect(self._on_clear_all_triggered)

        self._clear_button = _tool_button("Clear Frame", checkable=False, icon_name="clear")
        self._clear_button.setObjectName("actionButton")
        self._clear_button.setEnabled(False)

        clear_menu = QtWidgets.QMenu(self._clear_button)
        clear_menu.addActions([self._clear_frame_action, self._clear_all_frames_action])
        self._clear_button.setMenu(clear_menu)
        self._clear_button.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        layout.addWidget(self._clear_button)

        self._buttons[constants.Tool.PEN].setChecked(True)

    def _on_clear_all_triggered(self):
        choice = commands.alertPanel(
            True, commands.InfoAlert, "Clear all annotations from the current timeline?", None, "OK", "Cancel", None
        )
        if choice == 0:
            self.clear_all_requested.emit()

    def set_active_tool(self, tool):
        button = self._buttons.get(tool)
        if button:
            button.setChecked(True)

    def set_tool_enabled(self, tool, enabled):
        """Enable or disable a tool button. If the tool is active when disabled, switches to pen."""
        button = self._buttons.get(tool)
        if button:
            button.setEnabled(enabled)
            if not enabled and button.isChecked():
                self._buttons[constants.Tool.PEN].setChecked(True)
                self.tool_changed.emit(constants.Tool.PEN)

    def set_undo_enabled(self, enabled):
        self._undo_action.setEnabled(enabled)

    def set_redo_enabled(self, enabled):
        self._redo_action.setEnabled(enabled)

    def set_clear_enabled(self, clear_frame_enabled, clear_all_frames_enabled):
        self._clear_frame_action.setEnabled(clear_frame_enabled)
        self._clear_all_frames_action.setEnabled(clear_all_frames_enabled)
        self._clear_button.setEnabled(clear_all_frames_enabled)

    def set_color(self, color):
        """Update the swatch color display."""
        self.swatch.set_color(color)


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

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("annotationBeta")

        layout = QtWidgets.QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self._strip = AnnotateToolStrip()
        layout.addWidget(self._strip)

        self._panel = AnnotateSecondaryPanel()
        layout.addWidget(self._panel)

        # Floating color picker popup (no parent — truly floating)
        self._picker_popup = ColorPickerPopup()
        self._picker_popup.color_changed.connect(self._on_color_changed)

        self._strip.tool_changed.connect(self._on_tool_changed)
        self._strip.undo_requested.connect(self.undo_requested)
        self._strip.redo_requested.connect(self.redo_requested)
        self._strip.clear_requested.connect(self.clear_requested)
        self._strip.clear_all_requested.connect(self.clear_all_requested)
        self._strip.swatch.clicked.connect(self._on_swatch_toggle)

        # color_changed comes from the popup, not the panel
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

        self._panel.set_page_for_tool(constants.Tool.PEN)

    def _on_tool_changed(self, tool):
        self._panel.set_page_for_tool(tool)
        self.tool_changed.emit(tool)

    def _on_swatch_toggle(self):
        if self._picker_popup.isVisible():
            self._picker_popup.hide()
        else:
            self._picker_popup.show_near(self._strip.swatch)

    def hide_popups(self):
        self._picker_popup.hide()

    def _on_color_changed(self, color):
        self._strip.set_color(color)
        self.color_changed.emit(color)

    def set_color(self, color):
        """Programmatically set the active color (e.g. on tool switch)."""
        self._strip.set_color(color)
        self._picker_popup.set_color(color)

    def set_size(self, value):
        self._panel.set_size(value)

    def set_opacity(self, value):
        self._panel.set_opacity(value)

    def set_filled(self, value):
        self._panel.set_filled(value)

    @property
    def panel(self):
        return self._panel

    @property
    def strip(self):
        return self._strip

    def set_undo_enabled(self, enabled):
        self._strip.set_undo_enabled(enabled)

    def set_redo_enabled(self, enabled):
        self._strip.set_redo_enabled(enabled)

    def set_clear_enabled(self, clear_frame_enabled, clear_all_frames_enabled):
        self._strip.set_clear_enabled(clear_frame_enabled, clear_all_frames_enabled)

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
