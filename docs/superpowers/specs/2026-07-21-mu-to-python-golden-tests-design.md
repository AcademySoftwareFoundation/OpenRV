# Mu → Python Migration via Golden Tests — Design

**Date:** 2026-07-21
**Status:** Approved design; pilot on `session_manager`
**Author:** brainstormed with Claude Code

## Goal

Migrate OpenRV's large, aging Mu codebase (~57K lines across ~194 source files)
to Python in **one migration loop per package**, using AI-driven edit/verify
iterations gated by automated **golden tests**. The loop ports the whole package;
you may tackle areas in any order within the loop, but Mu sources stay until
everything in `COVERAGE.md` passes. Python is already a first-class peer
extension language in RV (full `rv.rvtypes` parity, all 368 `commands` exposed
1:1, 20+ shipped Python packages), so this paves an existing road rather than
inventing a new one.

The first target is the **`session_manager`** package
(`src/plugins/rv-packages/session_manager/`, `session_manager.mu` ≈ 3,515 lines).

## Verification strategy

The migration is safe only if we can prove the Python port behaves identically
to the Mu original. We verify against the **real `rv` application run headless**,
with two layers:

Both layers are **hard gates** — a scenario passes only if **both** pass (JSON
`AND` pixels). They catch disjoint regression classes, so neither subsumes the
other:

1. **Behavioral equivalence (hard gate).** Given the same scripted scenario,
   the Python version must produce the *same node-graph state* (nodes, types,
   connections, key properties) as the Mu original. Captured as canonical JSON
   and compared exactly. Catches graph/state errors that don't paint (wrong
   connection, wrong non-visual property).
2. **Pixel equivalence (hard gate).** The panel is grabbed to a PNG after a
   wait-for-stable step and compared to the Mu baseline via the existing
   `rmsImageDiff -cmp -dmax <threshold>` tool. Catches *visual* regressions the
   JSON cannot see — layout, stylesheet, icons, widget geometry — which for a UI
   panel is the whole point of the port.

Rationale (revised): `session_manager` is a **UI panel**; its output *is* pixels.
A Python port can reproduce the node graph exactly and still render a broken
panel, and the JSON gate would pass. So pixels must gate too — closing the hole
where the AI loop satisfies JSON while breaking rendering.

