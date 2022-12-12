#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv.rvtypes import *
from rv.commands import *
from rv.extra_commands import *


class PyMyStuffMode(MinorMode):
    def faster(self, event):
        setFPS(fps() * 1.5)
        displayFeedback("%g fps" % fps(), 2.0)

    def slower(self, event):
        setFPS(fps() * 1.0 / 1.5)
        displayFeedback("%g fps" % fps(), 2.0)

    def __init__(self):
        MinorMode.__init__(self)
        self.init(
            "py-mystuff-mode",
            [
                ("key-down-->", self.faster, "speed up fps"),
                ("key-down--<", self.slower, "slow down fps"),
            ],
            None,
            [
                (
                    "MyStuff",
                    [
                        ("Increase FPS", self.faster, "=", None),
                        ("Decrease FPS", self.slower, "-", None),
                    ],
                )
            ],
        )


def createMode():
    return PyMyStuffMode()
