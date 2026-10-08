# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from enum import StrEnum

MODE_NAME = "annotate_beta_mode"
EVENT_TABLE_NAME = "annotate_beta_shape"
SETTINGS_GROUP = "AnnotateBeta"


class Tool(StrEnum):
    CURSOR = "cursor"
    PEN = "pen"
    AIRBRUSH = "airbrush"
    ERASER = "eraser"
    RECT = "rect"
    CIRCLE = "circle"
    ARROW = "arrow"
    LINE = "line"
    TEXT = "text"
    EYEDROPPER = "eyedropper"


SHAPE_TOOLS = frozenset({Tool.RECT, Tool.CIRCLE, Tool.ARROW, Tool.LINE})
BRUSH_TOOLS = frozenset({Tool.PEN, Tool.AIRBRUSH, Tool.ERASER})
DRAWING_TOOLS = SHAPE_TOOLS | BRUSH_TOOLS | {Tool.TEXT}


class ColorModifier(StrEnum):
    NORMAL = "normal"
    ADDITIVE = "additive"
    DARKEN = "darken"


class Brush(StrEnum):
    """Paint brush names understood by PaintIPNode's ribbon (non-stamp) path."""

    CIRCLE = "circle"
    GAUSS = "gauss"


class FontSize(StrEnum):
    SMALL = "small"
    MEDIUM = "medium"
    LARGE = "large"


DEFAULT_COLOR = "#ffdc00"
DEFAULT_FONT_FAMILY = "Helvetica"

SIZE_MIN = 1
SIZE_MAX = 100
DEFAULT_SIZE = 32

OPACITY_MIN = 0
OPACITY_MAX = 100
DEFAULT_OPACITY = 50

# Minimum spacing between recorded stroke points, as a fraction of stroke width.
# Matches the splat smoothing interval in PaintIPNode::compilePenComponent, so
# closer points only pile up as overlapping airbrush splats.
PEN_MIN_POINT_SPACING = 0.25
