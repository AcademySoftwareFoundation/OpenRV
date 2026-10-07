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

    def __init__(self, parent=None):
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


class ColorPickerPopup(QtWidgets.QFrame):
    """Floating color picker that appears to the right of the toolbar."""

    color_changed = QtCore.Signal(QtGui.QColor)

    def __init__(self, parent=None):
        super().__init__(parent, QtCore.Qt.Tool | QtCore.Qt.FramelessWindowHint)
        self.setObjectName("annotationBetaColorPopup")
        self.setAttribute(QtCore.Qt.WA_ShowWithoutActivating)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        self._picker = _ColorPickerSection()
        self._picker.color_changed.connect(self.color_changed)
        layout.addWidget(self._picker)
        self.adjustSize()

    def show_near(self, anchor_widget):
        """Position and show the popup to the right of anchor_widget."""
        position = anchor_widget.mapToGlobal(QtCore.QPoint(anchor_widget.width() + 6, 0))
        self.move(position)
        self.show()
        self.raise_()

    def set_color(self, color):
        self._picker.set_color(color)
