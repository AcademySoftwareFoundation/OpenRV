# *****************************************************************************
# Copyright (c) 2021 Autodesk, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

import rv.commands as rvc
import rv.extra_commands as rvec
import rv.rvtypes

##############################################################################
# GLOBAL PARAMS
##############################################################################

MEDIA_NUM_FRAMES = 10

##############################################################################
# Annotation Tools
##############################################################################


class RVAnnotTools(rv.rvtypes.MinorMode):
    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)
        self.init(
            "Annotation Tests",
            [
                (
                    "at-first-source",
                    self.annotate_first_source,
                    "Annotate First Source",
                ),
            ],
            None,
            [
                (
                    "Annotation Tools",
                    [
                        (
                            "Annotate First Source",
                            self.annotate_first_source,
                            None,
                            None,
                        ),
                    ],
                )
            ],
        )

    def annotate_first_source(self, _):
        source_node = rvc.sourcesAtFrame(0)[0]
        source_data = rvc.sourceMediaInfoList(source_node)[0]

        paint_node = rvec.associatedNode("RVPaint", source_node)

        for frame in range(source_data["startFrame"], source_data["endFrame"] + 1):
            paint_prop = "{}.paint".format(paint_node)
            text_name = "text:{}:{}:annotation".format(rvc.getIntProperty("{}.nextId".format(paint_prop))[0], frame)
            text_prop = "{}.{}".format(paint_node, text_name)

            rvc.setIntProperty("{}.{}".format(paint_prop, "show"), [1])
            rvc.newProperty("{}.{}".format(text_prop, "text"), rvc.StringType, 1)
            rvc.newProperty("{}.{}".format(text_prop, "position"), rvc.FloatType, 2)
            rvc.newProperty("{}.{}".format(text_prop, "color"), rvc.FloatType, 4)
            rvc.newProperty("{}.{}".format(text_prop, "size"), rvc.FloatType, 1)
            rvc.newProperty("{}.frame:{}.order".format(paint_node, frame), rvc.StringType, 1)

            rvc.setStringProperty(
                "{}.{}".format(text_prop, "text"),
                ["This is frame {}".format(frame)],
                True,
            )
            rvc.setFloatProperty("{}.{}".format(text_prop, "position"), [-0.6, 0.24], True)
            rvc.setFloatProperty("{}.{}".format(text_prop, "color"), [1.0, 1.0, 1.0, 1.0], True)
            rvc.setFloatProperty("{}.{}".format(text_prop, "size"), [0.01], True)
            rvc.setStringProperty("{}.frame:{}.order".format(paint_node, frame), [text_name], True)


g_the_mode = None


def createMode():
    global g_the_mode
    g_the_mode = RVAnnotTools()
    return g_the_mode


def theMode():
    global g_the_mode
    return g_the_mode
