# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from PySide6 import QtCore, QtWidgets, QtGui

_PRESETS = [
    "#FF3B30",  # red
    "#FF9500",  # orange
    "#FFCC00",  # yellow
    "#34C759",  # green
    "#32ADE6",  # cyan
    "#FFFFFF",  # white
]

_CUSTOM_COLOR_ROWS = 2
_CUSTOM_COLOR_COLUMNS = 8


class _HSVGradientWidget(QtWidgets.QWidget):
    """Saturation/value square. Hue is controlled via set_hue()."""

    value_changed = QtCore.Signal(float, float)  # saturation, value in [0, 1]

    def __init__(self, parent=None):
        super().__init__(parent)
        self._hue = 0.0
        self._saturation = 1.0
        self._value = 1.0
        self.setFixedSize(160, 120)
        self.setCursor(QtCore.Qt.CrossCursor)

    def set_hue(self, hue):
        self._hue = max(0.0, min(1.0, hue))
        self.update()

    def set_saturation_value(self, saturation, value):
        self._saturation = saturation
        self._value = value
        self.update()

    def saturation(self):
        return self._saturation

    def value(self):
        return self._value

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, False)
        width, height = self.width(), self.height()

        full_hue = QtGui.QColor.fromHsvF(self._hue, 1.0, 1.0)

        saturation_gradient = QtGui.QLinearGradient(0, 0, width, 0)
        saturation_gradient.setColorAt(0.0, QtGui.QColor(255, 255, 255))
        saturation_gradient.setColorAt(1.0, full_hue)
        painter.fillRect(self.rect(), saturation_gradient)

        value_gradient = QtGui.QLinearGradient(0, 0, 0, height)
        value_gradient.setColorAt(0.0, QtGui.QColor(0, 0, 0, 0))
        value_gradient.setColorAt(1.0, QtGui.QColor(0, 0, 0, 255))
        painter.fillRect(self.rect(), value_gradient)

        marker_x = int(self._saturation * (width - 1))
        marker_y = int((1.0 - self._value) * (height - 1))
        painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
        painter.setPen(QtGui.QPen(QtGui.QColor(255, 255, 255, 220), 1.5))
        painter.setBrush(QtCore.Qt.NoBrush)
        painter.drawEllipse(marker_x - 5, marker_y - 5, 10, 10)

    def _update_from_position(self, position):
        max_x = max(1, self.width() - 1)
        max_y = max(1, self.height() - 1)
        self._saturation = max(0.0, min(1.0, position.x() / max_x))
        self._value = max(0.0, min(1.0, 1.0 - position.y() / max_y))
        self.update()
        self.value_changed.emit(self._saturation, self._value)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._update_from_position(event.position())

    def mouseMoveEvent(self, event):
        if event.buttons() & QtCore.Qt.LeftButton:
            self._update_from_position(event.position())


class _HueSlider(QtWidgets.QWidget):
    """Horizontal rainbow hue bar."""

    hue_changed = QtCore.Signal(float)  # [0, 1]

    def __init__(self, parent=None):
        super().__init__(parent)
        self._hue = 0.0
        self.setFixedHeight(16)
        self.setCursor(QtCore.Qt.PointingHandCursor)

    def set_hue(self, hue):
        self._hue = max(0.0, min(1.0, hue))
        self.update()

    def hue(self):
        return self._hue

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, True)

        # Inset bar so the handle circle doesn't clip at edges
        bar = self.rect().adjusted(6, 3, -6, -3)

        gradient = QtGui.QLinearGradient(bar.left(), 0, bar.right(), 0)
        for stop in range(7):
            gradient.setColorAt(stop / 6.0, QtGui.QColor.fromHsvF(stop / 6.0, 1.0, 1.0))
        painter.setPen(QtCore.Qt.NoPen)
        painter.setBrush(gradient)
        painter.drawRoundedRect(bar, 3, 3)

        handle_x = bar.left() + int(self._hue * bar.width())
        center_y = self.height() // 2
        painter.setPen(QtGui.QPen(QtGui.QColor(255, 255, 255, 200), 1.5))
        painter.setBrush(QtGui.QColor.fromHsvF(self._hue, 1.0, 1.0))
        painter.drawEllipse(handle_x - 5, center_y - 5, 10, 10)

    def _update_from_position(self, x):
        bar_left = 6
        bar_width = max(1, self.width() - 12)
        self._hue = max(0.0, min(1.0, (x - bar_left) / bar_width))
        self.update()
        self.hue_changed.emit(self._hue)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._update_from_position(event.position().x())

    def mouseMoveEvent(self, event):
        if event.buttons() & QtCore.Qt.LeftButton:
            self._update_from_position(event.position().x())


