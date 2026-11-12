# Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

"""Undo/redo history of annotation actions per frame on each paint node."""

from collections import defaultdict
from collections.abc import Collection, Iterable
from dataclasses import dataclass
from enum import StrEnum

from annotate_beta_types import PaintComponent, PaintFrame


class PaintActionType(StrEnum):
    CREATE = "CREATE"
    CLEAR_ALL = "CLEAR_ALL"
    CLEAR_ALL_FRAMES = "CLEAR_ALL_FRAMES"


@dataclass(frozen=True)
class PaintAction:
    type: PaintActionType
    components: tuple[PaintComponent, ...]


class UndoHistory:
    def __init__(self):
        self._undo_stacks: defaultdict[PaintFrame, list[PaintAction]] = defaultdict(list)
        self._redo_stacks: defaultdict[PaintFrame, list[PaintAction]] = defaultdict(list)
        self._undo_clear_all_frames: PaintAction | None = None
        self._redo_clear_all_frames: PaintAction | None = None

    def push_create(self, component: PaintComponent):
        self._undo_stacks[component.paint_frame].append(PaintAction(PaintActionType.CREATE, (component,)))
        self._redo_stacks.pop(component.paint_frame, None)
        self._redo_clear_all_frames = None

    def push_clear_all(self, paint_frame: PaintFrame, components: Iterable[PaintComponent]):
        self._undo_stacks[paint_frame] = [PaintAction(PaintActionType.CLEAR_ALL, tuple(components))]
        self._redo_stacks.pop(paint_frame, None)
        self._undo_clear_all_frames = None
        self._redo_clear_all_frames = None

    def push_clear_all_frames(self, components: Iterable[PaintComponent]):
        self.clear()
        self._undo_clear_all_frames = PaintAction(PaintActionType.CLEAR_ALL_FRAMES, tuple(components))

    def discard_cleared_frame(self, paint_frame: PaintFrame):
        self._undo_stacks.pop(paint_frame, None)
        self._redo_stacks.pop(paint_frame, None)
        self._redo_clear_all_frames = None

    def can_undo(self, paint_frame: PaintFrame | None) -> bool:
        return self._undo_clear_all_frames is not None or bool(self._undo_stacks.get(paint_frame))

    def can_redo(self, paint_frame: PaintFrame | None) -> bool:
        return self._redo_clear_all_frames is not None or bool(self._redo_stacks.get(paint_frame))

    def undo(self, paint_frame: PaintFrame | None) -> PaintAction | None:
        undo_stack = self._undo_stacks.get(paint_frame)
        if undo_stack:
            action = undo_stack.pop()
            self._redo_stacks[paint_frame].append(action)
            return action
        action = self._undo_clear_all_frames
        if action is not None:
            self._undo_clear_all_frames = None
            self._redo_clear_all_frames = action
        return action

    def redo(self, paint_frame: PaintFrame | None) -> PaintAction | None:
        action = self._redo_clear_all_frames
        if action is not None:
            self._redo_clear_all_frames = None
            self._undo_clear_all_frames = action
            return action
        redo_stack = self._redo_stacks.get(paint_frame)
        if not redo_stack:
            return None
        action = redo_stack.pop()
        self._undo_stacks[paint_frame].append(action)
        return action

    def discard_latest_create(self, component: PaintComponent) -> bool:
        undo_stack = self._undo_stacks.get(component.paint_frame, [])
        if undo_stack[-1:] != [PaintAction(PaintActionType.CREATE, (component,))]:
            return False
        undo_stack.pop()
        return True

    def retain_paint_nodes(self, paint_nodes: Collection[str]):
        paint_frames = self._undo_stacks.keys() | self._redo_stacks.keys()
        stale_frames = [paint_frame for paint_frame in paint_frames if paint_frame.paint_node not in paint_nodes]

        for paint_frame in stale_frames:
            self._undo_stacks.pop(paint_frame, None)
            self._redo_stacks.pop(paint_frame, None)

        self._undo_clear_all_frames = self._retain_action(self._undo_clear_all_frames, paint_nodes)
        self._redo_clear_all_frames = self._retain_action(self._redo_clear_all_frames, paint_nodes)

    @staticmethod
    def _retain_action(action: PaintAction | None, paint_nodes: Collection[str]) -> PaintAction | None:
        if action is None:
            return None

        components = tuple(
            component for component in action.components if component.paint_frame.paint_node in paint_nodes
        )
        if not components:
            return None

        return PaintAction(action.type, components)

    def clear(self):
        self._undo_stacks.clear()
        self._redo_stacks.clear()
        self._undo_clear_all_frames = None
        self._redo_clear_all_frames = None
