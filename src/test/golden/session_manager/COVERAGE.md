# `session_manager` — Migration Coverage Contract

**Purpose.** Exhaustive list of `session_manager` behaviors that must keep working when
the package is ported from Mu to Python. Each item maps to verification gate(s) and scenario
id. The port is done when every item here passes per [`../VERIFICATION.md`](../VERIFICATION.md).

**Source of truth.** Mu implementation:
`src/plugins/rv-packages/session_manager/session_manager.mu.in` (~3532 lines),
`StackGroup_edit_mode.mu`, `SequenceGroup_edit_mode.mu`, `SwitchGroup_edit_mode.mu`,
`FolderGroup_edit_mode.mu`, `SourceGroup_edit_mode.mu`, `LayoutGroup_edit_mode.mu`,
`RetimeGroup_edit_mode.mu`, `Stack_edit_mode.mu`, `Switch_edit_mode.mu`,
`Composite_edit_mode.mu`, `transform_manip.mu`, `local_thumbnail_gen.py` (already Python).

**Iteration status (2026-08-04).** `run_migration_loop_mac.sh` exits 0: gates 0-5 pass
plus the post-gate 83-clip full-folder thumbnail check. Python is now RV's default
implementation for all twelve modes of this package (see
[`../VERIFICATION.md` § Mu/Python implementation toggle](../VERIFICATION.md#mupython-implementation-toggle)),
so gate 3 exercises the port rather than re-testing Mu, and gate 4 still reaches Mu via
`RV_MODE_IMPL_<mode>=mu`.

**Not yet done**, per [Definition of done](../VERIFICATION.md#definition-of-done):

| # | Item | State |
|---|---|---|
| 1, 6 | Every inventory item ✅ | 49 ✅ / 6 🟡 / 35 ⬜ — see [Behavior inventory](#behavior-inventory) |
| 3 | Gate 5 coverage bar (no ⬜ Mu-method rows) | 146 of 335 Mu symbols unit-tested (473 tests) |
| 4 | GUI sanity pixel review | **done this iteration** — 25/25 behavioral PASS, and all 44 PNG pairs byte-identical to `golden-mac/` (`rmsImageDiff -m` reports no max-diff line for any of them), so there is no rendering difference to attribute to noise or to a regression |
| 8 | Independent code-review agent | **run** — two fresh agents (port fidelity vs Mu; infrastructure + tests). All blocking findings fixed and verified; see below |

Gate 5's *suite* passes and is no longer vacuous (the tests import the port; hiding
`session_manager.py` makes the gate fail). What is missing is breadth, not honesty.

**Fixture path.** Real mp4 media for thumbnail/filmstrip and progressive-load tests:
`/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001/`.
Override via `SM_MERIDIAN_DIR` env var. Run scripts default to this path.

---

## Primary outcomes

Approved 2026-07-30.

| # | User-visible outcome | Graph / property signal | Pixel discriminant | Scenario(s) | B | P |
|---|----------------------|-------------------------|--------------------|-------------|---|---|
| 1 | Session tree categorises nodes: SOURCES / SEQUENCES / STACKS / FOLDERS / LAYOUTS / OTHER | `viewNodes()` each in correct category; `nodeType()` → category routing deterministic | Tree panel PNG: category headers visible with child rows | `sm_tree_categories` | ✅ | ✅ |
| 2 | Clicking a tree node sets that node as the active view | `viewNode() == clicked_node` | Checkmark `✓` in status col moves; view label text updates | `sm_select_node` | ✅ | ✅ |
| 3 | Delete removes the selected node from the session | `not nodeExists(node)` after delete | Row disappears from tree | `sm_delete_source` | ✅ | ✅ |
| 4 | Add Black / Color / Bars / Color Chart creates a movieproc source in SOURCES | New `RVSourceGroup`; `media.movie` contains movieproc URL of correct type; correct name | SOURCES shows new row | `sm_add_movieproc_black`, `sm_add_movieproc_solid`, `sm_add_movieproc_bars`, `sm_add_movieproc_colorchart` | ✅ | ✅ |
| 5 | Add Sequence / Stack wraps selected sources as inputs | New `RVSequenceGroup` / `RVStackGroup`; `nodeConnections()` shows sources as inputs | New SEQUENCES / STACKS row; inputs panel lists wrapped sources | `sm_add_sequence`, `sm_add_stack` | ✅ | ✅ |
| 6 | Inline rename updates the node's display name | `getStringProperty(node+".ui.name")[0] == new_name` | Tree row text shows new name | `sm_rename_inline` | ✅ | ✅ |
| 7 | Input reorder (up/down) and sort (A-Z / Z-A) change node connections | `nodeConnections(viewNode())._0` in expected order | Inputs panel rows in new order | `sm_inputs_reorder`, `sm_inputs_sort` | ✅ | ✅ |
| 8 | Real mp4 source thumbnail appears in source row after load | `session-manager-preview-available` fires; thumbnail on disk in cache | Source row widget shows thumbnail PNG (before=fallback, after=thumbnail differ) | `sm_meridian_mp4_load` | ✅ | ✅ |

---

## File inventory (approved 2026-07-30)

| Path | Role | Migration action |
|---|---|---|
| `session_manager.mu.in` | Main `SessionManagerMode` Mu source (~3532 lines) | Port to `session_manager.py`; keep until done |
| `StackGroup_edit_mode.mu` | Stack editor tab | Port to `StackGroup_edit_mode.py` |
| `SequenceGroup_edit_mode.mu` | Sequence editor tab | Port |
| `SwitchGroup_edit_mode.mu` | Switch editor tab | Port |
| `FolderGroup_edit_mode.mu` | Folder editor tab | Port |
| `SourceGroup_edit_mode.mu` | Source editor tab | Port |
| `LayoutGroup_edit_mode.mu` | Layout editor tab | Port |
| `RetimeGroup_edit_mode.mu` | Retime editor tab | Port |
| `Stack_edit_mode.mu` | Stack (non-group) editor | Port |
| `Switch_edit_mode.mu` | Switch editor | Port |
| `Composite_edit_mode.mu` | Composite editor | Port |
| `transform_manip.mu` | Transform manipulator | Port |
| `local_thumbnail_gen.py` | Already Python — thumbnail/filmstrip generator | Keep unchanged |
| `*.ui` | Qt Designer UI files (7 dialogs + main) | Keep unchanged |
| `*.png` | All icon images | Keep unchanged |
| `PACKAGE` | Mode registration | Add `.py` entries alongside `.mu.in` |
| `CMakeLists.txt` | Build target | Unchanged |

**External callers.** Two Mu packages used this one through `require session_manager`,
which cannot resolve a Python module. Both now reach it without the require
(Definition of done #7 — the cross-package API stays callable before the Mu source is
removed):

| Caller | Used | Now reaches it by |
|---|---|---|
| `rvnuke/rvnuke_mode.mu.in` | `theMode()`, `theMode().selectedNodes()`, `setToolTipProp()` | `sessionManagerLoaded()` / `sessionManagerSelectedNodes()` helpers; the tooltip is a single property write, inlined as `setSessionManagerToolTip()` |
| `maya_tools/maya_tools.mu.in` | `theMode()`, `theMode().selectedNodes()` | the same two helpers |

The helpers rest on two primitives, both verified against the **Python** mode in a
live RV (`--impl default`, so the Python port was the one loaded):

- `rvui.minorModeFromName("session_manager") neq nil` → `true`. A Python mode is
  registered as a `PyMinorMode` carrying the same `_modeName`, so the loaded check
  works regardless of implementation.
- `commands.sendInternalEvent("session-manager-selected-nodes", "")` → the selected
  node name. Both implementations answer this event (`selectedNodesEvent`), so gate 4
  keeps passing and a Mu-mode session behaves identically.

All three Mu modules were forced to compile (`runtime.eval("1", [module])`) to prove
the patches parse; the wrapper functions themselves are not called directly by a test,
because `runtime.eval` cannot invoke a function in another package's module scope —
`rvnuke_mode.deb()`, which predates this change, fails the same way.

- `state.sessionManager` is assigned in `SessionManagerMode.__init__` and may be read by other packages.
- Any package calling `modeManager.activateMode("session_manager")` must continue to work.

**Files to create:**

| Path | Role |
|---|---|
| `session_manager.py` | Python port + `createMode()` |
| `StackGroup_edit_mode.py` | Python port |
| `SequenceGroup_edit_mode.py` | Python port |
| `SwitchGroup_edit_mode.py` | Python port |
| `FolderGroup_edit_mode.py` | Python port |
| `SourceGroup_edit_mode.py` | Python port |
| `LayoutGroup_edit_mode.py` | Python port |
| `RetimeGroup_edit_mode.py` | Python port |
| `Stack_edit_mode.py` | Python port |
| `Switch_edit_mode.py` | Python port |
| `Composite_edit_mode.py` | Python port |
| `transform_manip.py` | Python port |
| `src/test/golden/session_manager/scenarios/*.py` | 24 golden scenarios |
| `src/test/golden/session_manager/unit/test_*.py` | Gate 5 unit tests |
| Run scripts (see Harness section below) | Migration loop + capture |

---

## Verification method

Gates (**B** / **P**), migration loop, capture, definition of done:
[`../VERIFICATION.md`](../VERIFICATION.md).

Coverage legend: **✅** = pinned by committed Mu golden; **🟡** = partial; **⬜** = awaiting capture.

**Harness note.** The `run_scenario.py` default `--mode` already covers all
`session_manager` sibling modes (`SESSION_MANAGER_ALL_MODES`). No per-scenario
`--mode` override needed unless testing a specific sub-mode in isolation.

**Pixel strategy.** Session manager is a QDockWidget — grab the dock's widget (`_baseWidget`)
for the tree/button bar PNG (`panel.png`). Grab the nav panel separately for `nav.png`.
Use fixed `QSize(400, 600)` on the base widget before grabbing to avoid monitor-DPI drift.

---

## Migration loop (this package)

```bash
cd src/test/golden/session_manager
./run_migration_loop_mac.sh
```

**Gate failure hints:**

| Output | Fix focus |
|---|---|
| `GATE 0 FAILED` | New runtime errors vs Mu baseline in `runtime_errors.txt` |
| `GATE 1 FAILED` | Graph/property mismatch — check `session.rv` diff from `compare.py` |
| `GATE 2 FAILED` | Pixel regression — open PNGs with `rmsImageDiff`; check widget grab size |
| `SANITY FAILED` | Real-display behavioral drift |
| `GATE 3 FAILED` | PACKAGE wiring or mode-registration issue |
| `GATE 4 FAILED` | Harness/golden corruption — re-capture Mu |
| `GATE 5 FAILED` | Missing/failing unit tests — check `unit/test_*.py` + `COVERAGE.md` Mu-methods table |

**Running gate 5 alone:**

```bash
GOLDEN_PKG_DIR=src/test/golden/session_manager src/test/golden/harness/run_unit_tests.sh
```

It picks RV's bundled interpreter automatically (the tests import the port, which
imports PySide6); `GOLDEN_PYTHON=<path>` overrides. If it reports "no interpreter with
PySide6 found", the staged app has not been built.

**Known limitations:**
- The mp4 scenarios are no longer skipped by `run_gui_sanity_gate.sh`: the helpers poll
  `loadTotal()` while pumping instead of calling `waitForProgressiveLoading()`, which is
  what used to deadlock a real display. Only `sm_folder_thumbnails_all` stays out, on
  cost grounds — `run_folder_thumbnails_all.sh` runs it after the gates.
- Media fixture path defaults to
  `/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001/`.
  Override: `SM_MERIDIAN_DIR=<path> ./run_migration_loop_mac.sh`.

---

## Behavior inventory

**Status (this iteration).** Gates 0-2 passed for all 25 gated scenarios against the
Python port, and gate 4 confirmed Mu still matches the same committed goldens, so
every row marked ✅ below is pinned in both implementations. Rows left ⬜ have neither a scenario nor a unit test. 🟡 means a unit test pins the
behaviour but no golden scenario exercises it: H1-H6 (drag and drop policy, in
`unit/test_tree_view.py` — no scenario drives a pointer drag), J1-J3 and K1-K3 and N1
(tab state, config and splitter, in `unit/test_mode.py`), and I3-I4 (preview path
events). ✅ is reserved for behaviours a committed golden pins, per the legend.


### A — Tree view & node categorization

| ID | Behavior | Gate | Status |
|---|---|---|---|
| A1 | SOURCES / SEQUENCES / STACKS / LAYOUTS / FOLDERS / OTHER headers appear when nodes of those types exist | B+P | ✅ |
| A2 | Category headers collapse/expand; state persisted in `#Session.sm_view.<CATEGORY>` | B+P | ✅ |
| A3 | Source nodes show sub-tree: media file (bold), views, layers, channels | B+P | ✅ |
| A4 | Sub-component expansion state persisted in `sm_state.expandedSubState` | B | ✅ |
| A5 | Node expansion state persisted in `sm_state.expandState` | B | ✅ |
| A6 | Sort order within folder persisted in `sm_state.sortKey` / `sm_state.sortKeyParent` | B | ✅ |
| A7 | Currently active view node shows `✓` in status column | B+P | ✅ |
| A8 | Folder nodes are drop-targets; non-folder category items are not | P | ⬜ |
| A9 | Node type → correct icon (RVSourceGroup=videofile, RVStackGroup=photoalbum, etc.) | P | ✅ |
| A10 | Tree column widths auto-resize to content | P | ⬜ |

### B — Node selection & view navigation

| ID | Behavior | Gate | Status |
|---|---|---|---|
| B1 | Single-click top-level item → `setViewNode()`; inputs panel updates | B+P | ✅ |
| B2 | Double-click top-level item → `viewByIndex()` | B | ⬜ |
| B3 | Click radio-button column on sub-component → `setImageRequest()` | B | ✅ |
| B4 | `request.imageComponent` change → radio icons update (blue-on vs dark) | B+P | ✅ |
| B5 | Prev-view / next-view buttons navigate `previousViewNode()` / `nextViewNode()` | B+P | ✅ |
| B6 | Home (select current) button scrolls tree to current view node and highlights it | B+P | ✅ |
| B7 | View label shows `uiName(viewNode())` | P | ✅ |
| B8 | `after-graph-view-change` event → `selectViewableNode()` + `updateNavUI()` + `restoreTabState()` | B | ✅ |
| B9 | Prev/next buttons disabled when no previous/next node exists | P | ✅ |

### C — Adding nodes (Add button menu)

| ID | Behavior | Gate | Status |
|---|---|---|---|
| C1 | Add > Sequence → new `RVSequenceGroup` with selected sources; `renameByType` names it | B+P | ✅ |
| C2 | Add > Stack → new `RVStackGroup` | B+P | ✅ |
| C3 | Add > Switch → new `RVSwitchGroup` | B | ✅ |
| C4 | Add > Layout → new `RVLayoutGroup` | B | ⬜ |
| C5 | Add > Retime → new `RVRetimeGroup` | B | ⬜ |
| C6 | Add > Color → new `RVColor` node | B | ⬜ |
| C7 | Add > OCIO → new `RVOCIO` node | B | ⬜ |
| C8 | Add > New Node by Type… → dialog shows all node types; creates chosen type | B | ⬜ |
| C9 | Add > Black… → `black,*.movieproc` source added; named "Black" | B+P | ✅ |
| C10 | Add > Color… → `solid,*.movieproc` with chosen RGB | B+P | ✅ |
| C11 | Add > Color Bars… → `smptebars,*.movieproc`; color controls hidden | B+P | ✅ |
| C12 | Add > SRGB Color Chart… → `srgbcolorchart,*.movieproc` | B | ✅ |
| C13 | Add > ACES Color Chart… → `acescolorchart,*.movieproc` | B | ✅ |
| C14 | Add > Blank… → `blank,*.movieproc`; width/height hidden | B | ⬜ |
| C15 | Create Image dialog FPS defaults from `General/fps` setting | B | ⬜ |
| C16 | Color picker in dialog updates button background and `_cidColor` | P | ✅ |

### D — Folder operations

| ID | Behavior | Gate | Status |
|---|---|---|---|
| D1 | Folder > Empty Folder → new `RVFolderGroup` with no inputs; named "Empty Folder" | B+P | ✅ |
| D2 | Folder > From Selection → folder wraps selected nodes; removes from current parent | B+P | ✅ |
| D3 | Folder > From Copy of Selection → folder wraps copies; original parent connections unchanged | B | ⬜ |
| D4 | Context menu → Folder submenu mirrors folder button menu | P | ⬜ |

### E — Delete operations

| ID | Behavior | Gate | Status |
|---|---|---|---|
| E1 | Delete button on selected source → `deleteNode()` | B+P | ✅ |
| E2 | Delete button on node inside folder → `removeInput()` when node has other parents | B | ⬜ |
| E3 | Delete folder → `deleteNode(folder)` | B+P | ✅ |
| E4 | Inputs panel delete button → removes selected inputs from `viewNode()` connections | B | ✅ |
| E5 | Delete with multiple selection deletes all selected | B | ⬜ |

### F — Rename / inline edit

| ID | Behavior | Gate | Status |
|---|---|---|---|
| F1 | F2 / Edit key → inline edit activated | B+P | ⬜ |
| F2 | Edit Info button → `_viewTreeView.edit(index)` | B+P | ⬜ |
| F3 | Rename on tree item → `setUIName(node, new_text)` | B+P | ✅ |
| F4 | `ui.name` change event → `_lazyUpdateTimer` fires → tree label refreshed | B | ✅ |

### G — Inputs panel

| ID | Behavior | Gate | Status |
|---|---|---|---|
| G1 | Inputs panel shows `nodeConnections(viewNode())._0` | B+P | ✅ |
| G2 | Source inputs show preview widget (thumbnail + name + meta) when previews enabled | P | ⬜ |
| G3 | Non-source inputs show icon + `uiName` | B+P | ✅ |
| G4 | Order Up moves selected input(s) one position toward top | B+P | ✅ |
| G5 | Order Down moves selected input(s) one position toward bottom | B+P | ✅ |
| G6 | Sort A-Z sorts all inputs alphabetically ascending; sets node connections | B+P | ✅ |
| G7 | Sort Z-A sorts descending | B+P | ✅ |
| G8 | Folder node sort also updates `sm_state.sortKey` on each child | B | ✅ |
| G9 | Inputs panel disabled for `RVSourceGroup` and `RVFileSource` nodes | P | ⬜ |
| G10 | Double-click input → `viewByIndex()` sets that node as view | B | ⬜ |

### H — Drag and drop

| ID | Behavior | Gate | Status |
|---|---|---|---|
| H1 | Drag source from tree → drop onto folder → `CopyAction` adds as input | B | 🟡 |
| H2 | Drag source in tree → reorder → `MoveAction` updates parent connections | B | 🟡 |
| H3 | Drag from tree to inputs view → forces `CopyAction` | B | 🟡 |
| H4 | Dragging non-folder nodes disables the FOLDERS section as drop target | P | 🟡 |
| H5 | `NodeModel.mimeData()` encodes `rvnode://` URLs for dragged items | B | 🟡 |
| H6 | Drop within tree triggers `_sortTimer` to re-assign sort order | B | 🟡 |

### I — Source previews (thumbnail / filmstrip)

| ID | Behavior | Gate | Status |
|---|---|---|---|
| I1 | With `RV_SESSION_MANAGER_USE_THUMBNAILS=0`, previews disabled | P | ✅ |
| I2 | With previews enabled, source rows show `SourcePreviewWidget` | P | ✅ |
| I3 | Thumbnail path event returns cached path | B | 🟡 |
| I4 | Filmstrip path event returns cached path | B | 🟡 |
| I5 | `session-manager-preview-available` → row widget updated | P | ✅ |
| I6 | Config > Show Source Previews toggle persisted in settings | B+P | ✅ |
| I7 | Real mp4 → thumbnail visible in source row | P | ✅ |
| I8 | Real mp4 → filmstrip generated; meta label shows "mp4" | B+P | ✅ |
| I9 | Fallback pixmap shown when no thumbnail yet generated | P | ✅ |

### J — Editor tab (per-node type)

| ID | Behavior | Gate | Status |
|---|---|---|---|
| J1 | Tab index saved to `sm_state.tab` on view change | B | 🟡 |
| J2 | Tab state restored on `after-graph-view-change` | B | 🟡 |
| J3 | Selecting `RVSourceGroup` auto-switches to tab index 1 | B | 🟡 |
| J4 | `view-edit-mode-activated` → per-type edit widget loads | B | ⬜ |

### K — Config / startup

| ID | Behavior | Gate | Status |
|---|---|---|---|
| K1 | Config > Always Show → `showOnStartup=yes` | B | 🟡 |
| K2 | Config > Never Show → `showOnStartup=no` | B | 🟡 |
| K3 | Config > Restore Last → `showOnStartup=last` | B | 🟡 |

### L — Context menu

| ID | Behavior | Gate | Status |
|---|---|---|---|
| L1 | Right-click tree → context menu with Delete / Edit Info / Select Current | P | ⬜ |
| L2 | Context menu → Folder and Create submenus visible | P | ⬜ |

### M — Events

| ID | Behavior | Gate | Status |
|---|---|---|---|
| M1 | `new-node` → `updateTree()` | B | ⬜ |
| M2 | `after-node-delete` → `updateTree()` | B | ✅ |
| M3 | `after-clear-session` → `updateTree()` | B | ⬜ |
| M4 | `graph-node-inputs-changed` → `updateInputs(viewNode())` | B | ⬜ |
| M5 | `graph-state-change` on `ui.name` → `_lazyUpdateTimer` | B | ✅ |
| M6 | `graph-state-change` on `request.imageComponent` → sub-component icons update | B+P | ⬜ |
| M7 | `before/after-progressive-loading` → suppress tree updates during bulk load | B | ✅ |

### N — Splitter

| ID | Behavior | Gate | Status |
|---|---|---|---|
| N1 | Splitter move → `#Session.sm_window.splitter` float fraction written | B | 🟡 |

---

## Dropped behaviors (non-deterministic — covered by unit tests instead)

- **FilmstripWidget hover/scrub** (`showFrameAtX`, `mouseMoveEvent`, `HoverEnter/Leave`) — pointer-position dependent; covered in `unit/test_preview_widgets.py`
- **ThumbnailWidget.load / setFallback** — pure image load; covered in `unit/test_preview_widgets.py`
- **`sm_reopen_after_hide`** — dock close/reopen interaction with window minimize; timing-dependent modal state
- **`sm_toggle_diag`** — debug toggle diagnostic; no deterministic golden possible
- **`sm_thumb_diag` / `sm_thumb_diag2`** — thumbnail diagnostic/debug; covered by unit tests for `local_thumbnail_gen.py`

---

## Scenarios (25 gated golden tests)

`sm_folder_thumbnails` is also gated but was missing from this table;
`sm_folder_thumbnails_all` runs after the gates via `run_folder_thumbnails_all.sh`.

| ID | Primary outcome(s) | Coverage | Skip from |
|---|---|---|---|
| `sm_tree_categories` | #1 | A1, A2, A7, A9 | — |
| `sm_tree_folder_sort` | — | A5, A6 | — |
| `sm_select_node` | #2 | B1, B7, B8, B9 | — |
| `sm_subcomponent_select` | — | B3, B4, A3, A4 | — |
| `sm_nav_prev_next` | — | B5, B6 | — |
| `sm_add_sequence` | #5 | C1, G1, G3 | — |
| `sm_add_stack` | #5 | C2 | — |
| `sm_add_switch` | — | C3 | — |
| `sm_add_folder_empty` | — | D1 | — |
| `sm_add_folder_from_selection` | — | D2 | — |
| `sm_add_movieproc_black` | #4 | C9 | — |
| `sm_add_movieproc_solid` | #4 | C10, C16 | — |
| `sm_add_movieproc_bars` | #4 | C11 | — |
| `sm_add_movieproc_colorchart` | #4 | C12, C13 | — |
| `sm_delete_source` | #3 | E1, M2 | — |
| `sm_delete_folder` | — | E3 | — |
| `sm_inputs_reorder` | #7 | G4, G5 | — |
| `sm_inputs_sort` | #7 | G6, G7, G8 | — |
| `sm_inputs_delete` | — | E4 | — |
| `sm_rename_inline` | #6 | F3, F4, M5 | — |
| `sm_previews_toggle` | — | I1, I2, I6, I9 | — |
| `sm_meridian_mp4_load` | #8 | I5, I7, I8, M7 | — |
| `sm_media_add_sources` | — | I5, M7 | — |
| `sm_mp4_all` | #8 | I7, I8, G1, C1 | — |

---

## Mu methods → Python unit tests

**Mandatory gate 5.** The suite passes (424 tests) and is not vacuous, but the
coverage bar is **not met**: 189 of 335 Mu symbols still have no unit test. The table below is the full inventory across all twelve
Mu sources (the earlier version of this table listed 107 rows from
`session_manager.mu.in` only and omitted the eleven sibling modes entirely).

**Unit tests exercise the port.** Every module under `unit/` imports the real module
from `src/plugins/rv-packages/session_manager/` through `unit/_rv_stubs.py`, which
fakes only the `rv.*` bindings. This is worth stating because it was not previously
true: until this iteration all sixteen modules asserted against logic re-implemented
inside the test files, and the whole suite passed with `session_manager.py` deleted.
`harness/run_unit_tests.sh` now runs under RV's bundled interpreter (the tests need
the same PySide6 the port runs against) and fails if zero tests execute, so an
all-skipped run can no longer read as a pass.

| Mu source | Symbols | ✅ unit-tested | ⬜ not yet |
|---|---:|---:|---:|
| `session_manager.mu.in` | 144 | 69 | 75 |
| `Composite_edit_mode.mu` | 11 | 5 | 6 |
| `FolderGroup_edit_mode.mu` | 9 | 2 | 7 |
| `LayoutGroup_edit_mode.mu` | 33 | 13 | 20 |
| `RetimeGroup_edit_mode.mu` | 20 | 5 | 15 |
| `SequenceGroup_edit_mode.mu` | 19 | 7 | 12 |
| `SourceGroup_edit_mode.mu` | 23 | 9 | 14 |
| `StackGroup_edit_mode.mu` | 7 | 4 | 3 |
| `Stack_edit_mode.mu` | 21 | 6 | 15 |
| `SwitchGroup_edit_mode.mu` | 5 | 3 | 2 |
| `Switch_edit_mode.mu` | 18 | 3 | 15 |
| `transform_manip.mu` | 25 | 20 | 5 |
| **TOTAL** | **335** | **146** | **189** |


<details><summary><code>session_manager.mu.in</code> — 144 symbols, 58 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `EventFilter` | — | ⬜ |
| `FilmstripWidget` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `InputsView` | — | ⬜ |
| `NodeModel` | — | ⬜ |
| `NodeTreeView` | — | ⬜ |
| `SessionManagerMode` | — | ⬜ |
| `SourcePreviewWidget` | `unit/test_preview_widgets.py::TestSourcePreviewWidget` | ✅ |
| `ThumbnailWidget` | `unit/test_preview_widgets.py::TestThumbnailWidget` | ✅ |
| `activate` | — | ⬜ |
| `addEditor` | — | ⬜ |
| `addInput` | `unit/test_node_ops.py::TestAddInput` | ✅ |
| `addMovieProc` | — | ⬜ |
| `addNodeByTypeName` | — | ⬜ |
| `addNodeOfType` | — | ⬜ |
| `addRow` | `unit/test_helpers.py::TestAddRow` | ✅ |
| `addThingSlot` | — | ⬜ |
| `afterGraphViewChange` | — | ⬜ |
| `afterProgressiveLoading` | — | ⬜ |
| `assignSortOrder` | `unit/test_state_props.py::TestAssignSortOrder` | ✅ |
| `auxFilePath` | `unit/test_mode.py::TestAuxFilePath` | ✅ |
| `auxIcon` | — | ⬜ |
| `beforeGraphViewChange` | — | ⬜ |
| `beforeProgressiveLoading` | — | ⬜ |
| `chooseColorSlot` | — | ⬜ |
| `colorAdjustedIcon` | — | ⬜ |
| `componentAndFolderNodeFromHash` | — | ⬜ |
| `componentMatch` | `unit/test_helpers.py::TestComponentMatch` | ✅ |
| `configSlot` | `unit/test_mode.py::TestConfigSlot` | ✅ |
| `contains` | — | ⬜ |
| `createMode` | — | ⬜ |
| `deactivate` | — | ⬜ |
| `deleteViewableSlot` | — | ⬜ |
| `dragEnterEvent` | `unit/test_tree_view.py::TestDragEnterEvent` | ✅ |
| `dragMoveEvent` | `unit/test_tree_view.py::TestDragMoveEvent` | ✅ |
| `dropEvent` | — | ⬜ |
| `editViewInfoSlot` | — | ⬜ |
| `enterQuittingState` | — | ⬜ |
| `event` | `unit/test_preview_widgets.py::TestSourcePreviewWidget` | ✅ |
| `eventFilter` | — | ⬜ |
| `filteredDraggedPaths` | `unit/test_tree_view.py::TestFilteredDraggedPaths` | ✅ |
| `hasInput` | `unit/test_node_ops.py::TestHasInput` | ✅ |
| `hashedSubComponent` | `unit/test_hashed_subcomponent.py` | ✅ |
| `iconForNode` | `unit/test_mode.py::TestIconForNode` | ✅ |
| `includes` | `unit/test_helpers.py::TestIncludes` | ✅ |
| `indexOf` | — | ⬜ |
| `indexOfItem` | — | ⬜ |
| `inputRowsInsertedSlot` | — | ⬜ |
| `inputRowsRemovedSlot` | — | ⬜ |
| `inputsDeleteSlot` | — | ⬜ |
| `isExpandedInParent` | `unit/test_state_props.py::TestExpandedInParent` | ✅ |
| `isImageRequestPropEqual` | `unit/test_image_request.py::TestIsImageRequestPropEqual` | ✅ |
| `isLoaded` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `isSubComponentExpanded` | `unit/test_state_props.py::TestSubComponentExpanded` | ✅ |
| `itemIsSubComponent` | `unit/test_helpers.py::TestSubComponentType` | ✅ |
| `itemNode` | `unit/test_helpers.py::TestItemNode` | ✅ |
| `itemOfNode` | `unit/test_helpers.py::TestMapItems` | ✅ |
| `itemParentNode` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemPressed` | — | ⬜ |
| `itemSubComponentHash` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemSubComponentMedia` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemSubComponentStringData` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemSubComponentType` | `unit/test_helpers.py::TestSubComponentType` | ✅ |
| `itemSubComponentTypeForName` | `unit/test_helpers.py::TestSubComponentTypeForName` | ✅ |
| `itemSubComponentValue` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `load` | `unit/test_preview_widgets.py` | ✅ |
| `loadStrip` | — | ⬜ |
| `loadThumbnail` | — | ⬜ |
| `mainWinVisTimeout` | — | ⬜ |
| `makeImage` | — | ⬜ |
| `makeNewNodeOfType` | — | ⬜ |
| `makeSourceRowWidget` | — | ⬜ |
| `map` | `unit/test_helpers.py::TestMapItems` | ✅ |
| `mapOverItem` | — | ⬜ |
| `mimeData` | `unit/test_node_model.py::TestMimeData` | ✅ |
| `mimeTypes` | `unit/test_node_model.py::TestMimeTypes` | ✅ |
| `mouseMoveEvent` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `navButtonClicked` | `unit/test_mode.py::TestNavButtonClicked` | ✅ |
| `newColorSlot` | — | ⬜ |
| `newFolderSlot` | — | ⬜ |
| `newNodeRow` | — | ⬜ |
| `newNodeStatusColumns` | — | ⬜ |
| `newNodeSubComponent` | — | ⬜ |
| `newSubComponentNode` | — | ⬜ |
| `nodeFromIndex` | `unit/test_helpers.py::TestNodeFromIndex` | ✅ |
| `nodeInputs` | `unit/test_helpers.py::TestNodeInputs` | ✅ |
| `nodeInputsChanged` | — | ⬜ |
| `onCategoryStateChanged` | — | ⬜ |
| `printRows` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `rebuildInputsFromList` | — | ⬜ |
| `reloadEditorTab` | — | ⬜ |
| `remove` | — | ⬜ |
| `removeInput` | `unit/test_node_ops.py::TestRemoveInput` | ✅ |
| `renameByType` | `unit/test_rename.py::TestRenameByType` | ✅ |
| `reorderSelected` | — | ⬜ |
| `resizeColumns` | `unit/test_helpers.py::TestResizeColumns` | ✅ |
| `restoreTabState` | `unit/test_mode.py::TestTabState` | ✅ |
| `saveTabState` | `unit/test_mode.py::TestTabState` | ✅ |
| `selectCurrentViewSlot` | — | ⬜ |
| `selectInputsRange` | — | ⬜ |
| `selectViewableNode` | — | ⬜ |
| `selectedConvertedSubComponents` | — | ⬜ |
| `selectedItems` | — | ⬜ |
| `selectedNodePaths` | `unit/test_tree_view.py::TestSelectedNodePaths` | ✅ |
| `selectedNodes` | `unit/test_sort_inputs.py (via selectedNodesEvent)` | ✅ |
| `selectedNodesEvent` | `unit/test_cross_package_api.py::TestSelectedNodesEvent` | ✅ |
| `setExpandedInParent` | `unit/test_state_props.py::TestExpandedInParent` | ✅ |
| `setFallback` | `unit/test_preview_widgets.py::TestThumbnailWidget` | ✅ |
| `setImageRequest` | `unit/test_image_request.py::TestSetImageRequestToggle` | ✅ |
| `setImageRequestProp` | `unit/test_image_request.py::TestSetImageRequestProp` | ✅ |
| `setInputs` | `unit/test_node_ops.py::TestSetInputs` | ✅ |
| `setItemExpandedState` | — | ⬜ |
| `setNodeRequest` | `unit/test_image_request.py::TestSetNodeRequest` | ✅ |
| `setNodeStatus` | `unit/test_mode.py::TestSetNodeStatus` | ✅ |
| `setSortKeyInParent` | `unit/test_state_props.py::TestSortKey` | ✅ |
| `setSubComponentExpanded` | `unit/test_state_props.py::TestSubComponentExpanded` | ✅ |
| `setToolTipProp` | `unit/test_state_props.py::TestToolTipProp` | ✅ |
| `showFrameAtX` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `showRows` | — | ⬜ |
| `sortFolderChildren` | `unit/test_tree_view.py::TestSortFolderChildren` | ✅ |
| `sortFolders` | `unit/test_tree_view.py::TestSortFolders` | ✅ |
| `sortInputs` | `unit/test_sort_inputs.py` | ✅ |
| `sortKeyInParent` | `unit/test_state_props.py::TestSortKey` | ✅ |
| `sourceFromSubComponent` | — | ⬜ |
| `sourceNodeOfGroup` | `unit/test_helpers.py::TestSourceNodeOfGroup` | ✅ |
| `splitterMoved` | `unit/test_mode.py::TestSplitterMoved` | ✅ |
| `subComponentItemsOfNode` | `unit/test_helpers.py::TestSubComponentItemsOfNode` | ✅ |
| `subComponentPropValue` | `unit/test_subcomponent_prop.py` | ✅ |
| `tabChangeSlot` | `unit/test_mode.py::TestTabState` | ✅ |
| `theMode` | — | ⬜ |
| `togglePreviews` | `unit/test_mode.py::TestTogglePreviews` | ✅ |
| `toolTipFromProp` | `unit/test_state_props.py::TestToolTipProp` | ✅ |
| `updateInputs` | — | ⬜ |
| `updateNavUI` | — | ⬜ |
| `updateNodePreviewEvent` | — | ⬜ |
| `updateTree` | — | ⬜ |
| `updateTreeEvent` | — | ⬜ |
| `useEditor` | — | ⬜ |
| `viewByIndex` | — | ⬜ |
| `viewContextMenuSlot` | — | ⬜ |
| `viewEditModeActivated` | — | ⬜ |
| `viewItemChanged` | — | ⬜ |
| `viewSelectionChanged` | — | ⬜ |
| `visibilityChanged` | — | ⬜ |

</details>

<details><summary><code>Composite_edit_mode.mu</code> — 11 symbols, 5 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `CompositeEditMode` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `createMode` | — | ⬜ |
| `loadUI` | — | ⬜ |
| `opState` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `setDissolveAmount` | `unit/test_composite_edit_mode.py::TestDissolveAmount` | ✅ |
| `setDissolveAmountFromSlider` | `unit/test_composite_edit_mode.py::TestDissolveAmount` | ✅ |
| `setOp` | `unit/test_composite_edit_mode.py::TestSetOp` | ✅ |
| `setOpEvent` | `unit/test_composite_edit_mode.py::TestSetOp` | ✅ |
| `updateUI` | `unit/test_composite_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>FolderGroup_edit_mode.mu</code> — 9 symbols, 2 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `FolderGroupEditMode` | — | ⬜ |
| `activate` | — | ⬜ |
| `activateUI` | — | ⬜ |
| `createMode` | — | ⬜ |
| `deactivate` | — | ⬜ |
| `loadUI` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `setViewType` | `unit/test_folder_group_edit_mode.py::TestSetViewType` | ✅ |
| `updateUI` | `unit/test_folder_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>LayoutGroup_edit_mode.mu</code> — 33 symbols, 13 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `LayoutGroupEditMode` | — | ⬜ |
| `activate` | — | ⬜ |
| `activateTransformMode` | — | ⬜ |
| `activateUI` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `createMode` | — | ⬜ |
| `deactivate` | — | ⬜ |
| `gridColumnsChangedSlot` | — | ⬜ |
| `gridRowsChangedSlot` | — | ⬜ |
| `isLayoutMode` | `unit/test_layout_group_edit_mode.py::TestIsLayoutMode` | ✅ |
| `layoutInColumn` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutInColumnEvent` | — | ⬜ |
| `layoutInGrid` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutInGridEvent` | — | ⬜ |
| `layoutInRow` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutInRowEvent` | — | ⬜ |
| `layoutManually` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutManuallyEvent` | — | ⬜ |
| `layoutMode` | `unit/test_layout_group_edit_mode.py::TestLayoutMode` | ✅ |
| `layoutPacked` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutPacked2` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutPacked2Event` | — | ⬜ |
| `layoutPackedEvent` | — | ⬜ |
| `layoutStatic` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutStaticEvent` | — | ⬜ |
| `loadUI` | — | ⬜ |
| `modeComboChangedSlot` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `setGridRowsColumns` | `unit/test_layout_group_edit_mode.py::TestSpacingAndGrid` | ✅ |
| `setLayoutMode` | `unit/test_layout_group_edit_mode.py::TestLayoutMode` | ✅ |
| `setSpacing` | `unit/test_layout_group_edit_mode.py::TestSpacingAndGrid` | ✅ |
| `spacingSliderChangedSlot` | — | ⬜ |
| `updateUI` | `unit/test_layout_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>RetimeGroup_edit_mode.mu</code> — 20 symbols, 5 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `RetimeGroupEditMode` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `convertToFPS` | — | ⬜ |
| `createMode` | — | ⬜ |
| `editSlot` | — | ⬜ |
| `factorPrompt` | — | ⬜ |
| `fpsPrompt` | — | ⬜ |
| `loadUI` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `reset` | `unit/test_retime_group_edit_mode.py::TestReset` | ✅ |
| `resetSlot` | — | ⬜ |
| `resetTiming` | — | ⬜ |
| `reverse` | `unit/test_retime_group_edit_mode.py::TestReverse` | ✅ |
| `reverseSlot` | — | ⬜ |
| `reverseTiming` | — | ⬜ |
| `setConvertFPS` | `unit/test_retime_group_edit_mode.py::TestSetConvertFPS` | ✅ |
| `setFactorValue` | `unit/test_retime_group_edit_mode.py::TestSetFactorValue` | ✅ |
| `slowDownPrompt` | — | ⬜ |
| `speedUpPrompt` | — | ⬜ |
| `updateUI` | `unit/test_retime_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>SequenceGroup_edit_mode.mu</code> — 19 symbols, 7 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SequenceGroupEditMode` | — | ⬜ |
| `activate` | — | ⬜ |
| `activateUI` | — | ⬜ |
| `afterSessionRead` | `unit/test_sequence_group_edit_mode.py::TestSessionReadFreeze` | ✅ |
| `autoEDL` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `beforeSessionRead` | `unit/test_sequence_group_edit_mode.py::TestSessionReadFreeze` | ✅ |
| `checkBoxSlot` | `unit/test_sequence_group_edit_mode.py::TestCheckBoxSlot` | ✅ |
| `createMode` | — | ⬜ |
| `fpsChanged` | `unit/test_sequence_group_edit_mode.py::TestFpsChanged` | ✅ |
| `heightChanged` | `unit/test_sequence_group_edit_mode.py::TestSizeEdits` | ✅ |
| `loadUI` | — | ⬜ |
| `menu` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `stateFunc` | — | ⬜ |
| `updateUI` | `unit/test_sequence_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |
| `updateUIEvent` | — | ⬜ |
| `useCutInfo` | — | ⬜ |
| `widthChanged` | `unit/test_sequence_group_edit_mode.py::TestSizeEdits` | ✅ |

</details>

<details><summary><code>SourceGroup_edit_mode.mu</code> — 23 symbols, 9 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SourceGroupEditMode` | — | ⬜ |
| `activate` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `changedSlot` | — | ⬜ |
| `createMode` | — | ⬜ |
| `cutInPrompt` | `unit/test_source_group_edit_mode.py::TestPrompts` | ✅ |
| `cutOutPrompt` | `unit/test_source_group_edit_mode.py::TestPrompts` | ✅ |
| `finishedSlot` | — | ⬜ |
| `loadUI` | — | ⬜ |
| `newInPoint` | `unit/test_source_group_edit_mode.py::TestNewInOutPoint` | ✅ |
| `newOutPoint` | `unit/test_source_group_edit_mode.py::TestNewInOutPoint` | ✅ |
| `propertyChanged` | — | ⬜ |
| `reset` | `unit/test_source_group_edit_mode.py::TestReset` | ✅ |
| `resetCut` | `unit/test_source_group_edit_mode.py::TestReset` | ✅ |
| `resetSlot` | — | ⬜ |
| `setCutValue` | `unit/test_source_group_edit_mode.py::TestSetCutValue` | ✅ |
| `sourceMenuState` | — | ⬜ |
| `syncGuiInOut` | — | ⬜ |
| `syncSlot` | `unit/test_source_group_edit_mode.py::TestSyncSlot` | ✅ |
| `syncState` | — | ⬜ |
| `toggleSync` | — | ⬜ |
| `updateFromProps` | — | ⬜ |
| `updateUI` | `unit/test_source_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>StackGroup_edit_mode.mu</code> — 7 symbols, 0 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `StackGroupEditMode` | — | ⬜ |
| `activate` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |
| `activateUI` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |
| `auxFilePath` | — | ⬜ |
| `createMode` | — | ⬜ |
| `deactivate` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |
| `propertyChanged` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |

</details>

<details><summary><code>Stack_edit_mode.mu</code> — 21 symbols, 6 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `StackEditMode` | — | ⬜ |
| `activate` | — | ⬜ |
| `alignStartFrames` | — | ⬜ |
| `autoRetimeInputs` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `checkBoxSlot` | `unit/test_stack_edit_mode.py::TestCheckBoxSlot` | ✅ |
| `createMode` | — | ⬜ |
| `fpsChanged` | `unit/test_stack_edit_mode.py::TestFpsChanged` | ✅ |
| `heightChanged` | `unit/test_stack_edit_mode.py::TestSizeEdits` | ✅ |
| `loadUI` | — | ⬜ |
| `menu` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `retimeState` | — | ⬜ |
| `setChosenAudioInput` | `unit/test_stack_edit_mode.py::TestSetChosenAudioInput` | ✅ |
| `stateFunc` | — | ⬜ |
| `strictFrameRanges` | — | ⬜ |
| `updateMenu` | — | ⬜ |
| `updateUI` | `unit/test_stack_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |
| `updateUIEvent` | — | ⬜ |
| `useCutInfo` | — | ⬜ |
| `widthChanged` | `unit/test_stack_edit_mode.py::TestSizeEdits` | ✅ |

</details>

<details><summary><code>SwitchGroup_edit_mode.mu</code> — 5 symbols, 0 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SwitchGroupEditMode` | — | ⬜ |
| `activate` | `unit/test_group_edit_modes.py::TestSwitchGroupEditMode` | ✅ |
| `activateUI` | `unit/test_group_edit_modes.py::TestSwitchGroupEditMode` | ✅ |
| `createMode` | — | ⬜ |
| `deactivate` | `unit/test_group_edit_modes.py::TestSwitchGroupEditMode` | ✅ |

</details>

<details><summary><code>Switch_edit_mode.mu</code> — 18 symbols, 3 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SwitchEditMode` | — | ⬜ |
| `activate` | — | ⬜ |
| `alignStartFrames` | — | ⬜ |
| `auxFilePath` | — | ⬜ |
| `checkBoxSlot` | `unit/test_switch_edit_mode.py::TestCheckBoxSlot` | ✅ |
| `createMode` | — | ⬜ |
| `heightChanged` | — | ⬜ |
| `loadUI` | — | ⬜ |
| `menu` | — | ⬜ |
| `propertyChanged` | — | ⬜ |
| `retimeState` | — | ⬜ |
| `setSelectedInput` | `unit/test_switch_edit_mode.py::TestSetSelectedInput` | ✅ |
| `stateFunc` | — | ⬜ |
| `updateMenu` | — | ⬜ |
| `updateUI` | `unit/test_switch_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |
| `updateUIEvent` | — | ⬜ |
| `useCutInfo` | — | ⬜ |
| `widthChanged` | — | ⬜ |

</details>

<details><summary><code>transform_manip.mu</code> — 25 symbols, 6 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `TransformManip` | — | ⬜ |
| `activate` | — | ⬜ |
| `activeImageIndex` | `unit/test_transform_manip_mode.py::TestActiveImageIndex` | ✅ |
| `afterGraphViewChange` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `beforeGraphViewChange` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `closestPointOnLine` | `unit/test_transform_manip.py::TestClosestPointOnLine` | ✅ |
| `computeGC` | `unit/test_transform_manip.py::TestComputeGC` | ✅ |
| `control` | `unit/test_transform_manip_mode.py::TestControlHitTest` | ✅ |
| `createMode` | — | ⬜ |
| `deactivate` | `unit/test_transform_manip_pointer.py::TestDeactivate` | ✅ |
| `drag` | `unit/test_transform_manip_pointer.py::TestDragFreeTranslation` | ✅ |
| `drawCorners` | — | ⬜ |
| `editNode` | `unit/test_transform_manip_mode.py::TestEditNode` | ✅ |
| `findEditingNodes` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `fitAll` | `unit/test_transform_manip_mode.py::TestFitAll` | ✅ |
| `move` | `unit/test_transform_manip_pointer.py::TestMove` | ✅ |
| `nodeAspect` | `unit/test_transform_manip_mode.py::TestNodeAspect` | ✅ |
| `nodeInputsChanged` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `push` | `unit/test_transform_manip_pointer.py::TestPush` | ✅ |
| `release` | `unit/test_transform_manip_pointer.py::TestRelease` | ✅ |
| `removeTags` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `render` | — | ⬜ |
| `resetAll` | `unit/test_transform_manip_mode.py::TestResetAll` | ✅ |
| `setManipState` | `unit/test_transform_manip_mode.py::TestSetManipState` | ✅ |
| `tagValue` | `unit/test_transform_manip.py::TestTagValue` | ✅ |

</details>


### What the remaining ⬜ rows are

Mostly three kinds, none of them covered by a golden scenario either:

- **Mode lifecycle and tree construction** in `session_manager.mu.in` — `updateTree`,
  `newNodeRow`, `makeSourceRowWidget`, `activate`/`deactivate`, the slot handlers.
  These need a constructed dock widget, so they want either a fixture that builds the
  mode against the fake graph or an in-RV test.
- **Menu state functions and event wrappers** across the siblings — one-line
  delegations (`xEvent` → `x()`), cheap to cover once a per-mode fixture exists.
- **`StackGroup_edit_mode` and `SwitchGroup_edit_mode`** — no tests at all yet; both
  are small.

---

## Code review outcome (2026-08-04)

Two independent agents reviewed this iteration: one comparing each `.py` port against
its `.mu` ground truth, one on the core/harness/test changes. Every blocking finding
below was fixed and re-verified; the loop passes all six gates afterwards.

### Defects in the port

| Severity | Defect | Fix |
|---|---|---|
| high | 12 sites called `int(Qt.CursorShape.X)`, which raises `TypeError` under PySide6 6.5 — the same trap as `Qt.CheckState`. `move()` raised before it could find an edit node, so the **transform manipulator never worked at all**, and `deactivate()` raised before `removeTags()`, leaving `tag.tmanip` properties to be saved into session files | `.value`, plus `unit/test_transform_manip_pointer.py` |
| high | `drag()` computed the corner diagonal unconditionally. `control()` returns the centroid as the grab point for a non-corner grab, so a free-translation drag normalised `(0,0)` and raised `ZeroDivisionError` on every event. Mu survives because its float division yields NaN and that branch never reads the values | diagonal moved into the corner branch — guarding `normalize()` is **not** sufficient, since a zero direction makes `/ downDist` raise next |
| high | The cross-package API regressed: RV dispatches internal events only to **active** modes, and `session_manager` is `load: delay`, so with the panel closed `sessionManagerSelectedNodes()` returned empty and rvnuke/maya_tools menu states silently flipped. `sourceSelected()` returns Neutral for an empty list, so its item was *enabled and did nothing* | read the mode through `selectedNodeLines()` (restores the pre-migration semantics), event kept as the Mu fallback, plus an empty-selection guard |
| low | Mu's settings self-repair `catch` is unreachable in Python (`readSettings` coerces, `str()` cannot raise), so a corrupt `showOnStartup` is left rather than reset | recorded, not fixed — needs a product call |
| low | drag `text/plain` renders a media list as `['a.mov']` where Mu renders `string[] {"a.mov"}` | recorded; no in-RV consumer |

### Defects in the gate itself

| Severity | Defect | Fix |
|---|---|---|
| high | `set -euo pipefail` plus `_out="$(...)"` aborted `run_unit_tests.sh` before printing why a run failed — a 350-test failure reported only "GATE 5 FAILED" | capture with `\|\| _rc=$?`; note `if ! cmd` does *not* work, as the negation makes `$?` read 0 |
| high | `FakeGraph` treated `set*Property`'s third argument as create-if-missing. It is `allowResize`; RV throws `badProperty` first, which is why `cprop` exists. Gutting `_cprop` in the port left **every test passing** | stub made strict, 81 seeding sites moved to an explicit `seedInt/Float/String` API. Gutting `_cprop` now fails 59 tests, and the migration surfaced 6 more wrong assertions |
| high | `GOLDEN_PYTHON` skipped the PySide6 check, so a stock interpreter made every module skip and the gate pass on zero tests | override is checked too; the "0 tests ran" guard now reads the executed count from either runner |
| medium | `stage_python_modes()` never removed orphans, and staged dirs precede the package on `PYTHONPATH` — a deleted module kept loading from its stale staged copy, so gates passed against code no longer in the tree | per-package manifest; orphans are un-staged and logged |
| medium | The automatic Mu fallback in `mode_manager.mu` was unsound: a Python mode registers itself inside `init()`, so a later constructor failure left it registered and the Mu module's own `defineMinorMode` threw "Duplicate mode" — swallowed into a `showWarning` that is silent without `-ModeManagerVerbose`. Net effect: no mode and no message | fallback removed; a Python implementation that exists and fails now reports loudly |
| medium | No test covered the cross-package API at all; both entry points survived mutation to `return None` | `unit/test_cross_package_api.py` |
| low | Two tests could not fail (`updateUI` freeze with `_ui` None; a hover assertion that `QWidget.event` already satisfies) | one removed as redundant, one rewritten to test the override's side effect |

### Confirmed sound

Both reviewers checked and found faithful: all ~56 property-setter overload choices,
the Mu cons-list ordering elsewhere in the port, `nil`/`None`/`""` distinctions,
index arithmetic, exception scope and flag leakage, every other PySide6 enum use, the
`requestedModeImpl` precedence, the `find_spec` probe primitives, and test isolation.