class _SwatchButton(QtWidgets.QAbstractButton):
    """Small rounded square color preset."""

    clicked_color = QtCore.Signal(QtGui.QColor)

    def __init__(self, color, parent=None):
        super().__init__(parent)
        self.setObjectName("presetSwatch")
        self._color = color
        self.setFixedSize(22, 22)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setToolTip(color.name().upper())
        self.clicked.connect(lambda: self.clicked_color.emit(self._color))

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
        painter.setPen(QtGui.QPen(self.palette().color(self.foregroundRole()), 1))
        painter.setBrush(self._color)
        painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 3, 3)


class _ColorPickerSection(QtWidgets.QWidget):
    """Color picker contents: saturation/value square, hue bar and preset swatches."""

    color_changed = QtCore.Signal(QtGui.QColor)
    dialog_requested = QtCore.Signal()

    def __init__(self, dialog_icon, parent=None):
        super().__init__(parent)
        self._color = QtGui.QColor(255, 255, 255)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(8)

        self._gradient = _HSVGradientWidget()
        self._gradient.value_changed.connect(self._on_saturation_value_changed)
        layout.addWidget(self._gradient, alignment=QtCore.Qt.AlignHCenter)

        self._hue_bar = _HueSlider()
        self._hue_bar.hue_changed.connect(self._on_hue_changed)
        layout.addWidget(self._hue_bar)

        swatch_row = QtWidgets.QWidget()
        swatch_row.setObjectName("swatchRow")
        swatch_layout = QtWidgets.QHBoxLayout(swatch_row)
        swatch_layout.setContentsMargins(0, 0, 0, 0)
        swatch_layout.setSpacing(4)
        for hex_color in _PRESETS:
            swatch = _SwatchButton(QtGui.QColor(hex_color))
            swatch.clicked_color.connect(self._on_preset_clicked)
            swatch_layout.addWidget(swatch)
        swatch_layout.addStretch()

        dialog_button = QtWidgets.QToolButton()
        dialog_button.setObjectName("colorDialogButton")
        dialog_button.setIcon(dialog_icon)
        dialog_button.setFixedSize(22, 22)
        dialog_button.setAutoRaise(True)
        dialog_button.setCursor(QtCore.Qt.PointingHandCursor)
        dialog_button.setToolTip("More Colors...")
        dialog_button.clicked.connect(self.dialog_requested)
        swatch_layout.addWidget(dialog_button)
        layout.addWidget(swatch_row)

    def set_color(self, color):
        """Sync all controls to color without emitting color_changed."""
        self._color = QtGui.QColor(color)
        hue, saturation, value, _ = self._color.getHsvF()
        if hue < 0:
            hue = 0.0
        self._gradient.set_hue(hue)
        self._gradient.set_saturation_value(saturation, value)
        self._hue_bar.set_hue(hue)

    def _on_hue_changed(self, hue):
        self._gradient.set_hue(hue)
        self._color = QtGui.QColor.fromHsvF(hue, self._gradient.saturation(), self._gradient.value())
        self.color_changed.emit(self._color)

    def _on_saturation_value_changed(self, saturation, value):
        self._color = QtGui.QColor.fromHsvF(self._hue_bar.hue(), saturation, value)
        self.color_changed.emit(self._color)

    def _on_preset_clicked(self, color):
        self.set_color(color)
        self.color_changed.emit(color)


