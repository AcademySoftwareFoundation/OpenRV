# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from enum import StrEnum, auto

MODE_NAME = "annotate_beta_mode"
EVENT_TABLE_NAME = "annotate_beta_shape"
SETTINGS_GROUP = "AnnotateBeta"


class Tool(StrEnum):
    CURSOR = auto()
    PEN = auto()
    BRUSH = auto()
    ERASER = auto()
    RECT = auto()
    CIRCLE = auto()
    ARROW = auto()
    LINE = auto()
    TEXT = auto()
    EYEDROPPER = auto()


SHAPE_TOOLS = frozenset({Tool.RECT, Tool.CIRCLE, Tool.ARROW, Tool.LINE})
BRUSH_TOOLS = frozenset({Tool.PEN, Tool.BRUSH, Tool.ERASER})
DRAWING_TOOLS = SHAPE_TOOLS | BRUSH_TOOLS | {Tool.TEXT}


class ColorModifier(StrEnum):
    NORMAL = auto()
    ADDITIVE = auto()
    DARKEN = auto()


class Brush(StrEnum):
    """Paint brush names understood by PaintIPNode's ribbon (non-stamp) path."""

    CIRCLE = auto()
    GAUSS = auto()


class FontSize(StrEnum):
    SMALL = auto()
    MEDIUM = auto()
    LARGE = auto()


class BrushSetting(StrEnum):
    HARDNESS = auto()
    SPACING = auto()
    SIZE_VARIATION = auto()
    OPACITY_VARIATION = auto()
    ROTATION_VARIATION = auto()
    SPACING_VARIATION = auto()


DEFAULT_COLOR = "#ffdc00"
DEFAULT_FONT_FAMILY = "Helvetica"

FONT_BATCH_TIME_BUDGET_SECONDS = 0.001

SIZE_MIN = 1
SIZE_MAX = 100
DEFAULT_SIZE = 32

OPACITY_MIN = 0
OPACITY_MAX = 100
DEFAULT_OPACITY = 50

BRUSH_SETTING_MIN = 0
BRUSH_SETTING_MAX = 100
DEFAULT_BRUSH_SETTINGS = {
    BrushSetting.HARDNESS: 100,
    BrushSetting.SPACING: 25,
    BrushSetting.SIZE_VARIATION: 0,
    BrushSetting.OPACITY_VARIATION: 0,
    BrushSetting.ROTATION_VARIATION: 0,
    BrushSetting.SPACING_VARIATION: 0,
}
