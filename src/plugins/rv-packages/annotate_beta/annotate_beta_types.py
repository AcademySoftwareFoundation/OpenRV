# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

"""Components stored on RVPaint nodes."""

from dataclasses import dataclass


@dataclass(frozen=True)
class PaintFrame:
    paint_node: str
    frame: int


@dataclass(frozen=True)
class PaintComponent:
    paint_frame: PaintFrame
    name: str
