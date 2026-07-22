# Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

try:
    from PySide2 import QtCore, QtWidgets, QtGui
except ImportError:
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

    value_changed = QtCore.Signal(float, float)  # sat, val in [0, 1]

    def __init__(self, parent=None):
        super().__init__(parent)
        self._hue = 0.0
        self._sat = 1.0
        self._val = 1.0
        self.setFixedSize(160, 120)
        self.setCursor(QtCore.Qt.CrossCursor)

    def set_hue(self, hue_f):
        self._hue = max(0.0, min(1.0, hue_f))
        self.update()

    def set_sv(self, sat, val):
        self._sat = sat
        self._val = val
        self.update()

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing, False)
        w, h = self.width(), self.height()

        full_hue = QtGui.QColor.fromHsvF(self._hue, 1.0, 1.0)

        hg = QtGui.QLinearGradient(0, 0, w, 0)
        hg.setColorAt(0.0, QtGui.QColor(255, 255, 255))
        hg.setColorAt(1.0, full_hue)
        p.fillRect(self.rect(), hg)

        vg = QtGui.QLinearGradient(0, 0, 0, h)
        vg.setColorAt(0.0, QtGui.QColor(0, 0, 0, 0))
        vg.setColorAt(1.0, QtGui.QColor(0, 0, 0, 255))
        p.fillRect(self.rect(), vg)

        cx = int(self._sat * (w - 1))
        cy = int((1.0 - self._val) * (h - 1))
        p.setRenderHint(QtGui.QPainter.Antialiasing, True)
        p.setPen(QtGui.QPen(QtGui.QColor(255, 255, 255, 220), 1.5))
        p.setBrush(QtCore.Qt.NoBrush)
        p.drawEllipse(cx - 5, cy - 5, 10, 10)

    def _update_from_pos(self, pos):
        w = max(1, self.width() - 1)
        h = max(1, self.height() - 1)
        self._sat = max(0.0, min(1.0, pos.x() / w))
        self._val = max(0.0, min(1.0, 1.0 - pos.y() / h))
        self.update()
        self.value_changed.emit(self._sat, self._val)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._update_from_pos(event.pos())

    def mouseMoveEvent(self, event):
        if event.buttons() & QtCore.Qt.LeftButton:
            self._update_from_pos(event.pos())


class _HueSlider(QtWidgets.QWidget):
    """Horizontal rainbow hue bar."""

    hue_changed = QtCore.Signal(float)  # [0, 1]

    def __init__(self, parent=None):
        super().__init__(parent)
        self._hue = 0.0
        self.setFixedHeight(16)
        self.setCursor(QtCore.Qt.PointingHandCursor)

    def set_hue(self, hue_f):
        self._hue = max(0.0, min(1.0, hue_f))
        self.update()

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing, True)

        # Inset bar so the handle circle doesn't clip at edges
        bar = self.rect().adjusted(6, 3, -6, -3)

        grad = QtGui.QLinearGradient(bar.left(), 0, bar.right(), 0)
        for i in range(7):
            grad.setColorAt(i / 6.0, QtGui.QColor.fromHsvF(i / 6.0, 1.0, 1.0))
        p.setPen(QtCore.Qt.NoPen)
        p.setBrush(grad)
        p.drawRoundedRect(bar, 3, 3)

        x = bar.left() + int(self._hue * bar.width())
        cy = self.height() // 2
        p.setPen(QtGui.QPen(QtGui.QColor(255, 255, 255, 200), 1.5))
        p.setBrush(QtGui.QColor.fromHsvF(self._hue, 1.0, 1.0))
        p.drawEllipse(x - 5, cy - 5, 10, 10)

    def _update_from_pos(self, x):
        bar_left = 6
        bar_width = max(1, self.width() - 12)
        self._hue = max(0.0, min(1.0, (x - bar_left) / bar_width))
        self.update()
        self.hue_changed.emit(self._hue)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._update_from_pos(event.pos().x())

    def mouseMoveEvent(self, event):
        if event.buttons() & QtCore.Qt.LeftButton:
            self._update_from_pos(event.pos().x())


class _SwatchButton(QtWidgets.QAbstractButton):
    """Small rounded square colour preset."""

    clicked_colour = QtCore.Signal(QtGui.QColor)

    def __init__(self, colour, parent=None):
        super().__init__(parent)
        self._colour = colour
        self.setFixedSize(22, 22)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setToolTip(colour.name().upper())

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing, True)
        p.setPen(QtGui.QPen(QtGui.QColor("#555555"), 1))
        p.setBrush(self._colour)
        p.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 3, 3)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.clicked_colour.emit(self._colour)


class ColourPickerSection(QtWidgets.QWidget):
    """Inline colour picker — shown/hidden inside the secondary panel."""

    colour_changed = QtCore.Signal(QtGui.QColor)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._colour = QtGui.QColor(255, 255, 255)

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(10, 10, 10, 10)
        lay.setSpacing(8)

        self._sv = _HSVGradientWidget()
        self._sv.value_changed.connect(self._on_sv_changed)
        lay.addWidget(self._sv, alignment=QtCore.Qt.AlignHCenter)

        self._hue_bar = _HueSlider()
        self._hue_bar.hue_changed.connect(self._on_hue_changed)
        lay.addWidget(self._hue_bar)

        swatch_row = QtWidgets.QWidget()
        swatch_row.setStyleSheet("QWidget { background: transparent; }")
        srow = QtWidgets.QHBoxLayout(swatch_row)
        srow.setContentsMargins(0, 0, 0, 0)
        srow.setSpacing(4)
        for hex_col in _PRESETS:
            btn = _SwatchButton(QtGui.QColor(hex_col))
            btn.clicked_colour.connect(self._on_preset_clicked)
            srow.addWidget(btn)
        srow.addStretch()
        lay.addWidget(swatch_row)

    def set_colour(self, colour):
        """Sync all controls to colour without emitting colour_changed."""
        self._colour = QtGui.QColor(colour)
        h, s, v, _ = self._colour.getHsvF()
        if h < 0:
            h = 0.0
        self._sv.set_hue(h)
        self._sv.set_sv(s, v)
        self._hue_bar.set_hue(h)

    # ------------------------------------------------------------------

    def _on_hue_changed(self, hue_f):
        self._sv.set_hue(hue_f)
        self._colour = QtGui.QColor.fromHsvF(hue_f, self._sv._sat, self._sv._val)
        self.colour_changed.emit(self._colour)

    def _on_sv_changed(self, sat, val):
        self._colour = QtGui.QColor.fromHsvF(self._hue_bar._hue, sat, val)
        self.colour_changed.emit(self._colour)

    def _on_preset_clicked(self, colour):
        self.set_colour(colour)
        self.colour_changed.emit(colour)
