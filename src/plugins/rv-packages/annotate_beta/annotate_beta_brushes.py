# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass
from rv import commands


@dataclass(frozen=True)
class BrushEntry:
    name: str
    tip_path: str
    defaults: dict[str, float | bool]


def load_brush_catalogue() -> list[BrushEntry]:
    brush_catalogue = commands.brushCatalogue()
    return [BrushEntry(entry["name"], entry["tipPath"], entry["defaults"]) for entry in brush_catalogue]
