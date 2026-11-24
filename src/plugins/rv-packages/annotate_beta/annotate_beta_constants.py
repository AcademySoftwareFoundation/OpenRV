# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum, StrEnum, auto

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


class StampBlendMode(IntEnum):
    NORMAL = 0
    ERASER = 1
    ADDITIVE = 2


PEN_BRUSH = "pen"

BRUSH_DISPLAY_NAMES = {
    "pen": "Pen",
    "pencil": "Pencil",
    "coarse_watercolor": "Watercolor",
    "camo": "Camo",
    "charcoal": "Charcoal",
    "hr": "HR",
}


class BrushSetting(StrEnum):
    """Brush parameters the settings sliders edit, named as in the catalogue."""

    HARDNESS = "hardness"
    SPACING_BIAS = "spacingBias"
    RADIUS_JITTER = "radiusJitter"
    OPACITY_JITTER = "opacityJitter"
    ROTATION_JITTER = "rotationJitter"
    SPACING_JITTER = "spacingJitter"


class EraserBrush(StrEnum):
    HARD = auto()
    SOFT = auto()


ERASER_BRUSH_VALUES = {
    EraserBrush.HARD: {},
    EraserBrush.SOFT: {BrushSetting.HARDNESS: 0.5},
}

BRUSH_SETTING_MIN = 0
BRUSH_SETTING_MAX = 100
BRUSH_SETTING_RANGES = {
    BrushSetting.HARDNESS: (0, 1),
    BrushSetting.SPACING_BIAS: (0.1, 4),
    BrushSetting.RADIUS_JITTER: (0, 1),
    BrushSetting.OPACITY_JITTER: (0, 1),
    BrushSetting.ROTATION_JITTER: (0, 180),
    BrushSetting.SPACING_JITTER: (0, 4),
}


class FontSize(IntEnum):
    """Preset font sizes, in pixels."""

    SMALL = 24
    MEDIUM = 48
    LARGE = 72


FONT_SIZE_MIN = 8
FONT_SIZE_MAX = 72
DEFAULT_FONT_SIZE = FontSize.MEDIUM


DEFAULT_COLOR = "#ffdc00"
DEFAULT_FONT_FAMILY = "Helvetica"

FONT_BATCH_TIME_BUDGET_SECONDS = 0.001

SIZE_MIN = 1
SIZE_MAX = 100
DEFAULT_SIZE = 32

OPACITY_MIN = 0
OPACITY_MAX = 100
DEFAULT_OPACITY = 50
