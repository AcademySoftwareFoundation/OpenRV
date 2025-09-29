#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import os

try:
    from PySide2 import QtGui, QtCore, QtWidgets
    from PySide2.QtGui import *
    from PySide2.QtCore import *
    from PySide2.QtWidgets import *
    from PySide2.QtUiTools import QUiLoader
except ImportError:
  try:
    from PySide6 import QtGui, QtCore, QtWidgets
    from PySide6.QtGui import *
    from PySide6.QtCore import *
    from PySide6.QtWidgets import *
    from PySide6.QtUiTools import QUiLoader
  except ImportError:
    pass

from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import rv
import rv.qtutils

import pyside_example  # need to get at the module itself


class PySideDockTest(rv.rvtypes.MinorMode):
    "A python mode example that uses PySide"

    def orientCamera(self):
        "update projection matrix, especially when aspect ratio changes"

        glEnable(GL_CULL_FACE)
        glCullFace(GL_BACK)

        glPushAttrib(GL_TRANSFORM_BIT)  # remember current GL_MATRIX_MODE

        # Set the camera lens so that we have a perspective viewing volume whose
        # horizontal bounds at the near clipping plane are -2..2 and vertical
        # bounds are -1.5..1.5.  The near clipping plane is 1 unit from the camera
        # and the far clipping plane is 40 units away.
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glFrustum(-2, 2, -1.5, 1.5, 1, 40)

        # Set up transforms so that the tetrahedron which is defined right at
        # the origin will be rotated and moved into the view volume.  First we
        # rotate 70 degrees around y so we can see a lot of the left side.
        # Then we rotate 50 degrees around x to "drop" the top of the pyramid
        # down a bit.  Then we move the object back 3 units "into the screen".
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        glTranslatef(0, 0, -3)
        glRotatef(50, 1, 0, 0)
        glRotatef(rv.commands.frame() % 360, 0, 1, 0)

        glPopAttrib()  # restore GL_MATRIX_MODE

    def renderGL(self):
        # ... render stuff in here ...
        # Draw a white grid "floor" for the tetrahedron to sit on.
        glColor3f(1.0, 1.0, 1.0)
        glBegin(GL_LINES)
        i = -2.5
        while i <= 2.5:
            glVertex3f(i, 0, 2.5)
            glVertex3f(i, 0, -2.5)
            glVertex3f(2.5, 0, i)
            glVertex3f(-2.5, 0, i)
            i += 0.25
        glEnd()

        # Draw the tetrahedron.  It is a four sided figure, so when defining it
        # with a triangle strip we have to repeat the last two vertices.
        glBegin(GL_TRIANGLE_STRIP)
        glColor3f(1, 1, 1)
        glVertex3f(0, 2, 0)
        glColor3f(1, 0, 0)
        glVertex3f(-1, 0, 1)
        glColor3f(0, 1, 0)
        glVertex3f(1, 0, 1)
        glColor3f(0, 0, 1)
        glVertex3f(0, 0, -1.4)
        glColor3f(1, 1, 1)
        glVertex3f(0, 2, 0)
        glColor3f(1, 0, 0)
        glVertex3f(-1, 0, 1)
        glEnd()

    def checkBoxPressed(self, checkbox, prop):
        def F():
            try:
                if checkbox.isChecked():
                    if self.rvSessionQObject is not None:
                        self.rvSessionQObject.setWindowOpacity(1.0)
                    rv.commands.setIntProperty(prop, [1], True)
                else:
                    if self.rvSessionQObject is not None:
                        self.rvSessionQObject.setWindowOpacity(0.5)
                    rv.commands.setIntProperty(prop, [0], True)
            except Exception:
                pass

        return F

    def dialChanged(self, index, last, spins, prop):
        def F(value):
            diff = float(value - last[index])
            if diff < -180:
                diff = value - last[index] + 360
            elif diff > 180:
                diff = value - last[index] - 360
            diff /= 360.0
            last[index] = float(value)
            try:
                p = rv.commands.getFloatProperty(prop, 0, 1231231)
                p[0] += diff
                if p[0] > spins[index].maximum():
                    p[0] = spins[index].maximum()
                if p[0] < spins[index].minimum():
                    p[0] = spins[index].minimum()
                spins[index].setValue(p[0])
                rv.commands.setFloatProperty(prop, p, True)
            except Exception:
                pass

        return F

    def spinChanged(self, index, spins, prop):
        def F(value):
            try:
                rv.commands.setFloatProperty(prop, [p], True)
            except Exception:
                pass

        def F():
            try:
                p = spins[index].value()
                rv.commands.setFloatProperty(prop, [p], True)
            except Exception:
                pass

        return F

    def findSet(self, typeObj, names):
        array = []
        for n in names:
            array.append(self.dialog.findChild(typeObj, n))
            if array[-1] is None:
                print("Can't find", n)
        return array

    def hookup(self, checkbox, spins, dials, prop, last):
        checkbox.released.connect(
            self.checkBoxPressed(checkbox, "%s.node.active" % prop)
        )
        for i in range(0, 3):
            dial = dials[i]
            spin = spins[i]
            propName = "%s.warp.k%d" % (prop, i + 1)
            dial.valueChanged.connect(self.dialChanged(i, last, spins, propName))
            spin.valueChanged.connect(self.spinChanged(i, spins, propName))
            last[i] = dial.value()

    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)
        self.init("pyside_example", None, None)

        self.loader = QUiLoader()
        uifile = QFile(
            os.path.join(
                self.supportPath(pyside_example, "pyside_example"), "control.ui"
            )
        )
        uifile.open(QFile.ReadOnly)
        self.dialog = self.loader.load(uifile)
        uifile.close()

        self.enableCheckBox = self.dialog.findChild(QCheckBox, "enableCheckBox")

        #
        # To retrieve the current RV session window and
        # use it as a Qt QMainWindow, we do the following:
        self.rvSessionQObject = rv.qtutils.sessionWindow()

        # have to hold refs here so they don't get deleted
        self.radialDistortDials = self.findSet(QDial, ["k1Dial", "k2Dial", "k3Dial"])

        self.radialDistortSpins = self.findSet(
            QDoubleSpinBox, ["k1SpinBox", "k2SpinBox", "k3SpinBox"]
        )

        self.lastRadialDistort = [0, 0, 0]

        self.hookup(
            self.enableCheckBox,
            self.radialDistortSpins,
            self.radialDistortDials,
            "#RVLensWarp",
            self.lastRadialDistort,
        )

    def render(self, event):
        self.orientCamera()
        self.renderGL()

    def activate(self):
        rv.rvtypes.MinorMode.activate(self)
        self.dialog.show()

    def deactivate(self):
        rv.rvtypes.MinorMode.deactivate(self)
        self.dialog.hide()


def createMode():
    "Required to initialize the module. RV will call this function to create your mode."
    return PySideDockTest()