class _NextCustomSlotOverlay(QtWidgets.QWidget):
    """Outlines the custom color slot that "Add to Custom Colors" will write into.

    QColorDialog keeps that slot private, so it is mirrored from the same two
    signals Qt updates it from (_q_nextCustom and _q_addCustom in qcolordialog.cpp).
    """

    def __init__(self, well, add_button):
        super().__init__(well)
        self.setAttribute(QtCore.Qt.WA_TransparentForMouseEvents)
        self._well = well
        self._index = 0
        grid = well.sizeHint()
        self._cell_width = grid.width() // _CUSTOM_COLOR_COLUMNS
        self._cell_height = grid.height() // _CUSTOM_COLOR_ROWS
        self.setGeometry(QtCore.QRect(QtCore.QPoint(0, 0), grid))
        QtCore.QObject.connect(well, QtCore.SIGNAL("currentChanged(int,int)"), self._on_current_changed)
        add_button.clicked.connect(self._on_added)

    def _on_current_changed(self, row, column):
        self._index = row + _CUSTOM_COLOR_ROWS * column
        self.update()

    def _on_added(self):
        self._index = (self._index + 1) % (_CUSTOM_COLOR_ROWS * _CUSTOM_COLOR_COLUMNS)
        self.update()

    def paintEvent(self, event):
        if not 0 <= self._index < _CUSTOM_COLOR_ROWS * _CUSTOM_COLOR_COLUMNS:
            return
        row = self._index % _CUSTOM_COLOR_ROWS
        column = self._index // _CUSTOM_COLOR_ROWS
        if self._well.isRightToLeft():
            column = _CUSTOM_COLOR_COLUMNS - column - 1
        cell = QtCore.QRect(column * self._cell_width, row * self._cell_height, self._cell_width, self._cell_height)
        painter = QtGui.QPainter(self)
        painter.setPen(QtGui.QPen(self.palette().color(QtGui.QPalette.Highlight), 2))
        painter.setBrush(QtCore.Qt.NoBrush)
        painter.drawRect(cell.adjusted(1, 1, -1, -1))


def add_next_custom_slot_highlight(dialog):
    """Outline the next custom color slot in a non-native QColorDialog.

    Does nothing if the dialog's layout isn't the expected one.
    """
    translate = QtCore.QCoreApplication.translate
    labels = [
        label
        for label in dialog.findChildren(QtWidgets.QLabel)
        if label.text() == translate("QColorDialog", "&Custom colors")
    ]
    buttons = [
        button
        for button in dialog.findChildren(QtWidgets.QPushButton)
        if button.text() == translate("QColorDialog", "&Add to Custom Colors")
    ]
    if len(labels) != 1 or len(buttons) != 1 or labels[0].buddy() is None:
        return
    _NextCustomSlotOverlay(labels[0].buddy(), buttons[0])


class ColorPickerPopup(QtWidgets.QFrame):
    """Floating color picker that appears to the right of the toolbar."""

    color_changed = QtCore.Signal(QtGui.QColor)
    dialog_requested = QtCore.Signal()

    def __init__(self, dialog_icon, parent=None):
        super().__init__(parent, QtCore.Qt.Tool | QtCore.Qt.FramelessWindowHint)
        self.setObjectName("annotationBetaColorPopup")
        self.setAttribute(QtCore.Qt.WA_ShowWithoutActivating)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        self._picker = _ColorPickerSection(dialog_icon)
        self._picker.color_changed.connect(self.color_changed)
        self._picker.dialog_requested.connect(self.dialog_requested)
        layout.addWidget(self._picker)
        self.adjustSize()

    def show_near(self, anchor_widget):
        """Position and show the popup to the right of anchor_widget."""
        position = anchor_widget.mapToGlobal(QtCore.QPoint(anchor_widget.width() + 6, 0))
        self.move(position)
        self.show()
        self.raise_()

    def hideEvent(self, event):
        # Clear hover state so no button shows as hovered when the popup reopens
        for widget in self.findChildren(QtWidgets.QWidget):
            if widget.testAttribute(QtCore.Qt.WA_UnderMouse):
                widget.setAttribute(QtCore.Qt.WA_UnderMouse, False)
                widget.update()
        super().hideEvent(event)

    def set_color(self, color):
        self._picker.set_color(color)
