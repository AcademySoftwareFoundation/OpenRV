# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass

from rv import commands

import annotate_beta_constants as constants


@dataclass(frozen=True)
class BrushEntry:
    name: str
    tip_path: str
    defaults: dict[str, float | bool]

    def default_slider_percent(self, setting: constants.BrushSetting) -> int:
        setting_min, setting_max = constants.BRUSH_SETTING_RANGES[setting]
        catalogue_value = self.defaults[setting]
        position_on_slider = (catalogue_value - setting_min) / (setting_max - setting_min)
        return round(position_on_slider * constants.BRUSH_SETTING_MAX)


def load_brush_catalogue() -> list[BrushEntry]:
    brush_catalogue = commands.brushCatalogue()
    return [BrushEntry(entry["name"], entry["tipPath"], entry["defaults"]) for entry in brush_catalogue]
