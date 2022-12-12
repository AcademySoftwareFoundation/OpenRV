#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import rv.commands
import os

from PySide2 import QtGui, QtWidgets
from PySide2.QtGui import *
from PySide2.QtWidgets import *
from shiboken2 import wrapInstance
from shiboken2 import getCppPointer


def sessionWindow():
    """
    Returns the QMainWindow for the current RV session window.
    """

    rvPyLongPtr = rv.commands.sessionWindow()
    if rvPyLongPtr is not None:
        return wrapInstance(rvPyLongPtr, QMainWindow)
    else:
        return None


def sessionGLView():
    """
    Returns the QOpenGLWidget for the current RV session GL view.
    """

    rvPyLongPtr = rv.commands.sessionGLView()
    if rvPyLongPtr is not None:
        return wrapInstance(rvPyLongPtr, QOpenGLWidget)
    else:
        return None


def sessionTopToolBar():
    """
    Returns the QToolBar for the current RV session top tool bar.
    """

    rvPyLongPtr = rv.commands.sessionTopToolBar()
    if rvPyLongPtr is not None:
        return wrapInstance(rvPyLongPtr, QToolBar)
    else:
        return None


def sessionBottomToolBar():
    """
    Returns the QToolBar for the current RV session bottom tool bar.
    """

    rvPyLongPtr = rv.commands.sessionBottomToolBar()
    if rvPyLongPtr is not None:
        return wrapInstance(rvPyLongPtr, QToolBar)
    else:
        return None


def javascriptExport(webFrame):
    """
    Add the "rvsession" object to the JavaScript instance of the given
    QWebFrame, connecting it to RV's event system and scripting layers.
    """

    rvPyLongPtrs = getCppPointer(webFrame)
    rv.commands.javascriptExport(rvPyLongPtrs[0])
