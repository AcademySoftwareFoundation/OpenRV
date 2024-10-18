#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#

try:
    from PySide2 import QtCore
    from PySide2 import QtWidgets
    from PySide2 import QtWebChannel
    from PySide2 import QtWebEngineWidgets
    from PySide2 import QtQml

    from PySide2.QtWidgets import (
        qApp,
    )  # import qApp#, QMainWindow, QLabel, QPixmap, QColor, QGridLayout, QWidget, QPushButton, QComboBox
    from PySide2.QtWidgets import QDockWidget
except ImportError:
  try:
    from PySide6 import QtCore
    from PySide6 import QtWidgets
    from PySide6 import QtWebChannel
    from PySide6 import QtWebEngineWidgets
    from PySide6 import QtQml

    from PySide6.QtWidgets import (
        qApp,
    )  # import qApp#, QMainWindow, QLabel, QPixmap, QColor, QGridLayout, QWidget, QPushButton, QComboBox
    from PySide6.QtWidgets import QDockWidget
  except ImportError:
    pass

print(QtWebEngineWidgets.__file__)

import sys, os

import rv.commands
from shiboken2 import getCppPointer

from rv import rvtypes
from rv import qtutils


class WebView2(rvtypes.MinorMode):
    def __init__(self):
        rvtypes.MinorMode.__init__(self)
        self.init("webview2", None, None)

        self.window = qtutils.sessionWindow()

        self.dock_widget = QDockWidget("Chromium Doc Widget", self.window)
        self.window.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.dock_widget)

        self.webview = QtWebEngineWidgets.QWebEngineView()
        self.webview.setMaximumHeight(140)
        self.dock_widget.setWidget(self.webview)

        qtutils.javascriptExport(self.webview.page())

        defaultpath = "file:///" + os.path.join(
            self.supportPath(sys.modules[__name__]), "webview_example.html"
        )
        self.webview.load(defaultpath)

    def activate(self):
        rvtypes.MinorMode.activate(self)
        self.dock_widget.show()

    def deactivate(self):
        rvtypes.MinorMode.deactivate(self)
        self.dock_widget.hide()


def createMode():
    return WebView2()
