# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass

from rv import commands

import annotate_beta_constants as constants


@dataclass(frozen=True)
class BrushEntry:
    name: str
    display_name: str
    tip_path: str
    defaults: dict[str, float | bool]


def value_to_slider_percent(setting: constants.BrushSetting, catalogue_value: float) -> int:
    setting_min, setting_max = constants.BRUSH_SETTING_RANGES[setting]
    position_on_slider = (catalogue_value - setting_min) / (setting_max - setting_min)
    return round(position_on_slider * constants.BRUSH_SETTING_MAX)


def slider_percent_to_value(setting: constants.BrushSetting, percent: int) -> float:
    setting_min, setting_max = constants.BRUSH_SETTING_RANGES[setting]
    return setting_min + percent / constants.BRUSH_SETTING_MAX * (setting_max - setting_min)


def load_brush_catalogue() -> list[BrushEntry]:
    catalogue = {entry["name"]: entry for entry in commands.brushCatalogue()}
    entries = []
    for name, display_name in constants.BRUSH_DISPLAY_NAMES.items():
        if name in catalogue:
            entry = catalogue[name]
            entries.append(BrushEntry(name, display_name, entry["tipPath"], entry["defaults"]))
    return entries