The original concern (pixels are a flaky oracle) is real but is a *capture*
problem, not a reason to demote the gate. For an AI edit/verify loop a **flaky**
gate is the actual hazard: intermittent failures train the loop to mutate code
until the dice land right (hacking the oracle). The fix is to make capture
**bit-deterministic** and gate at `-dmax 0` (or near it) — a failure then means
the rendering genuinely changed, which is exactly the signal we want. See
[Determinism requirements](#determinism-requirements-for-the-pixel-gate).

> **Tool constraint:** `rmsImageDiff` (`src/bin/imgtools/rmsImageDiff/main.cpp`)
> compares the **whole image**, errors if sizes/channels differ, and has **no
> masking / crop / region-of-interest**. Flags: `-f` (float), `-m` (report max
> error), `-cmp` (exit 1 on mismatch), `-dmax <v>` (per-channel error ceiling;
> `0` = exact). Because there is no ROI, nondeterminism (e.g. async thumbnails)
> must be removed **at capture time** or cropped out of the PNG *before* diffing
> — it cannot be masked at diff time.

### Why headless — and why NOT `QT_QPA_PLATFORM=offscreen`

> **Empirically verified 2026-07-21 (RTX A2000, driver 595.80, Rocky 9.8),
> against the built `_build/stage/app/bin/rv`.**
>
> `QT_QPA_PLATFORM=offscreen` **segfaults RV on startup** (exit 139). RV
> constructs an OpenGL viewport (`GLView`/`QOpenGLWidget`) at launch, and the
> offscreen platform provides no GLX context for it. The original premise of
> this doc — "offscreen still renders, so we grab from it" — is **false for RV**.
>
> The working headless path is a **virtual X server + software Mesa**:
> `xvfb-run -a -s "-screen 0 1280x1024x24" ./rv ...`. Xvfb gives RV the GLX
> context it needs; rendering goes through software Mesa (llvmpipe/swrast). RV
> starts, runs, and widget capture works (one non-fatal `NV-GLX Extension
> Missing` warning). Both `xvfb-run` and `mesa-libGL` are already installed on
> the Rocky runners.

Headless is required because (a) pixel comparison needs a locked, reproducible
environment free of window-manager decorations, compositor, DPI scaling, and
stray focus/hover state, and (b) the Rocky Linux CI runners have no physical
display. Software Mesa under xvfb is **deterministic given a pinned Mesa
version**, which is what makes the pixel gate safe — see below.

**Proof of the full pipeline (all verified end-to-end):** launch under xvfb →
open the panel via `sendInternalEvent("key-down--x")` → locate the
`sessionManager` widget by objectName → wait-for-stable (~300ms) → `grab()` to
PNG (319×577). Two independent runs produced **pixel-identical** PNGs, and
`rmsImageDiff -cmp -dmax 0` reported "Images are matched" (exit 0), while a
different image was correctly rejected. The `-dmax 0` gate is therefore real.

### Determinism requirements for the pixel gate

A hard pixel gate is only safe if the capture is bit-reproducible. Sources of
nondeterminism and their fixes:

| Source | Fix |
|---|---|
| GPU/driver variance | Rendered through **software Mesa under xvfb**, not the GPU — deterministic across machines given a pinned Mesa version. (Verified: two runs pixel-identical.) |
| Async thumbnails | Media-free scenarios (e.g. read-only tree) use **fixtures with no media**, so no thumbnails exist. Media-bearing scenarios: block `wait-for-stable` until all thumbnail jobs complete, **or** crop the thumbnail column out of the PNG before diffing (external crop → equal-size images → `rmsImageDiff`). |
| Fonts / hinting | Pin a bundled font + fixed `fontconfig` in the CI container; set a fixed `QT_FONT_DPI`. |
| HiDPI scaling | `QT_SCALE_FACTOR=1`, `QT_ENABLE_HIGHDPI_SCALING=0`, fixed widget size. |
| Animations / hover | Disable animations; scenarios drive via `rv.commands`, so no stray focus/hover. |
| Xvfb / Mesa drift | Pin the xvfb screen geometry and Mesa version in the container; **capture goldens in the same container/path** used to test. |

Gate at `-dmax 0` (exact) on the pinned xvfb + software-Mesa path; loosen `dmax`
only if residual noise is *observed*, and always log `-m` (max error) so drift
surfaces instead of being silently absorbed. Never loosen `dmax` to paper over
flakiness — that reintroduces the hack-the-oracle risk.

### OpenGL viewport (future)

If future work needs the OpenGL viewport, GPU rendering is **not**
bit-reproducible across cards/drivers, so gated goldens for that work must be
rendered through **OSMesa software** (`rvio_sw`), keeping them deterministic. A
GPU-bound headless X server (`src/test/golden/harness/headless_x.sh`) is reserved
for interactive smoke tests, **not** gated goldens. Plain xvfb also falls back to
software Mesa and does not exercise the GPU.

## Harness architecture

One-time reusable infrastructure (the repo needs it anyway — `rvio` and
`rmsImageDiff` exist but are currently unwired into any test).

```
src/test/golden/
  harness/                    # REUSABLE across all future packages
    run_scenario.py           #   launch headless rv, load fixture, run scenario, dump outputs
    capture.py                #   node-graph -> canonical JSON; panel widget -> PNG (deterministic, wait-for-stable)
    compare.py                #   exact JSON diff; rmsImageDiff wrapper for pixels (both hard gates)
  session_manager/
    scenarios/                # command-API scenario scripts (one behavior each)
    golden/                   # baseline JSON + reference PNGs captured from the Mu version
    CMakeLists.txt            # registers one CTest case per scenario
  CMakeLists.txt              # shared harness + ADD_SUBDIRECTORY per package
```

- **App driver** launches real `rv` **under xvfb + software Mesa**
  (`xvfb-run -a -s "-screen 0 1280x1024x24" rv ...` — *not* `QT_QPA_PLATFORM=offscreen`,
  which segfaults RV), loads a fixed fixture session, and executes a **scenario
  script** (via `-pyeval` / `sendInternalEvent`).
- **Behavioral capture** — **reuse existing serialization, do not build from
  scratch.** RV already exposes `saveSession(path, asCopy=true, compressed=false,
  sparse=false)` (via `rv.commands`), which writes a **text GTO** of the whole
  graph (nodes, types, connections, all persistent properties). RV already sorts
  nodes by name for stability and the text header has no timestamp. `capture.py`
  is therefore a thin `saveSession` wrapper. Fallback for finer canonical control:
  a commands walk (`nodes()`/`nodeType`/`properties()`/typed getters/
  `nodeConnections`/`nodesInGroup`).
- **Pixel capture** grabs the panel widget to PNG *after* a wait-for-stable step,
  under the pinned deterministic environment (see [Determinism
  requirements](#determinism-requirements-for-the-pixel-gate)). The PNG must be
  byte-reproducible on the pinned path so the gate can run at `-dmax 0`.
- **Comparators** — no session/graph diff exists today, so `compare.py` is a new
  but thin **normalize-then-diff** layer for behavior, plus a `rmsImageDiff -cmp
  -dmax` wrapper for pixels. **Both are hard gates**; a scenario passes only if
  both pass. Behavioral normalization must strip these non-deterministic bits
  from a raw session dump before diffing:
  - absolute media paths (rewrite to relative/placeholder)
  - force `sparse=false` (or `RV_COMPLETE_SESSION_FILES=1`) for a stable full dump
  - drop volatile header fields: `session.currentFrame`, `session.marks`,
    `session.viewNode`, range/region/fps
  - auto-generated node names (`sourceGroup000000`) if scenario build order varies
  - sort properties within a node if a fully canonical form is wanted
  (`gtoinfo --all` can render binary GTO to text if we ever capture binary.)
- **CTest wiring**: registered so the same checks run locally (`rvtest`) and,
  once proven, in GitHub CI. **Plan: local first, CI later.**

### Scenario driving: `rv.commands` API directly

Scenarios trigger panel logic by calling the same commands the package reacts to
(e.g. `newNode`, `setViewNode`, `sendInternalEvent`) rather than synthesizing Qt
mouse/keyboard events. This is deterministic and headless-friendly. Synthetic Qt
input events (`QTest`) are deferred until drag-and-drop areas that genuinely
require simulated drags.

## Mu/Python coexistence during migration

A mechanism to load *either* the Mu or the new Python `session_manager`
(env var or `PACKAGE` swap), so the harness can capture goldens from the Mu
version and later run the same scenarios against the Python version, without
deleting the original mid-migration.

## Suggested implementation order (`session_manager`)

Within a single migration loop, lower-risk areas are easier to land first:

1. **Read-only tree population** (pilot) — build the `QStandardItemModel` tree
   from the node graph. Maps most directly to Python; no drag-drop, no custom
   subclasses, no async thumbnails.
2. Menus, buttons, signal/slot wiring, `.ui` loading.
3. The per-view edit modes (`*_edit_mode.mu` — already separate files).
4. **Custom Qt subclasses (highest risk, last):** ~8 classes overriding virtuals —
   custom `mimeData`, drag-and-drop `dropEvent`s, an installed `eventFilter`.
   Doable in PySide6/shiboken but the most involved area, and the point where
   synthetic Qt input events become necessary.

The `local_thumbnail_gen.py` piece is already Python and shared.

## AI loop mechanics

One loop per package:

1. Agent receives the Mu sources, a Python skeleton (`MinorMode` + `.ui` load),
   and the harness command.
2. Agent edits Python → runs harness → reads the diff → iterates until behavioral
   and pixel gates pass for the whole package.
3. Human reviews the port and the diffs; commit when `COVERAGE.md` is fully green.

**Guardrails:** capped iterations; the loop never edits the golden baselines;
stops when the full package passes; human gate before merge.

## Risks / open questions

- **Drag-and-drop synthesis headless** — the custom subclasses need simulated
  drag events; command-API driving can't reach them. Deferred to drag-and-drop
  work within the loop, using `QTest`.
- **Async thumbnail nondeterminism** — requires a robust wait-for-stable before
  screenshot; behavioral gate is unaffected.
- **Screenshot determinism across platforms** — pin the pixel gate to a single
  CI platform/toolchain; rely on RMS threshold, not exact match.
- **Qt5/Qt6 differences** — the Mu version uses build-time token substitution
  (`@MU_QT_*@`); the Python port handles this at runtime (try/except PySide2/6),
  as existing Python packages already do.

## Pilot: harness bring-up (historical)

1. Build the minimal harness (driver + capture + compare) — just enough to run
   one scenario end-to-end.
2. Set up Mu/Python coexistence (toggle which version loads).
3. Author command-API scenarios; capture golden JSON + PNG from the **Mu**
   version; commit baselines.
4. Run the migration loop; iterate to green for the full package.
5. Human review; tune wait-for-stable timing before widening to other packages.

Once the loop is green end-to-end for a package, repeat the same method for
other packages, then the broader Mu codebase.
