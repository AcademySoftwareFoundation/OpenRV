#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv.rvtypes import MinorMode
import rv.commands as commands
import rv.extra_commands as extra_commands


class PyMyStuffMode(MinorMode):
    def faster(self, event):
        commands.setFPS(commands.fps() * 1.5)
        extra_commands.displayFeedback("%g fps" % commands.fps(), 2.0)

    def slower(self, event):
        commands.setFPS(commands.fps() * 1.0 / 1.5)
        extra_commands.displayFeedback("%g fps" % commands.fps(), 2.0)

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
