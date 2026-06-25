# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
OTIO schemadef for a text annotation shape.

Font properties are discrete fields so each platform constructs its own
representation (CSS for web clients, QFont for RV) without string parsing.
anchor is the baseline start point, consistent with min/max/start_position
on other shape types.

fontSize is in normalised units (relative to image height).
For 16:9 content: small ≈ 0.15, medium ≈ 0.22, large ≈ 0.33.

All coordinates are in normalised image space (see position_schema.py).

Example:
    text = otio.schemadef.Text.Text(
        anchor=otio.schemadef.Position.Position(x=-3.0, y=2.0),
        text="Frame needs colour correction",
        fontFamily="Inter",
        fontSize=0.22,
        fontWeight="normal",
        fontStyle="normal",
        textDecoration="none",
        textAlign="left",
        innerColor=[1.0, 1.0, 1.0, 1.0],
        id="e5f6a7b8-c9d0-1234-efab-345678901234",
        visible=True,
        layer_range=otio.opentime.TimeRange(...),
    )
"""

import opentimelineio as otio


@otio.core.register_type
class Text(otio.core.SerializableObject):
    """A text annotation."""

    _serializable_label = "Text.1"
    _name = "Text"

    def __init__(
        self,
        anchor=None,
        text: str | None = None,
        fontFamily: str | None = None,
        fontSize: float | None = None,
        fontWeight: str | None = None,
        fontStyle: str | None = None,
        textDecoration: str | None = None,
        textAlign: str | None = None,
        innerColor: list | None = None,
        id: str | None = None,
        visible: bool | None = None,
        layer_range: otio.opentime.TimeRange | None = None,
    ) -> None:
        super().__init__()
        self.anchor = anchor
        self.text = text
        self.fontFamily = fontFamily
        self.fontSize = fontSize
        self.fontWeight = fontWeight
        self.fontStyle = fontStyle
        self.textDecoration = textDecoration
        self.textAlign = textAlign
        self.innerColor = innerColor
        self.id = id
        self.visible = visible
        self.layer_range = layer_range

    anchor = otio.core.serializable_field("anchor", doc="Baseline start point (Position.1)")
    text = otio.core.serializable_field("text", required_type=str, doc="Text content")
    fontFamily = otio.core.serializable_field("fontFamily", required_type=str, doc="Font family, e.g. 'Inter'")
    fontSize = otio.core.serializable_field(
        "fontSize", required_type=float, doc="Font size in normalised units (relative to image height)"
    )

    # "normal" | "bold"
    fontWeight = otio.core.serializable_field("fontWeight", required_type=str, doc="Font weight: 'normal' or 'bold'")

    # "normal" | "italic"
    fontStyle = otio.core.serializable_field("fontStyle", required_type=str, doc="Font style: 'normal' or 'italic'")

    # "none" | "underline"
    textDecoration = otio.core.serializable_field(
        "textDecoration", required_type=str, doc="Text decoration: 'none' or 'underline'"
    )

    # "left" | "center" | "right"
    textAlign = otio.core.serializable_field(
        "textAlign", required_type=str, doc="Text alignment relative to anchor: 'left', 'center', or 'right'"
    )

    _innerColor = otio.core.serializable_field("innerColor", required_type=list, doc="Text fill colour [r, g, b, a]")

    @property
    def innerColor(self) -> list:
        return self._innerColor

    @innerColor.setter
    def innerColor(self, val: list) -> None:
        self._innerColor = val

    id = otio.core.serializable_field("id", required_type=str, doc="UUID for undo/redo tracking")
    visible = otio.core.serializable_field("visible", required_type=bool, doc="Show/hide")

    _layer_range = otio.core.serializable_field(
        "layer_range",
        required_type=otio.opentime.TimeRange,
        doc="Time range this annotation is visible on",
    )

    @property
    def layer_range(self) -> otio.opentime.TimeRange:
        return self._layer_range

    @layer_range.setter
    def layer_range(self, val) -> None:
        self._layer_range = val

    def __str__(self) -> str:
        return f"Text({self.id}, {self.text!r})"

    def __repr__(self) -> str:
        return (
            f"otio.schemadef.Text.Text("
            f"id={self.id!r}, text={self.text!r}, fontFamily={self.fontFamily!r}, "
            f"fontSize={self.fontSize!r}, anchor={self.anchor!r})"
        )
