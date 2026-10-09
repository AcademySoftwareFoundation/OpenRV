# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

"""Undo/redo history of annotation actions per frame on each paint node."""

from collections import defaultdict
from dataclasses import dataclass
from enum import StrEnum


class PaintActionType(StrEnum):
    CREATE = "CREATE"
    CLEAR_ALL = "CLEAR_ALL"


@dataclass(frozen=True)
class PaintAction:
    type: PaintActionType
    node_names: list[str]  # Full property path prefixes, e.g. "RVPaint_1.rect:3:42:host_123"


@dataclass(frozen=True)
class PaintFrame:
    paint_node: str
    frame: int


class UndoHistory:
    def __init__(self):
        self._undo_stacks: defaultdict[PaintFrame, list[PaintAction]] = defaultdict(list)
        self._redo_stacks: defaultdict[PaintFrame, list[PaintAction]] = defaultdict(list)

    def push(self, paint_frame: PaintFrame, action: PaintAction):
        self._undo_stacks[paint_frame].append(action)
        self._redo_stacks.pop(paint_frame, None)

    def can_undo(self, paint_frame: PaintFrame | None) -> bool:
        return bool(self._undo_stacks.get(paint_frame))

    def can_redo(self, paint_frame: PaintFrame | None) -> bool:
        return bool(self._redo_stacks.get(paint_frame))

    def undo(self, paint_frame: PaintFrame | None) -> PaintAction | None:
        undo_stack = self._undo_stacks.get(paint_frame)
        if not undo_stack:
            return None
        action = undo_stack.pop()
        self._redo_stacks[paint_frame].append(action)
        return action

    def redo(self, paint_frame: PaintFrame | None) -> PaintAction | None:
        redo_stack = self._redo_stacks.get(paint_frame)
        if not redo_stack:
            return None
        action = redo_stack.pop()
        self._undo_stacks[paint_frame].append(action)
        return action

    def discard_if_latest(self, paint_frame: PaintFrame, action: PaintAction) -> bool:
        undo_stack = self._undo_stacks.get(paint_frame, [])
        if undo_stack[-1:] != [action]:
            return False
        undo_stack.pop()
        return True

    def clear(self):
        self._undo_stacks.clear()
        self._redo_stacks.clear()
