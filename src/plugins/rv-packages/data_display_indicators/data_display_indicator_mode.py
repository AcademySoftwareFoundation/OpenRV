#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import re, math

import rv.rvtypes as rvt
import rv.commands as rvc
import rv.extra_commands as rvec

from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *


class WireBox(object):
    def render(self):
        glColor(self.r, self.g, self.b, 1.0)
        glBegin(GL_LINE_LOOP)
        glVertex(self.w)
        glVertex(self.x)
        glVertex(self.y)
        glVertex(self.z)
        glEnd()

    def __init__(self, r, g, b, w, x, y, z):
        self.r = r
        self.g = g
        self.b = b
        self.w = w
        self.x = x
        self.y = y
        self.z = z


class EXRWindowIndicatorMode(rvt.MinorMode):
    def setupProjection(self, event):
        """
        Set the orthographic view to draw the bound boxes in
        """

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluOrtho2D(0.0, event.domain()[0] - 1, 0.0, event.domain()[1] - 1)

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

    def getWindows(self, source):
        """
        Get EXR Data/Display window values from EXR attributes (transmitted from RV
        EXR reader as frame buffer attributes).
        """

        attrs = rvc.sourceAttributes(source)

        dataH = 0.0
        pa = 1.0
        for a in attrs:

            if a[0] == "EXR/dataWindow":
                clean = re.sub("[\(\)]", " ", a[1])
                parts = clean.split(" ")

                dataX = float(parts[1])
                dataY = float(parts[2])
                dataW = float(parts[6]) + 1.0 - dataX
                dataH = float(parts[7]) + 1.0 - dataY

            if a[0] == "EXR/displayWindow":
                clean = re.sub("[\(\)]", " ", a[1])
                parts = clean.split(" ")

                dispX = float(parts[1])
                dispY = float(parts[2])
                dispW = float(parts[6]) + 1.0 - dispX
                dispH = float(parts[7]) + 1.0 - dispY

            if a[0] == "PixelAspectRatio":
                pa = float(a[1])

        if dataH == 0.0:
            return None

        return (dataX, dataY, dataW, dataH, dispX, dispY, dispW, dispH, pa)

    def getBoxes(self, s):
        """
        Collect the coordinates and colors of the bounding boxes
        """

        boxes = []

        #
        #   Get data/display windows if EXR
        #

        wins = self.getWindows(s)

        if not wins:
            return []

        (dataX, dataY, dataW, dataH, dispX, dispY, dispW, dispH, pa) = wins

        #
        #   Display window:
        #

        geom = rvc.imageGeometry(s, False)
        boxes.append(WireBox(0.5, 0.5, 1.0, geom[0], geom[1], geom[2], geom[3]))

        # Useful for debugging
        # geom2 = rvc.imageGeometry(s, True)
        # boxes.append(WireBox(0.5, 1.0, 0.5, geom2[0], geom2[1], geom2[2], geom2[3]))

        #
        #   Data window (skip if same as Display):
        #

        if dataW != dispW or dataH != dispH:

            #
            #   Find the angle/radians of rotation
            #

            flip = 0
            flop = 0
            angle = 0
            for xfrm in rvec.nodesInEvalPath(rvc.frame(), "RVTransform2D", s):
                flip = rvc.getIntProperty("%s.transform.flip" % xfrm)[0] - flip
                flop = rvc.getIntProperty("%s.transform.flop" % xfrm)[0] - flop
                angle += rvc.getFloatProperty("%s.transform.rotate" % xfrm)[0]
            theta = (angle / 180.0) * math.pi

            #
            #   "Unrotate" the image geometry to calculate scale
            #

            unRotMatrix = (
                (math.cos(-theta), math.sin(-theta)),
                (-math.sin(-theta), math.cos(-theta)),
            )

            geomCoords = (geom[0], geom[1], geom[2], geom[3])
            geomUnRot = self.dot2D(geomCoords, unRotMatrix)

            scaleY = (geomUnRot[1][1] - geomUnRot[2][1]) / dispH
            flipflop = -1.0 if (flip != flop) else 1.0
            scaleX = scaleY * pa * flipflop

            #
            #   Rotate the boundary differences between windows
            #

            rotMatrix = (
                (math.cos(theta), math.sin(theta)),
                (-math.sin(theta), math.cos(theta)),
            )

            diffCoords = (
                (
                    (dispX - dataX) * scaleX,
                    ((dataY + dataH) - (dispY + dispH)) * scaleY,
                ),
                (
                    ((dispX + dispW) - (dataX + dataW)) * scaleX,
                    ((dataY + dataH) - (dispY + dispH)) * scaleY,
                ),
                (
                    ((dispX + dispW) - (dataX + dataW)) * scaleX,
                    (dataY - dispY) * scaleY,
                ),
                ((dispX - dataX) * scaleX, (dataY - dispY) * scaleY),
            )

            diffRot = self.dot2D(diffCoords, rotMatrix)

            #
            #   Add the diffs to the image geometry to get the Data window
            #

            boxes.append(
                WireBox(
                    1.0,
                    0.5,
                    0.5,
                    (geom[0][0] + diffRot[0][0], geom[0][1] + diffRot[0][1]),
                    (geom[1][0] + diffRot[1][0], geom[1][1] + diffRot[1][1]),
                    (geom[2][0] + diffRot[2][0], geom[2][1] + diffRot[2][1]),
                    (geom[3][0] + diffRot[3][0], geom[3][1] + diffRot[3][1]),
                )
            )

        return boxes

    def dot2D(self, array1, array2):
        array3 = [[0, 0], [0, 0], [0, 0], [0, 0]]
        for i in range(4):
            for j in range(2):
                array3[i][j] = sum(p * q for p, q in zip(array1[i], array2[j]))

        return array3

    def render(self, event):

        sources = rvc.sourcesAtFrame(rvc.frame())

        if len(sources) == 0:
            return

        boxes = []
        for s in sources:
            boxes += self.getBoxes(s)

        self.setupProjection(event)
        for box in boxes:
            box.render()

    def __init__(self):

        rvt.MinorMode.__init__(self)

        self.init(
            "data-display-indicator-mode",
            None,  # no bindings
            None,  # no bindings
            None,
        )  # menu item supplied by package system


def createMode():
    return EXRWindowIndicatorMode()
