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

**State**, per [Definition of done](../VERIFICATION.md#definition-of-done):

| # | Item | State |
|---|---|---|
| 1, 6 | Every inventory item pinned | **no ⬜ rows** — 64 ✅ (committed golden) / 25 🟡 (unit test only, the row is not reproducible headlessly) / 1 ❌ (pre-existing defect, C7). See [Behavior inventory](#behavior-inventory) |
| 3 | Gate 5 coverage bar (no untested Mu-method rows) | **met** — 329 of 335 Mu symbols unit-tested, the other 6 marked ➖ with the reason and what covers them instead (1078 tests) |
| 4 | GUI sanity pixel review | **done** — 37/37 behavioral PASS on a real display, and all 60 PNG pairs byte-identical to `golden-mac/`, so there is no rendering difference to attribute to noise or to a regression |
| 8 | Independent code-review agent | **run** — two fresh agents (port fidelity vs Mu; infrastructure + tests). All blocking findings fixed and verified; see below |

Gate 5's suite passes and is not vacuous: the tests import the port, and hiding
`session_manager.py` makes the gate fail.

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

Coverage legend: **✅** = pinned by a committed Mu golden; **🟡** = pinned by a unit test only, because the row needs a pointer, a modal dialog or a focused window the headless harness cannot produce; **❌** = pre-existing defect, not a port regression.

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

**Status.** Gates 0-2 pass for all 38 gated scenarios against the Python port, and
gate 4 confirms Mu still matches the same committed goldens, so every row marked ✅
below is pinned in both implementations.

🟡 means a unit test pins the behaviour but no golden scenario can: the row needs
something the headless harness cannot produce. That is H1-H6 (a pointer drag), L1-L2
and D4 (a context menu — `QMenu.exec` blocks), C8 and C15 (a modal dialog), B2, G10
and F1-F2 (a focused double-click or key press), A8 and H4 (drop-target state visible
only mid-drag), I3-I4 (preview path events), and J1-J3, K1-K3 and N1 (tab state,
config and splitter, all written on paths a scenario cannot reach without one of the
above). Each names its test in the row. ✅ is reserved for behaviours a committed
golden pins, per the legend.


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
| A8 | Folder nodes are drop-targets; non-folder category items are not | P | 🟡 |
| A9 | Node type → correct icon (RVSourceGroup=videofile, RVStackGroup=photoalbum, etc.) | P | ✅ |
| A10 | Tree column widths auto-resize to content | P | ✅ |

### B — Node selection & view navigation

| ID | Behavior | Gate | Status |
|---|---|---|---|
| B1 | Single-click top-level item → `setViewNode()`; inputs panel updates | B+P | ✅ |
| B2 | Double-click top-level item → `viewByIndex()` | B | 🟡 |
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
| C4 | Add > Layout → new `RVLayoutGroup` | B | ✅ |
| C5 | Add > Retime → new `RVRetimeGroup` | B | ✅ |
| C6 | Add > Color → new `RVColor` node | B | ✅ |
| C7 | Add > OCIO → new `RVOCIO` node | B | ❌ |
| C8 | Add > New Node by Type… → dialog shows all node types; creates chosen type | B | 🟡 |
| C9 | Add > Black… → `black,*.movieproc` source added; named "Black" | B+P | ✅ |
| C10 | Add > Color… → `solid,*.movieproc` with chosen RGB | B+P | ✅ |
| C11 | Add > Color Bars… → `smptebars,*.movieproc`; color controls hidden | B+P | ✅ |
| C12 | Add > SRGB Color Chart… → `srgbcolorchart,*.movieproc` | B | ✅ |
| C13 | Add > ACES Color Chart… → `acescolorchart,*.movieproc` | B | ✅ |
| C14 | Add > Blank… → `blank,*.movieproc`; width/height hidden | B | ✅ |
| C15 | Create Image dialog FPS defaults from `General/fps` setting | B | 🟡 |
| C16 | Color picker in dialog updates button background and `_cidColor` | P | ✅ |

### D — Folder operations

| ID | Behavior | Gate | Status |
|---|---|---|---|
| D1 | Folder > Empty Folder → new `RVFolderGroup` with no inputs; named "Empty Folder" | B+P | ✅ |
| D2 | Folder > From Selection → folder wraps selected nodes; removes from current parent | B+P | ✅ |
| D3 | Folder > From Copy of Selection → folder wraps copies; original parent connections unchanged | B | ✅ |
| D4 | Context menu → Folder submenu mirrors folder button menu | P | 🟡 |

### E — Delete operations

| ID | Behavior | Gate | Status |
|---|---|---|---|
| E1 | Delete button on selected source → `deleteNode()` | B+P | ✅ |
| E2 | Delete on a node in **more than one folder** → `removeInput()`; one folder plus another parent type still deletes outright | B | ✅ |
| E3 | Delete folder → `deleteNode(folder)` | B+P | ✅ |
| E4 | Inputs panel delete button → removes selected inputs from `viewNode()` connections | B | ✅ |
| E5 | Delete with multiple selection deletes all selected | B | ✅ |

### F — Rename / inline edit

| ID | Behavior | Gate | Status |
|---|---|---|---|
| F1 | F2 / Edit key → inline edit activated | B+P | 🟡 |
| F2 | Edit Info button → `_viewTreeView.edit(index)` | B+P | 🟡 |
| F3 | Rename on tree item → `setUIName(node, new_text)` | B+P | ✅ |
| F4 | `ui.name` change event → `_lazyUpdateTimer` fires → tree label refreshed | B | ✅ |

### G — Inputs panel

| ID | Behavior | Gate | Status |
|---|---|---|---|
| G1 | Inputs panel shows `nodeConnections(viewNode())._0` | B+P | ✅ |
| G2 | Source inputs show preview widget (thumbnail + name + meta) when previews enabled | P | ✅ |
| G3 | Non-source inputs show icon + `uiName` | B+P | ✅ |
| G4 | Order Up moves selected input(s) one position toward top | B+P | ✅ |
| G5 | Order Down moves selected input(s) one position toward bottom | B+P | ✅ |
| G6 | Sort A-Z sorts all inputs alphabetically ascending; sets node connections | B+P | ✅ |
| G7 | Sort Z-A sorts descending | B+P | ✅ |
| G8 | Folder node sort also updates `sm_state.sortKey` on each child | B | ✅ |
| G9 | Inputs panel disabled for `RVSourceGroup` and `RVFileSource` nodes | P | ✅ |
| G10 | Double-click input → `viewByIndex()` sets that node as view | B | 🟡 |

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
| J4 | `view-edit-mode-activated` → per-type edit widget loads | B | ✅ |

### K — Config / startup

| ID | Behavior | Gate | Status |
|---|---|---|---|
| K1 | Config > Always Show → `showOnStartup=yes` | B | 🟡 |
| K2 | Config > Never Show → `showOnStartup=no` | B | 🟡 |
| K3 | Config > Restore Last → `showOnStartup=last` | B | 🟡 |

### L — Context menu

| ID | Behavior | Gate | Status |
|---|---|---|---|
| L1 | Right-click tree → context menu with Delete / Edit Info / Select Current | P | 🟡 |
| L2 | Context menu → Folder and Create submenus visible | P | 🟡 |

### M — Events

| ID | Behavior | Gate | Status |
|---|---|---|---|
| M1 | `new-node` → `updateTree()` | B | ✅ |
| M2 | `after-node-delete` → `updateTree()` | B | ✅ |
| M3 | `after-clear-session` → `updateTree()` | B | ✅ |
| M4 | `graph-node-inputs-changed` → `updateInputs(viewNode())` | B | ✅ |
| M5 | `graph-state-change` on `ui.name` → `_lazyUpdateTimer` | B | ✅ |
| M6 | `graph-state-change` on `request.imageComponent` → sub-component icons update | B+P | ✅ |
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

### Headless limitations (cannot be pinned by a golden on this harness)

`run_scenario.py` drives RV through `-pyeval`, before the Qt event loop, with no real
window focus. Some triggers do not propagate there, which is already noted inside
`sm_select_node` for the single-click case:

| Row | Why | Covered by |
|---|---|---|
| B2, G10, F1, F2 | A synthesised double-click on a tree row / inputs row does not reach `viewByIndex()` headlessly. Re-verified by writing the scenario and running it: `viewNode()` stays on the previous node under **Mu** as well as under Python, so it is a harness limitation, not a port defect, and the scenario was dropped rather than committed with a baseline that pins nothing | `unit/test_mode_interactions.py::TestViewByIndex`, `::TestEditViewInfoSlot`, `unit/test_mode_slots.py::TestItemPressed` → 🟡 |
| C15 | The Create Image dialog is modal; VERIFICATION.md drops modal UI from the golden inventory | `unit/test_mode_interactions.py::TestCreateImageDialogDefaults` → 🟡 |
| A8 | A real drag needs a pointer grab and a live event loop, which `-pyeval` has not got. The policy is pinned where it is decided instead: `dragEnterEvent` clearing the FOLDERS row's `ItemIsDropEnabled` for a non-folder drag, and `dragMoveEvent`'s rejection rules | `unit/test_mode_interactions.py::TestFolderDropTargets` + `unit/test_tree_view.py` → 🟡 |
| C8 | The New Node by Type dialog is modal. The type list it is filled from and the creation path it feeds are pinned instead | `unit/test_mode_interactions.py::TestNewNodeByTypeDialog` → 🟡 |
| L1, L2, D4 | The context menu is shown with `QMenu.exec()`, which blocks until dismissed and never returns headlessly. Its *construction* is checked instead — including that the Folder submenu is literally the same QMenu object as the folder button's, so the two cannot drift | `unit/test_mode_interactions.py::TestContextMenuConstruction` → 🟡 |
| M3 | Grabbing the dock's widget *after* `clearSession()` segfaults RV — reproduced on Mu, so pre-existing. `sm_tree_event_clear` grabs the panel before the clear and asserts the post-clear state on the model and the saved graph | `sm_tree_event_clear` (✅, panel PNG pre-clear) |
| I7, I8 (at 83 clips) | The full-folder variant `sm_folder_thumbnails_all` is gated behaviorally, not on pixels. At 83 rows Qt re-rasterises the row labels with different subpixel weights between two runs of the *same* implementation — same glyphs, same layout, glyph-edge deltas up to 167/255 — so no meaningful dmax absorbs it. What the scenario is for is asserted on the graph and the thumbnail cache instead: 83 thumbnails and 83 filmstrips written, 83 rows off the fallback icon, 83 *distinct* row images. The pixel-exact check on the same panel is `sm_folder_thumbnails` at 12 clips, which does capture deterministically | `sm_folder_thumbnails` (✅, dmax 0) + `sm_folder_thumbnails_all` assertions |

### Known pre-existing defects (not port regressions)

| Row | Defect |
|---|---|
| C7 | **Add ▸ OCIO cannot work.** Both `session_manager.mu.in` and `session_manager.py` pass `"RVOCIO"` to `newNode`, and this build has no such node type — it ships `OCIO`, `OCIODisplay`, `OCIOFile`, `OCIOLook`. The action raises in either implementation. It is also unpinnable by a golden: the traceback names `session_manager.py` frames under Python and Mu frames under Mu, so gate 0 reports a new signature whichever implementation captured the baseline. `sm_add_node_types` asserts `RVOCIO` is still absent, so if the node type ever appears the scenario fails and the row can be pinned properly. Marked ❌ rather than ⬜ — it is not missing coverage, it is a defect upstream of this migration. |
| — | **`nodeAspect(node)` ignores its argument**, measuring `viewNode()` instead (`transform_manip.mu:294`), so `fitAll`'s scale is always `1.0` and "Fit All Images" only resets transforms. Pinned as-is in `unit/test_transform_manip_mode.py`. |

## Scenarios (37 gated golden tests)

`sm_folder_thumbnails_all` is the 38th golden directory; it runs after the gates via
`run_folder_thumbnails_all.sh` rather than inside them, because it loads every clip
in the fixture folder and takes minutes.

| ID | Primary outcome(s) | Coverage | Skip from |
|---|---|---|---|
| `sm_tree_categories` | #1 | A1, A2, A7, A9 | — |
| `sm_tree_columns` | — | A10 | — |
| `sm_tree_event_newnode` | — | M1, M4 | — |
| `sm_tree_event_clear` | — | M3 | — |
| `sm_subcomponent_icons` | — | M6 | — |
| `sm_add_node_types` | — | C4, C5, C6, C7 | — |
| `sm_add_movieproc_blank` | — | C14 | — |
| `sm_folder_from_copy` | — | D3 | — |
| `sm_delete_multi` | — | E5 | — |
| `sm_delete_in_folder` | — | E2 | — |
| `sm_inputs_disabled_for_source` | — | G9 | — |
| `sm_inputs_preview_widget` | — | G2 | — |
| `sm_editor_tab_per_type` | — | J4 | — |
| `sm_folder_thumbnails` | #8 | I2, I5, I7, I8, I9, M7 | — |
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

**Mandatory gate 5.** The suite passes (1078 tests) and is not vacuous. Every one of
the 335 Mu symbols across all twelve Mu sources now maps to either a unit test on the
ported symbol or a ➖ row explaining why there is nothing to test and naming what does
cover it. (An earlier version of this table listed 107 rows from
`session_manager.mu.in` only and omitted the eleven sibling modes entirely.)

**Unit tests exercise the port.** Every module under `unit/` imports the real module
from `src/plugins/rv-packages/session_manager/` through `unit/_rv_stubs.py`, which
fakes only the `rv.*` bindings. This is worth stating because it was not previously
true: until this iteration all sixteen modules asserted against logic re-implemented
inside the test files, and the whole suite passed with `session_manager.py` deleted.
`harness/run_unit_tests.sh` now runs under RV's bundled interpreter (the tests need
the same PySide6 the port runs against) and fails if zero tests execute, so an
all-skipped run can no longer read as a pass.

**Statuses.** ✅ a unit test exercises the ported symbol. ➖ there is no ported
symbol to test — either the Mu helper was inlined as a Python built-in, or it is dead
in Mu, or it cannot be reached headlessly and is pinned by the golden gates instead.
Each ➖ row says which, and names whatever does cover it.

| Mu source | Symbols | ✅ unit-tested | ➖ n/a |
|---|---:|---:|---:|
| `session_manager.mu.in` | 144 | 138 | 6 |
| `Composite_edit_mode.mu` | 11 | 11 | 0 |
| `FolderGroup_edit_mode.mu` | 9 | 9 | 0 |
| `LayoutGroup_edit_mode.mu` | 33 | 33 | 0 |
| `RetimeGroup_edit_mode.mu` | 20 | 20 | 0 |
| `SequenceGroup_edit_mode.mu` | 19 | 19 | 0 |
| `SourceGroup_edit_mode.mu` | 23 | 23 | 0 |
| `StackGroup_edit_mode.mu` | 7 | 7 | 0 |
| `Stack_edit_mode.mu` | 21 | 21 | 0 |
| `SwitchGroup_edit_mode.mu` | 5 | 5 | 0 |
| `Switch_edit_mode.mu` | 18 | 18 | 0 |
| `transform_manip.mu` | 25 | 25 | 0 |
| **TOTAL** | **335** | **329** | **6** |


<details><summary><code>session_manager.mu.in</code> — 144 symbols, 58 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `EventFilter` | `unit/test_mode_events.py::TestEventFilter` | ✅ |
| `FilmstripWidget` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `InputsView` | `unit/test_tree_view.py::TestInputsView` | ✅ |
| `NodeModel` | `unit/test_node_model.py` | ✅ |
| `NodeTreeView` | `unit/test_tree_view.py` | ✅ |
| `SessionManagerMode` | *constructing the mode segfaults headlessly — pinned by gates 0–4* — `unit/test_mode_slots.py::TestConstructionIsNotUnitTestable` | ➖ |
| `SourcePreviewWidget` | `unit/test_preview_widgets.py::TestSourcePreviewWidget` | ✅ |
| `ThumbnailWidget` | `unit/test_preview_widgets.py::TestThumbnailWidget` | ✅ |
| `activate` | `unit/test_mode_events.py::TestActivateDeactivate` | ✅ |
| `addEditor` | `unit/test_mode_slots.py::TestEditorTabs` | ✅ |
| `addInput` | `unit/test_node_ops.py::TestAddInput` | ✅ |
| `addMovieProc` | `unit/test_mode_dialogs.py::TestCreateImageDialog` | ✅ |
| `addNodeByTypeName` | `unit/test_mode_dialogs.py::TestNewNodeByTypeDialog` | ✅ |
| `addNodeOfType` | `unit/test_mode_interactions.py::TestNewNodeByTypeDialog` | ✅ |
| `addRow` | `unit/test_helpers.py::TestAddRow` | ✅ |
| `addThingSlot` | `unit/test_mode_interactions.py::TestNewNodeByTypeDialog` | ✅ |
| `afterGraphViewChange` | `unit/test_mode_events.py::TestGraphViewChange` | ✅ |
| `afterProgressiveLoading` | `unit/test_mode_events.py::TestProgressiveLoading` | ✅ |
| `assignSortOrder` | `unit/test_state_props.py::TestAssignSortOrder` | ✅ |
| `auxFilePath` | `unit/test_mode.py::TestAuxFilePath` | ✅ |
| `auxIcon` | `unit/test_mode_tree_build.py::TestIcons` | ✅ |
| `beforeGraphViewChange` | `unit/test_mode_events.py::TestGraphViewChange` | ✅ |
| `beforeProgressiveLoading` | `unit/test_mode_events.py::TestProgressiveLoading` | ✅ |
| `chooseColorSlot` | `unit/test_mode_slots.py::TestColorSlots` | ✅ |
| `colorAdjustedIcon` | `unit/test_mode_tree_build.py::TestIcons` | ✅ |
| `componentAndFolderNodeFromHash` | `unit/test_mode_tree_build.py::TestSourceFromSubComponent` | ✅ |
| `componentMatch` | `unit/test_helpers.py::TestComponentMatch` | ✅ |
| `configSlot` | `unit/test_mode.py::TestConfigSlot` | ✅ |
| `contains` | *inlined as Python `in`* — `unit/test_state_props.py::TestSubComponentExpanded` | ➖ |
| `createMode` | *constructing the mode segfaults headlessly — pinned by gates 0–4* — `unit/test_mode_slots.py::TestConstructionIsNotUnitTestable` | ➖ |
| `deactivate` | `unit/test_mode_events.py::TestActivateDeactivate` | ✅ |
| `deleteViewableSlot` | `unit/test_mode_slots.py::TestDeleteViewableSlot` | ✅ |
| `dragEnterEvent` | `unit/test_tree_view.py::TestDragEnterEvent` | ✅ |
| `dragMoveEvent` | `unit/test_tree_view.py::TestDragMoveEvent` | ✅ |
| `dropEvent` | `unit/test_mode_dialogs.py::TestDropEvent` | ✅ |
| `editViewInfoSlot` | `unit/test_mode_interactions.py::TestEditViewInfoSlot` | ✅ |
| `enterQuittingState` | `unit/test_mode_events.py::TestQuittingAndCategory` | ✅ |
| `event` | `unit/test_preview_widgets.py::TestSourcePreviewWidget` | ✅ |
| `eventFilter` | `unit/test_mode_events.py::TestEventFilter` | ✅ |
| `filteredDraggedPaths` | `unit/test_tree_view.py::TestFilteredDraggedPaths` | ✅ |
| `hasInput` | `unit/test_node_ops.py::TestHasInput` | ✅ |
| `hashedSubComponent` | `unit/test_hashed_subcomponent.py` | ✅ |
| `iconForNode` | `unit/test_mode.py::TestIconForNode` | ✅ |
| `includes` | `unit/test_helpers.py::TestIncludes` | ✅ |
| `indexOf` | *inlined as `list.index()`* — `unit/test_state_props.py::TestAssignSortOrder` | ➖ |
| `indexOfItem` | *dead in Mu — defined, never called; not ported* | ➖ |
| `inputRowsInsertedSlot` | `unit/test_mode_events.py::TestInputRowSlots` | ✅ |
| `inputRowsRemovedSlot` | `unit/test_mode_events.py::TestInputRowSlots` | ✅ |
| `inputsDeleteSlot` | `unit/test_mode_panel.py::TestInputsDeleteSlot` | ✅ |
| `isExpandedInParent` | `unit/test_state_props.py::TestExpandedInParent` | ✅ |
| `isImageRequestPropEqual` | `unit/test_image_request.py::TestIsImageRequestPropEqual` | ✅ |
| `isLoaded` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `isSubComponentExpanded` | `unit/test_state_props.py::TestSubComponentExpanded` | ✅ |
| `itemIsSubComponent` | `unit/test_helpers.py::TestSubComponentType` | ✅ |
| `itemNode` | `unit/test_helpers.py::TestItemNode` | ✅ |
| `itemOfNode` | `unit/test_helpers.py::TestMapItems` | ✅ |
| `itemParentNode` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemPressed` | `unit/test_mode_slots.py::TestItemPressed` | ✅ |
| `itemSubComponentHash` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemSubComponentMedia` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemSubComponentStringData` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `itemSubComponentType` | `unit/test_helpers.py::TestSubComponentType` | ✅ |
| `itemSubComponentTypeForName` | `unit/test_helpers.py::TestSubComponentTypeForName` | ✅ |
| `itemSubComponentValue` | `unit/test_helpers.py::TestSubComponentAccessors` | ✅ |
| `load` | `unit/test_preview_widgets.py` | ✅ |
| `loadStrip` | `unit/test_preview_widgets.py::TestSourcePreviewWidget` | ✅ |
| `loadThumbnail` | `unit/test_preview_widgets.py::TestSourcePreviewWidget` | ✅ |
| `mainWinVisTimeout` | `unit/test_mode_events.py::TestVisibility` | ✅ |
| `makeImage` | `unit/test_mode_dialogs.py::TestCreateImageDialog` | ✅ |
| `makeNewNodeOfType` | `unit/test_mode_dialogs.py::TestNewNodeByTypeDialog` | ✅ |
| `makeSourceRowWidget` | `unit/test_mode_tree_build.py::TestMakeSourceRowWidget` | ✅ |
| `map` | `unit/test_helpers.py::TestMapItems` | ✅ |
| `mapOverItem` | `unit/test_mode_tree_build.py::TestMapOverItem` | ✅ |
| `mimeData` | `unit/test_node_model.py::TestMimeData` | ✅ |
| `mimeTypes` | `unit/test_node_model.py::TestMimeTypes` | ✅ |
| `mouseMoveEvent` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `navButtonClicked` | `unit/test_mode.py::TestNavButtonClicked` | ✅ |
| `newColorSlot` | `unit/test_mode_slots.py::TestColorSlots` | ✅ |
| `newFolderSlot` | `unit/test_mode_dialogs.py::TestNewFolderSlot` | ✅ |
| `newNodeRow` | `unit/test_mode_tree_build.py::TestNewNodeRow` | ✅ |
| `newNodeStatusColumns` | `unit/test_mode_tree_build.py::TestNewNodeRow` | ✅ |
| `newNodeSubComponent` | `unit/test_mode_tree_build.py::TestSubComponentRows` | ✅ |
| `newSubComponentNode` | `unit/test_mode_tree_build.py::TestSourceFromSubComponent` | ✅ |
| `nodeFromIndex` | `unit/test_helpers.py::TestNodeFromIndex` | ✅ |
| `nodeInputs` | `unit/test_helpers.py::TestNodeInputs` | ✅ |
| `nodeInputsChanged` | `unit/test_mode_events.py::TestNodeInputsChanged` | ✅ |
| `onCategoryStateChanged` | `unit/test_mode_events.py::TestQuittingAndCategory` | ✅ |
| `printRows` | `unit/test_mode_slots.py::TestPrintRows` | ✅ |
| `propertyChanged` | `unit/test_mode_events.py::TestPropertyChanged` | ✅ |
| `rebuildInputsFromList` | `unit/test_mode_panel.py::TestRebuildInputsFromList` | ✅ |
| `reloadEditorTab` | `unit/test_mode_slots.py::TestEditorTabs` | ✅ |
| `remove` | *inlined as a list comprehension* — `unit/test_state_props.py::TestSubComponentExpanded` | ➖ |
| `removeInput` | `unit/test_node_ops.py::TestRemoveInput` | ✅ |
| `renameByType` | `unit/test_rename.py::TestRenameByType` | ✅ |
| `reorderSelected` | `unit/test_mode_slots.py::TestReorderSelected` | ✅ |
| `resizeColumns` | `unit/test_helpers.py::TestResizeColumns` | ✅ |
| `restoreTabState` | `unit/test_mode.py::TestTabState` | ✅ |
| `saveTabState` | `unit/test_mode.py::TestTabState` | ✅ |
| `selectCurrentViewSlot` | `unit/test_mode_slots.py::TestViewSelectionChanged` | ✅ |
| `selectInputsRange` | `unit/test_mode_slots.py::TestSelectInputsRange` | ✅ |
| `selectViewableNode` | `unit/test_mode_panel.py::TestSelectViewableNode` | ✅ |
| `selectedConvertedSubComponents` | `unit/test_mode_slots.py::TestSelectionReaders` | ✅ |
| `selectedItems` | `unit/test_mode_slots.py::TestSelectionReaders` | ✅ |
| `selectedNodePaths` | `unit/test_tree_view.py::TestSelectedNodePaths` | ✅ |
| `selectedNodes` | `unit/test_sort_inputs.py (via selectedNodesEvent)` | ✅ |
| `selectedNodesEvent` | `unit/test_cross_package_api.py::TestSelectedNodesEvent` | ✅ |
| `setExpandedInParent` | `unit/test_state_props.py::TestExpandedInParent` | ✅ |
| `setFallback` | `unit/test_preview_widgets.py::TestThumbnailWidget` | ✅ |
| `setImageRequest` | `unit/test_image_request.py::TestSetImageRequestToggle` | ✅ |
| `setImageRequestProp` | `unit/test_image_request.py::TestSetImageRequestProp` | ✅ |
| `setInputs` | `unit/test_node_ops.py::TestSetInputs` | ✅ |
| `setItemExpandedState` | `unit/test_mode_panel.py::TestSetItemExpandedState` | ✅ |
| `setNodeRequest` | `unit/test_image_request.py::TestSetNodeRequest` | ✅ |
| `setNodeStatus` | `unit/test_mode.py::TestSetNodeStatus` | ✅ |
| `setSortKeyInParent` | `unit/test_state_props.py::TestSortKey` | ✅ |
| `setSubComponentExpanded` | `unit/test_state_props.py::TestSubComponentExpanded` | ✅ |
| `setToolTipProp` | `unit/test_state_props.py::TestToolTipProp` | ✅ |
| `showFrameAtX` | `unit/test_preview_widgets.py::TestFilmstripWidget` | ✅ |
| `showRows` | `unit/test_mode_slots.py::TestPrintRows` | ✅ |
| `sortFolderChildren` | `unit/test_tree_view.py::TestSortFolderChildren` | ✅ |
| `sortFolders` | `unit/test_tree_view.py::TestSortFolders` | ✅ |
| `sortInputs` | `unit/test_sort_inputs.py` | ✅ |
| `sortKeyInParent` | `unit/test_state_props.py::TestSortKey` | ✅ |
| `sourceFromSubComponent` | `unit/test_mode_tree_build.py::TestSourceFromSubComponent` | ✅ |
| `sourceNodeOfGroup` | `unit/test_helpers.py::TestSourceNodeOfGroup` | ✅ |
| `splitterMoved` | `unit/test_mode.py::TestSplitterMoved` | ✅ |
| `subComponentItemsOfNode` | `unit/test_helpers.py::TestSubComponentItemsOfNode` | ✅ |
| `subComponentPropValue` | `unit/test_subcomponent_prop.py` | ✅ |
| `tabChangeSlot` | `unit/test_mode.py::TestTabState` | ✅ |
| `theMode` | `unit/test_cross_package_api.py::TestSelectedNodeLines` | ✅ |
| `togglePreviews` | `unit/test_mode.py::TestTogglePreviews` | ✅ |
| `toolTipFromProp` | `unit/test_state_props.py::TestToolTipProp` | ✅ |
| `updateInputs` | `unit/test_mode_panel.py::TestUpdateInputs` | ✅ |
| `updateNavUI` | `unit/test_mode_panel.py::TestUpdateNavUI` | ✅ |
| `updateNodePreviewEvent` | `unit/test_mode_tree_build.py::TestUpdateNodePreviewEvent` | ✅ |
| `updateTree` | `unit/test_mode_tree_build.py::TestUpdateTree` | ✅ |
| `updateTreeEvent` | `unit/test_mode_events.py::TestProgressiveLoading` | ✅ |
| `useEditor` | `unit/test_mode_slots.py::TestEditorTabs` | ✅ |
| `viewByIndex` | `unit/test_mode_interactions.py::TestViewByIndex` | ✅ |
| `viewContextMenuSlot` | `unit/test_mode_interactions.py::TestContextMenuConstruction` | ✅ |
| `viewEditModeActivated` | `unit/test_mode_events.py::TestGraphViewChange` | ✅ |
| `viewItemChanged` | `unit/test_mode_dialogs.py::TestViewItemChanged` | ✅ |
| `viewSelectionChanged` | `unit/test_mode_slots.py::TestViewSelectionChanged` | ✅ |
| `visibilityChanged` | `unit/test_mode_events.py::TestVisibility` | ✅ |

</details>

<details><summary><code>Composite_edit_mode.mu</code> — 11 symbols, 5 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `CompositeEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestCompositeLoadUI` | ✅ |
| `opState` | `unit/test_edit_mode_menus.py::TestCompositeMenu` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_menus.py::TestCompositeMenu` | ✅ |
| `setDissolveAmount` | `unit/test_composite_edit_mode.py::TestDissolveAmount` | ✅ |
| `setDissolveAmountFromSlider` | `unit/test_composite_edit_mode.py::TestDissolveAmount` | ✅ |
| `setOp` | `unit/test_composite_edit_mode.py::TestSetOp` | ✅ |
| `setOpEvent` | `unit/test_composite_edit_mode.py::TestSetOp` | ✅ |
| `updateUI` | `unit/test_composite_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>FolderGroup_edit_mode.mu</code> — 9 symbols, 2 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `FolderGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_edit_mode_slots.py::TestFolderActivateUI` | ✅ |
| `activateUI` | `unit/test_edit_mode_slots.py::TestFolderActivateUI` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `deactivate` | `unit/test_edit_mode_slots.py::TestFolderActivateUI` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestFolderLoadUI` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_slots.py::TestFolderPropertyChanged` | ✅ |
| `setViewType` | `unit/test_folder_group_edit_mode.py::TestSetViewType` | ✅ |
| `updateUI` | `unit/test_folder_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>LayoutGroup_edit_mode.mu</code> — 33 symbols, 13 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `LayoutGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_edit_mode_slots.py::TestLayoutActivation` | ✅ |
| `activateTransformMode` | `unit/test_edit_mode_slots.py::TestLayoutActivation` | ✅ |
| `activateUI` | `unit/test_edit_mode_slots.py::TestLayoutActivation` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `deactivate` | `unit/test_edit_mode_slots.py::TestLayoutActivation` | ✅ |
| `gridColumnsChangedSlot` | `unit/test_edit_mode_slots.py::TestLayoutSlots` | ✅ |
| `gridRowsChangedSlot` | `unit/test_edit_mode_slots.py::TestLayoutSlots` | ✅ |
| `isLayoutMode` | `unit/test_layout_group_edit_mode.py::TestIsLayoutMode` | ✅ |
| `layoutInColumn` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutInColumnEvent` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `layoutInGrid` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutInGridEvent` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `layoutInRow` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutInRowEvent` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `layoutManually` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutManuallyEvent` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `layoutMode` | `unit/test_layout_group_edit_mode.py::TestLayoutMode` | ✅ |
| `layoutPacked` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutPacked2` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutPacked2Event` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `layoutPackedEvent` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `layoutStatic` | `unit/test_layout_group_edit_mode.py::TestLayoutSelectors` | ✅ |
| `layoutStaticEvent` | `unit/test_edit_mode_slots.py::TestLayoutMenuEvents` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestLayoutLoadUI` | ✅ |
| `modeComboChangedSlot` | `unit/test_edit_mode_slots.py::TestLayoutSlots` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_slots.py::TestLayoutPropertyChanged` | ✅ |
| `setGridRowsColumns` | `unit/test_layout_group_edit_mode.py::TestSpacingAndGrid` | ✅ |
| `setLayoutMode` | `unit/test_layout_group_edit_mode.py::TestLayoutMode` | ✅ |
| `setSpacing` | `unit/test_layout_group_edit_mode.py::TestSpacingAndGrid` | ✅ |
| `spacingSliderChangedSlot` | `unit/test_edit_mode_slots.py::TestLayoutSlots` | ✅ |
| `updateUI` | `unit/test_layout_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>RetimeGroup_edit_mode.mu</code> — 20 symbols, 5 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `RetimeGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `convertToFPS` | `unit/test_edit_mode_slots.py::TestRetimeConvertToFPS` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `editSlot` | `unit/test_edit_mode_slots.py::TestRetimeSlots` | ✅ |
| `factorPrompt` | `unit/test_edit_mode_slots.py::TestRetimePrompts` | ✅ |
| `fpsPrompt` | `unit/test_edit_mode_slots.py::TestRetimePrompts` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestRetimeLoadUI` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_slots.py::TestRetimePropertyChanged` | ✅ |
| `reset` | `unit/test_retime_group_edit_mode.py::TestReset` | ✅ |
| `resetSlot` | `unit/test_edit_mode_slots.py::TestRetimeSlots` | ✅ |
| `resetTiming` | `unit/test_edit_mode_slots.py::TestRetimeSlots` | ✅ |
| `reverse` | `unit/test_retime_group_edit_mode.py::TestReverse` | ✅ |
| `reverseSlot` | `unit/test_edit_mode_slots.py::TestRetimeSlots` | ✅ |
| `reverseTiming` | `unit/test_edit_mode_slots.py::TestRetimeSlots` | ✅ |
| `setConvertFPS` | `unit/test_retime_group_edit_mode.py::TestSetConvertFPS` | ✅ |
| `setFactorValue` | `unit/test_retime_group_edit_mode.py::TestSetFactorValue` | ✅ |
| `slowDownPrompt` | `unit/test_edit_mode_slots.py::TestRetimePrompts` | ✅ |
| `speedUpPrompt` | `unit/test_edit_mode_slots.py::TestRetimePrompts` | ✅ |
| `updateUI` | `unit/test_retime_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>SequenceGroup_edit_mode.mu</code> — 19 symbols, 7 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SequenceGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `activateUI` | `unit/test_edit_mode_ui_loading.py::TestSequenceLoadUI` | ✅ |
| `afterSessionRead` | `unit/test_sequence_group_edit_mode.py::TestSessionReadFreeze` | ✅ |
| `autoEDL` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `beforeSessionRead` | `unit/test_sequence_group_edit_mode.py::TestSessionReadFreeze` | ✅ |
| `checkBoxSlot` | `unit/test_sequence_group_edit_mode.py::TestCheckBoxSlot` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `fpsChanged` | `unit/test_sequence_group_edit_mode.py::TestFpsChanged` | ✅ |
| `heightChanged` | `unit/test_sequence_group_edit_mode.py::TestSizeEdits` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestSequenceLoadUI` | ✅ |
| `menu` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `stateFunc` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `updateUI` | `unit/test_sequence_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |
| `updateUIEvent` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `useCutInfo` | `unit/test_edit_mode_menus.py::TestSequenceMenu` | ✅ |
| `widthChanged` | `unit/test_sequence_group_edit_mode.py::TestSizeEdits` | ✅ |

</details>

<details><summary><code>SourceGroup_edit_mode.mu</code> — 23 symbols, 9 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SourceGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_edit_mode_slots.py::TestSourceUpdateFromProps` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `changedSlot` | `unit/test_edit_mode_slots.py::TestSourceChangedSlot` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `cutInPrompt` | `unit/test_source_group_edit_mode.py::TestPrompts` | ✅ |
| `cutOutPrompt` | `unit/test_source_group_edit_mode.py::TestPrompts` | ✅ |
| `finishedSlot` | `unit/test_edit_mode_slots.py::TestSourceFinishedSlot` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestSourceLoadUI` | ✅ |
| `newInPoint` | `unit/test_source_group_edit_mode.py::TestNewInOutPoint` | ✅ |
| `newOutPoint` | `unit/test_source_group_edit_mode.py::TestNewInOutPoint` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_slots.py::TestSourceResetAndPropertyChanged` | ✅ |
| `reset` | `unit/test_source_group_edit_mode.py::TestReset` | ✅ |
| `resetCut` | `unit/test_source_group_edit_mode.py::TestReset` | ✅ |
| `resetSlot` | `unit/test_edit_mode_slots.py::TestSourceResetAndPropertyChanged` | ✅ |
| `setCutValue` | `unit/test_source_group_edit_mode.py::TestSetCutValue` | ✅ |
| `sourceMenuState` | `unit/test_edit_mode_slots.py::TestSourceSyncGuiInOut` | ✅ |
| `syncGuiInOut` | `unit/test_edit_mode_slots.py::TestSourceSyncGuiInOut` | ✅ |
| `syncSlot` | `unit/test_source_group_edit_mode.py::TestSyncSlot` | ✅ |
| `syncState` | `unit/test_edit_mode_slots.py::TestSourceSyncGuiInOut` | ✅ |
| `toggleSync` | `unit/test_edit_mode_slots.py::TestSourceToggleSync` | ✅ |
| `updateFromProps` | `unit/test_edit_mode_slots.py::TestSourceUpdateFromProps` | ✅ |
| `updateUI` | `unit/test_source_group_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |

</details>

<details><summary><code>StackGroup_edit_mode.mu</code> — 7 symbols, 0 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `StackGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |
| `activateUI` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `deactivate` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |
| `propertyChanged` | `unit/test_group_edit_modes.py::TestStackGroupEditMode` | ✅ |

</details>

<details><summary><code>Stack_edit_mode.mu</code> — 21 symbols, 6 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `StackEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `alignStartFrames` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `autoRetimeInputs` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `checkBoxSlot` | `unit/test_stack_edit_mode.py::TestCheckBoxSlot` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `fpsChanged` | `unit/test_stack_edit_mode.py::TestFpsChanged` | ✅ |
| `heightChanged` | `unit/test_stack_edit_mode.py::TestSizeEdits` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestStackLoadUI` | ✅ |
| `menu` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `retimeState` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `setChosenAudioInput` | `unit/test_stack_edit_mode.py::TestSetChosenAudioInput` | ✅ |
| `stateFunc` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `strictFrameRanges` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `updateMenu` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `updateUI` | `unit/test_stack_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |
| `updateUIEvent` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `useCutInfo` | `unit/test_edit_mode_menus.py::TestStackMenu` | ✅ |
| `widthChanged` | `unit/test_stack_edit_mode.py::TestSizeEdits` | ✅ |

</details>

<details><summary><code>SwitchGroup_edit_mode.mu</code> — 5 symbols, 0 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SwitchGroupEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_group_edit_modes.py::TestSwitchGroupEditMode` | ✅ |
| `activateUI` | `unit/test_group_edit_modes.py::TestSwitchGroupEditMode` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `deactivate` | `unit/test_group_edit_modes.py::TestSwitchGroupEditMode` | ✅ |

</details>

<details><summary><code>Switch_edit_mode.mu</code> — 18 symbols, 3 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `SwitchEditMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `alignStartFrames` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `auxFilePath` | `unit/test_edit_mode_factories.py::TestAuxFilePath` | ✅ |
| `checkBoxSlot` | `unit/test_switch_edit_mode.py::TestCheckBoxSlot` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `heightChanged` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `loadUI` | `unit/test_edit_mode_ui_loading.py::TestSwitchLoadUI` | ✅ |
| `menu` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `propertyChanged` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `retimeState` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `setSelectedInput` | `unit/test_switch_edit_mode.py::TestSetSelectedInput` | ✅ |
| `stateFunc` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `updateMenu` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `updateUI` | `unit/test_switch_edit_mode.py::TestUpdateUIWithoutPanel` | ✅ |
| `updateUIEvent` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `useCutInfo` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |
| `widthChanged` | `unit/test_edit_mode_menus.py::TestSwitchMenu` | ✅ |

</details>

<details><summary><code>transform_manip.mu</code> — 25 symbols, 6 tested</summary>

| Mu symbol | Python test | Status |
|---|---|---|
| `TransformManip` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `activate` | `unit/test_transform_manip_render.py::TestActivate` | ✅ |
| `activeImageIndex` | `unit/test_transform_manip_mode.py::TestActiveImageIndex` | ✅ |
| `afterGraphViewChange` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `beforeGraphViewChange` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `closestPointOnLine` | `unit/test_transform_manip.py::TestClosestPointOnLine` | ✅ |
| `computeGC` | `unit/test_transform_manip.py::TestComputeGC` | ✅ |
| `control` | `unit/test_transform_manip_mode.py::TestControlHitTest` | ✅ |
| `createMode` | `unit/test_edit_mode_factories.py::TestCreateMode` | ✅ |
| `deactivate` | `unit/test_transform_manip_pointer.py::TestDeactivate` | ✅ |
| `drag` | `unit/test_transform_manip_pointer.py::TestDragFreeTranslation` | ✅ |
| `drawCorners` | `unit/test_transform_manip_render.py::TestRenderCorners` | ✅ |
| `editNode` | `unit/test_transform_manip_mode.py::TestEditNode` | ✅ |
| `findEditingNodes` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `fitAll` | `unit/test_transform_manip_mode.py::TestFitAll` | ✅ |
| `move` | `unit/test_transform_manip_pointer.py::TestMove` | ✅ |
| `nodeAspect` | `unit/test_transform_manip_mode.py::TestNodeAspect` | ✅ |
| `nodeInputsChanged` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `push` | `unit/test_transform_manip_pointer.py::TestPush` | ✅ |
| `release` | `unit/test_transform_manip_pointer.py::TestRelease` | ✅ |
| `removeTags` | `unit/test_transform_manip_mode.py::TestTagLifecycle` | ✅ |
| `render` | `unit/test_transform_manip_render.py::TestRenderOutline` | ✅ |
| `resetAll` | `unit/test_transform_manip_mode.py::TestResetAll` | ✅ |
| `setManipState` | `unit/test_transform_manip_mode.py::TestSetManipState` | ✅ |
| `tagValue` | `unit/test_transform_manip.py::TestTagValue` | ✅ |

</details>


### The six ➖ rows

Four are Mu list helpers with no Python counterpart. `contains`, `indexOf` and
`remove` are one-line loops over a `string[]`; the port spells them `in`,
`list.index()` and a comprehension at each call site, and those call sites are
covered. `indexOfItem` is defined in Mu and never called, so it was not ported.

The other two are `SessionManagerMode` (the constructor) and `createMode`.
Constructing the mode parents a dock widget to the session window and then
**segfaults** under the offscreen platform — exit 139, reproducible — somewhere in
the dock/WebEngine path. Every other method on the mode is reachable by building the
instance with `object.__new__` and attaching the two or three real widgets it
touches, which is how the panel, event, slot, tree and dialog test modules work. The
constructor itself is left to the golden gates: gate 3 launches RV with the package
loaded and all 38 scenarios drive a constructed mode.

## Harness fixes this iteration

Four things were wrong with the capture/compare path, not with the port. All four had
been quietly weakening or destabilising the pixel gate; gate 2 went from 17/37 to
37/37 once they were fixed.

**Mouse hover leaked into every grab.** `QWidget.grab()` renders current widget state,
and hover is part of it: the tree row under the physical pointer painted highlighted
and a toolbar button under it painted raised. Nothing controlled where the pointer
was, so the same scenario put an extra highlight on `OTHER` in one run and on
`Default Stack` in the next. `qt_scenario_utils.clear_hover()` now runs before every
grab. It has to clear two separate mechanisms: an item view keeps a private hover
*index* (cleared by `HoverLeave`), while a button reads `WA_UnderMouse`. Clearing only
the first left a 2/255 difference on one pixel of the inputs panel's trash button.

**`findChildren()` is not safe under a `wrapInstance` root.** The first version of
`clear_hover` enumerated the panel's children that way and left the caller's
`QTreeView` reference dead — "Internal C++ object already deleted" in six scenarios.
Reducing the function to the bare enumeration still killed them, so it is the
enumeration itself: minting a second set of wrappers inside that tree invalidates the
first. `QApplication.allWidgets()` is what the `_sm_common` accessors already use and
does not have the problem. Worth knowing beyond this package.

**Alpha channel presence was unstable.** Qt's opacity heuristic gave an RGB pixmap one
run and RGBA the next, and `rmsImageDiff` rejects that outright with "channel size
does not match" instead of reporting a pixel difference — so the gate could not say
what had changed. Grabs now normalise to `QImage.Format_RGB888`.

**`rmsImageDiff -cmp` exits 0 on a mismatch.** It only returns non-zero when it cannot
compare the files at all. `compare.py` was already correct (it parses the
"Images are matched." verdict from stdout), but an ad-hoc `rmsImageDiff ... && echo
same` reports every mismatch as a match, and that is how a reproducible gate failure
was briefly misread as flakiness. The comment in `compare.py` now states the trap
explicitly.

**Baselines are rendering-state sensitive.** During one full re-capture the machine's
text rasterisation changed partway through, leaving the first 18 baselines in one
state and the rest in another; the 18 then failed gate 2 reproducibly. Re-capturing
them fixed it. The lesson for anyone re-capturing: capture the whole set in one
sitting and re-run gate 2 immediately, and treat a failing set that is a contiguous
alphabetical prefix as a capture artefact rather than a port difference.

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
