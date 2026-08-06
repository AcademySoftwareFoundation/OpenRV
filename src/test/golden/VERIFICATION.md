# Golden-Test Verification Method (shared across all Mu→Python migrations)

This is the **shared verification contract** for every package migrated from Mu to
Python via golden tests. Each package has its own inventory doc (e.g.
`session_manager/COVERAGE.md`) that plugs into the method defined here. Design rationale:
`docs/superpowers/specs/2026-07-21-mu-to-python-golden-tests-design.md`.

The migration uses baseline behaviour and appearance from the Mu implementation. Refactored code will be compared with the baseline tests.

```
setup:   [Mu package] --capture--> golden (committed)     ┐ actual == golden → PASS
         [Mu package] --run------> actual                 ┘ (proves determinism only)

loop:    [Mu package] --capture--> golden (already committed)
         [Python port]--run------> actual   actual == golden ?  ← the real migration gate
```

A **passing scenario before a port exists means only that the Mu capture is reproducible
(Mu == Mu)** — it does not verify any migration. The real test is when the Python port is
toggled in (see [Mu/Python implementation toggle](#mupython-implementation-toggle))
and the same scenarios run against it.

---

## Mu/Python implementation toggle

Both the Mu and Python sources for a package can live in the same build. RV normally loads **Mu first** when a `.mu` module exists; Python is only a fallback. For migration we need to run a **Python** mode against Mu-captured goldens without removing the Mu sources from the tree.

### Environment variables

`<modeName>` is the RV mode name from `PACKAGE` / `rvload2` (extension stripped),
e.g. `session_manager`, `Stack_edit_mode`.

| Variable | Example | Effect |
|---|---|---|
| `RV_MODE_IMPL_<modeName>` | `RV_MODE_IMPL_session_manager=python` | Per-mode: `python` loads `<modeName>.py`; `mu` forces the Mu implementation. |
| `RV_PREFER_PYTHON_MODES` | `RV_PREFER_PYTHON_MODES=session_manager,pyhello` | Comma-separated list of modes to load from Python. |

Precedence: per-mode `RV_MODE_IMPL_*` → `RV_PREFER_PYTHON_MODES` → **Python when the
package ships a `<modeName>.py` at all** → Mu.

**The default is Python-first** (`mode_manager.mu`, `loadEntry`). Mu wins only where no
Python implementation exists, which is the direction the plugin set is moving: Mu modes
are retired package by package, and a ported package keeps its `.mu` source purely so
gate 4 can still run against it. `RV_MODE_IMPL_<mode>=mu` is how gate 4 asks for it.

Whether a Python implementation exists is answered by `importlib.util.find_spec()`
rather than by attempting the import: the Mu binding for `PyImport_Import()` calls
`PyErr_Print()` on a missing module, and most modes are Mu-only, so probing by import
would print a traceback for each of them at every startup. The probe costs ~3.6 ms
across ~100 modes.

### Harness

```bash
# Mu (default): capture goldens, prove Mu determinism, re-baseline
python3 src/test/golden/harness/run_scenario.py \
    --scenario src/test/golden/session_manager/scenarios/tree_readonly.py \
    --out /tmp/tree_readonly --impl mu

# Python: migration loop / port verification against committed goldens
python3 src/test/golden/harness/run_scenario.py \
    --scenario src/test/golden/session_manager/scenarios/tree_readonly.py \
    --out /tmp/tree_readonly --impl python
```

### Limits

- Requires `<modeName>.py` with a `createMode()` entry point on the Python path.

---

## The six gates

Every Mu→Python package migration must pass these **six mandatory gates**, in order.
Package orchestrators (`run_migration_loop.sh` / `run_migration_loop_mac.sh`) run them
sequentially; on any failure the script exits immediately.

| Gate | Name | Command (orchestrator sets) | Pass criteria |
|------|------|-----------------------------|---------------|
| **0** | Runtime clean | `GATE=runtime IMPL=python` | Every scenario: no **new** runtime error signatures vs committed Mu `runtime_errors.txt` in the golden dir (`harness/runtime_log_check.py`). Same pre-existing RV noise as Mu is OK; regressions are not. Enforced on every `run_scenario.py` call when `--runtime-golden-dir` is passed. |
| **1** | Behavioral | `GATE=behavioral IMPL=python` | Every scenario: normalized `session.rv` matches committed golden (node graph, properties, connections). |
| **2** | Pixel | `GATE=pixel IMPL=python` | Every golden PNG: `rmsImageDiff -cmp -dmax 0` (see [Determinism requirements (gate 2)](#determinism-requirements-gate-2)). |
| **3** | Default launch | `GATE=default` | Every scenario: behavioral match with `--impl default` (no `RV_MODE_IMPL_*`; RV picks its shipped default). |
| **4** | Mu baseline integrity | `GATE=both IMPL=mu` | Every scenario: Mu implementation still matches committed goldens (harness and baselines sound). |
| **5** | Python unit tests | `harness/run_unit_tests.sh` | Every Mu method/function in the package has recorded behavior in `COVERAGE.md` and a corresponding passing Python unit test (or chain test — see [Gate 5](#gate-5--python-unit-tests)). |

Gates **1** and **2** are complementary: behavioral catches logic the graph can see; pixel
catches layout/rendering it cannot. Some inventory items apply to one gate only (noted in
each package's `COVERAGE.md`).

### Gate 0 — Runtime clean

Mu capture writes `runtime_errors.txt` beside `session.rv` — one normalized
signature per line (tracebacks collapsed to stable ``Exception @ File …`` form).
`run_scenario.py` captures RV output to `$out/rv.log`. Exit code **5** if the run
introduces signatures **not** in that Mu baseline. Pre-existing RV/core noise
recorded at capture time passes; new package regressions do not. Missing
`runtime_errors.txt` means an empty baseline (strict until re-capture or
backfill). Event-handler exceptions do not fail the scenario script itself — this
gate catches them.

### Gate 1 — Behavioral

After a scripted scenario, `compare.py` diffs normalized GTO from `saveSession(sparse=False)`
against the committed golden.

### Gate 2 — Pixel

After a scripted scenario, panel/viewport PNGs are compared at `-dmax 0` under the pinned
headless path (Xvfb + software Mesa on Linux; real display + `golden-mac/` on macOS).

### Gate 3 — Default launch

Same behavioral check as gate 1, but scenarios run with `--impl default` so RV's normal
mode-selection path is exercised (not an explicit `RV_MODE_IMPL_*` override).

This gate is only meaningful once the default actually selects the port. While the
default was Mu-first it passed by re-testing Mu, duplicating gate 4 and proving nothing
about the migration. With Python-first defaulting it exercises the same implementation a
user gets.

### Gate 4 — Mu baseline integrity

Full scenario pass with `IMPL=mu`. Confirms committed goldens and harness still match Mu;
never hand-edit `golden/` or `golden-mac/`.

### Gate 5 — Python unit tests

Golden scenarios prove end-to-end parity; gate 5 pins **method-level** behavior of the
Python port. Before or during the migration loop, the agent must:

1. **Inventory every Mu method and function** in the package (`.mu` sources) — including
   private helpers, Qt overrides, and mode callbacks.
2. **Record behavior** for each entry in `COVERAGE.md` § Mu methods → Python unit tests:
   inputs, side effects, return values, graph/property mutations, and error paths observed
   from the Mu implementation.
3. **Write Python unit tests** under `src/test/golden/<package>/unit/test_*.py` that assert
   the same behavior on the Python port.

**Chain tests:** when Mu logic is only meaningful as a call chain (A calls B calls C with
shared state), one unit test may cover the **chain** instead of three isolated tests —
as long as the chain is named in the inventory and every Mu entry in that chain is accounted
for (either its own test or a named chain test).

**Pass criteria:**

- `COVERAGE.md` Mu-methods table has no ⬜ rows (every Mu method/function mapped).
- `python3 -m pytest src/test/golden/<package>/unit/` exits 0 via
  `harness/run_unit_tests.sh`.
- Each inventory row points at a `test_*.py` test (or chain test id).
- **The tests import the port.** A unit test that re-implements the logic it is
  checking passes whether or not the port exists and pins nothing. The cheap way to
  confirm a suite is real: move the ported module aside and re-run — gate 5 must fail.
- **Skips are not passes.** `run_unit_tests.sh` selects an interpreter that has the
  same PySide6 the port runs against (RV's bundled one; override with
  `GOLDEN_PYTHON`) and fails if zero tests execute, since a suite that skips itself
  reports success just as loudly as one that works.

**What to test:** pure logic, property formatting, parsing, state transitions, and helpers
that golden scenarios only exercise indirectly. Mock RV/Qt boundaries when needed — unit
tests must not require a live RV process unless unavoidable.

**Platform baselines:** Mac output compares to `golden-mac/` only; Linux to `golden/` only
— never cross-compare ([Mac-native capture](#mac-native-capture)).

**Conditional steps** (not gates): after gates 0–2 pass, the orchestrator may run [GUI
sanity](#gui-sanity-real-display) and the [code review agent](#code-review-agent). These
are required before calling migration done but are not numbered gates.

---

Used by every package inventory. **A ✅ means the behavior is pinned by a committed golden
(the Mu ground truth is recorded) — NOT that it has been verified in the Python port.**

- ✅ **covered** — a committed scenario exercises and pins this behavior.
- 🟡 **partial** — touched by a scenario but not fully pinned (e.g. rendered but not
  interacted with).
- ⬜ **todo** — no scenario yet; listed in the package's scenario backlog.

Behaviors with no deterministic graph/pixel outcome and no command equivalent (modal UI,
settings persistence, async previews, "event was sent") are **dropped** — removed from the
inventory rather than tracked — and noted in a short "Dropped" section per package.

---

## Primary outcomes (required per package)

**Scenario count is not thoroughness.** Before writing scenarios or capturing baselines,
every package's `COVERAGE.md` must open with a **Primary outcomes** section (see template
below). These are the 1–5 things a user would notice if the port broke — not chrome,
not "mode activated", not widget layout alone.

An agent (or human) must fill this **before** the user approves the behavior inventory.
Do not capture Mu goldens until Primary outcomes rows are approved.

### Rules

1. **Name the discriminant.** For each primary outcome, state what changes in `session.rv`
   *and* what should change on screen (if anything). Example: `request.imageComponent`
   → layer name; viewer shows red vs green vs blue patches from `test_layers.exr`.

2. **At least one scenario per primary outcome must pin the full outcome:**
   - **Behavioral (B):** the graph property (or equivalent) must differ from the unselected
     / default state in committed `session.rv` — not merely logged as a NOTE.
   - **Pixel (P), when user-visible:** if the outcome changes what is displayed, capture
     **two** viewport states (before/after or A vs B) that **must differ** in the image
     region, not only in the margin/widget. 

3. **Fixtures must be used.** If you add a fixture with visually distinct variants (colored
   layers, two clips, different resolutions), at least one primary-outcome scenario must
   exercise that discriminant. Do not add discriminants "for manual testing only."

4. **API vs click is not a substitute.** A command-API scenario may pin B; a real-click
   scenario may pin the trigger. Neither alone satisfies a **user-visible** primary outcome
   unless the click scenario also asserts B **and** (when applicable) P. Marking the
   inventory row ✅ while the click golden still has the default/empty outcome is **not
   allowed** — use 🟡 only until fixed, and do not call coverage complete.

5. **Scenario script must fail capture if outcome wrong.** Use `assert` on properties before
   `saveSession`. Do not commit baselines when the scenario logs "NOTE: outcome unchanged"
   unless that unchanged state *is* the behavior under test.

6. **Review checkpoint.** Before `./capture_golden*.sh`, the agent posts the Primary
   outcomes table to the user. User must confirm each row has a planned scenario id that
   satisfies rules 2–5.

### COVERAGE.md template (copy into every new package)

```markdown
## Primary outcomes

| # | User-visible outcome | Graph / property signal | Pixel discriminant | Scenario(s) | B | P |
|---|----------------------|-------------------------|------------------|---------------|---|---|
| 1 | … | … | … (e.g. viewport A vs B) | … | req | req if visible |

Rows must be ✅ (or justified 🟡) before migration is done. Secondary behaviors (menus,
shortcuts, dock/float chrome) are listed in the inventory tables below — they do not
replace primary outcomes.
```

---

## The harness

Reusable across all packages; lives in `src/test/golden/harness/`.

| File | Role |
|---|---|
| `run_scenario.py` | Launches RV headless (Xvfb + software Mesa), runs an in-process scenario, collects artifacts into an out dir. Captures RV log to `$out/rv.log`. With `--runtime-golden-dir`, fails on **new** runtime errors vs Mu `runtime_errors.txt`; use `--allow-runtime-errors` for Mu capture only. Pass `--impl mu\|python` and optional `--mode` / `--package`. Runs `golden_bootstrap.py` before each scenario. |
| `runtime_log_check.py` | Extracts normalized runtime signatures from `rv.log` / `traceback.txt`; delta-check vs golden `runtime_errors.txt`; `--write-baseline` for capture. |
| `golden_bootstrap.py` | In-RV pre-scenario hook: activates `source_setup` when `GOLDEN_SOURCE_SETUP=1` (set automatically for `tree_readonly.py`). |
| `migration_loop_agent_reminder.sh` | Sourced by `run_migration_loop*.sh` — prints agent read checklist; loop procedure in `mu-python-migration` skill §5. |
| `run_unit_tests.sh` | Gate 5 — runs `pytest` on `<package>/unit/test_*.py`. |
| `compare.py` | Normalized GTO diff + `rmsImageDiff` pixel compare (gates 1 and 2). Exit 0 = PASS. |

### Layout per package

```
src/test/golden/
  VERIFICATION.md            # this file (shared method)
  harness/                   # shared run_scenario.py + compare.py
  <package>/
    COVERAGE.md              # package-specific behavior inventory + file list
    scenarios/<id>.py        # in-RV scenarios (command-API driven; QTest for DnD)
    run_migration_loop.sh    # full migration loop orchestrator (Linux)
    run_migration_loop_mac.sh
    run_all_goldens.sh       # scenario runners (orchestrator only; debug individually)
    run_all_goldens_mac.sh
    run_gui_sanity_gate.sh   # conditional sanity step (orchestrator calls this)
    capture_golden.sh        # Mu baseline capture (Linux)
    capture_golden_mac.sh    # Mu baseline capture (macOS)
    golden/<id>/             # committed Linux baselines: session.rv, runtime_errors.txt (+ *.png)
    golden-mac/<id>/         # committed macOS baselines (separate pixel space)
    unit/test_*.py           # gate 5 — Python unit tests (one module or chain per Mu method/group)
```

Package-specific harness notes (fixtures, mode/package name mismatches, headless
caveats) belong in `COVERAGE.md`, not a separate doc — unless the package needs a
one-line pointer file, keep everything in COVERAGE.

### Headless operational rules

- Launch under `xvfb-run` with `LIBGL_ALWAYS_SOFTWARE=1`. **`QT_QPA_PLATFORM=offscreen`
  segfaults RV** (its offscreen GL plugin needs GLX). Software Mesa under Xvfb is
  deterministic given a pinned Mesa version.
- **RV redirects stdout to its own log** (`~/.local/share/rv.bin/rv.bin.log`). Scenarios
  must write results to explicit files under `$GOLDEN_OUT`, not print them.
- **`close()` does not quit a windowless RV.** Every scenario ends by hard-exiting; the
  runner wraps scenarios so they always `os._exit`.
- `-pyeval` runs **before** the Qt event loop, so widgets don't paint on their own — pump
  the event loop (`QApplication.processEvents`) before `grab()`.
- **Immediate modes** (e.g. `source_setup`) load at `state-initialized` but start **inactive**
  in headless runs; the harness re-activates them via `golden_bootstrap.py` when needed.
  Set `GOLDEN_SOURCE_SETUP=1` to force color setup for all scenarios (default: off except
  `tree_readonly.py`, which pins movieproc `sRGB2linear=1`).
- Scenarios drive the package via the `rv.commands` API (deterministic, headless-safe).
  Drag-and-drop and other pointer interactions need synthetic Qt input events (`QTest`);
  schedule those scenarios last.

---

## Determinism requirements (gate 2)

A hard `-dmax 0` gate is only safe if capture is bit-reproducible.

| Source of nondeterminism | Fix |
|---|---|
| GPU/driver variance | Render through **software Mesa under Xvfb**, not the GPU; pin the Mesa version. |
| Async thumbnails/previews | Use media-free fixtures where possible; else quiesce on the relevant "available" event for every item before grabbing, **or** crop the nondeterministic region out of the PNG before diffing (`rmsImageDiff` has no ROI/mask). |
| Fonts / hinting | Pin a bundled font + fixed `fontconfig`; set a fixed `QT_FONT_DPI`. |
| HiDPI scaling | `QT_SCALE_FACTOR=1`, `QT_ENABLE_HIGHDPI_SCALING=0`, fixed widget size. |
| Animations / hover | Disable animations; command-API driving avoids stray focus/hover. |
| Xvfb / Mesa drift | Pin Xvfb screen geometry and Mesa version; capture goldens in the same container/path used to test. |

Gate at `-dmax 0` (exact); loosen `dmax` only if residual noise is *observed*, and always
log `-m` (max error) so drift surfaces. Never loosen `dmax` to paper over flakiness — a
flaky gate trains the AI loop to hack the oracle.

---

## GUI sanity (real display)

`run_all_goldens.sh` above is deterministic, but it pins exactly one rendering path: Xvfb +
software Mesa. A port can pass that gate while being visibly broken under a real
GPU/compositor/font stack — or, less obviously, the reverse: differ from Mu only because of
environment noise the headless path can't see. `<package>/run_gui_sanity_gate.sh` exists to
catch that class of regression by re-running the same scenarios against a real on-screen
display instead of Xvfb.

This is a **required step, not a smoke test — and deliberately not a numbered gate.** It runs
two independent checks per scenario:

- **Behavioral (node graph):** same as headless, always exact, HARD. A real-display run
  producing a different node graph than the pinned golden is exactly as serious as it is
  headlessly, and fails the script's exit code (the loop must iterate again).
- **Pixel (panel.png etc.):** no threshold, no verdict, via `compare.py --pixel-mode
  report`. Real GPU/font/compositor rendering is never byte-identical to a golden captured
  under Xvfb + software Mesa, so any fixed `dmax` is wrong in one of two directions: tight
  enough to catch real regressions and it fails permanently on rendering noise (training
  whoever/whatever runs the loop to ignore this gate as always-red, which is worse than not
  having it); loose enough to stay quiet on noise and it can silently swallow a real
  regression. Rather than guess a number, the gate prints quantitative info — RMS, the
  max-diff pixel location and its two values, and both PNG paths — and leaves the judgment to
  a **reviewer, human or AI**: does this look like ordinary rendering noise (anti-aliasing,
  font hinting, GPU vs. software raster) or a real behavioral/visual regression? If judged
  real, that's a failure for this iteration even though the script exited `0` — the AI
  running the loop is expected to open the flagged PNGs (its own image-reading tool, or by
  eye) and make that call itself, same as a human would eyeball a screenshot.

Missing artifacts are still a hard fail either way — whether a PNG was produced at all is
objective (a broken port that can't find the widget never writes the file), only its pixel
*content* is left to review.

`run_gui_sanity_gate.sh` prints a `NEEDS_AI_REVIEW:` line listing every scenario whose
behavioral gate passed but which has a pixel report attached, so the reviewer knows exactly
which scenarios to look at without re-reading the whole log.

The script's final phase re-runs scenarios with `--impl default` on a real display. See
[Gate 3](#gate-3--default-launch); the orchestrator runs the authoritative Gate 3 pass via
`run_all_goldens*`.
---

## Mac-native capture

Unlike [GUI sanity](#gui-sanity-real-display) (real display, but judged against Linux
`golden/` baselines, so pixel is report-only), macOS migration uses a **separate committed
baseline tree** `golden-mac/`: Mac output is compared only against Mac-captured baselines via
`run_all_goldens_mac.sh` / `capture_golden_mac.sh`. It does not replace `golden/` — the two
pixel spaces are not comparable and are never diffed against each other.

**Why separate Mac baselines work**:

- Two back-to-back captures of `tree_readonly` produced a byte-identical `session.rv` and a
  byte-identical `panel.png` (`rmsImageDiff -m` reported no diff). Real-display rendering
  *can* be bit-reproducible on a fixed machine/session, unlike the cross-machine/cross-GPU
  case the GUI sanity gate exists for.
- The *behavioral* graph captured on Mac matched the already-committed Linux `golden/`
  baseline exactly — the node graph is platform-independent, so in principle `golden-mac/`'s
  behavioral half is redundant with `golden/`'s. It's still captured and stored per-scenario
  in `golden-mac/` (not deduplicated against `golden/`) to keep each platform's baseline set
  self-contained per the [layout convention](#the-harness).
- *Pixel* is not platform-independent and cannot be pinned once for both: the same scenario's
  Mac capture came out at exactly 2x the Linux golden's raw pixel dimensions (Retina/HiDPI
  backing-scale-factor), before any content is even compared. `golden-mac/` pixel baselines
  are mandatory and separate.

**Determinism is not assumed globally — it's enforced per capture machine.**
`capture_golden_mac.sh` runs every scenario twice back-to-back and refuses to commit a
baseline unless both runs are byte-identical (session.rv via `diff`, every PNG via
`rmsImageDiff -m` showing no max-diff line). A scenario that fails this check is skipped
with an error, never committed with a caveat — same "never loosen the gate to paper over
flakiness" principle as everywhere else in this doc, applied automatically at capture time
instead of discovered later.

---

## Migration loop

The migration loop runs [the six gates](#the-six-gates) via each package's orchestrator.
Package inventories (`COVERAGE.md`) hold behavior lists, fixtures, and package-specific
debug hints only.

**Purpose:** the loop is meant to be **agent-driven** — start it with Cursor's `/loop`
command so the agent re-runs the migration prompt each tick until
[Definition of done](#definition-of-done) is satisfied. Example:

```text
/loop 5m Migrate <package> Mu→Python: read COVERAGE.md, fix Python port, run ./run_migration_loop_mac.sh, satisfy all six gates and Definition of done.
```

The shell orchestrator runs gates 0–5; the agent owns inventory, unit-test authoring,
sanity pixel review, and code review between ticks.

### Orchestrator script

Each package provides `run_migration_loop.sh` (Linux / `golden/`) and/or
`run_migration_loop_mac.sh` (macOS / `golden-mac/`). One invocation runs gates **0 → 5**
in order, then conditional GUI sanity and code-review steps if gates 0–2 passed.

Scenario iteration lives in `run_all_goldens*.sh`; gate sequencing lives in the
orchestrator. On any failure the script **exits immediately**. Fix the port, then run the
**same script again** — the agent drives re-runs (see skill §5).

```bash
# macOS (typical dev path when golden-mac/ exists):
cd src/test/golden/<package>
./run_migration_loop_mac.sh

# Linux (after capture_golden.sh):
./run_migration_loop.sh
```

Do **not** run `GATE=behavioral ./run_all_goldens*.sh` as the normal workflow — that is
for debugging a single failing scenario only.

Individual gate env vars (`GATE`, `IMPL`) are set by the orchestrator — do not override
when running the full loop. See [The six gates](#the-six-gates) for what each gate checks.

Orchestrator env vars (all packages):

| Variable | Default | Purpose |
|----------|---------|---------|
| `SKIP_SANITY` | `0` | `1` = skip GUI sanity step (local dev only) |
| `SKIP_REVIEW` | `0` | `1` = skip code-review reminder (local dev only) |
| `SKIP_PIXEL_GATE` | `0` | `1` = accept gate 2 failure and continue (package-specific; use sparingly) |

If gate 0, 1, or 2 fails, the orchestrator never reaches sanity, review, or gates 3–5.

Each `./run_migration_loop*.sh` run prints an agent reminder via
`harness/migration_loop_agent_reminder.sh`. **Exit code 0 does not mean migration is done**
— see [Definition of done](#definition-of-done).

### On failure — which gate?

| Script output | Typical cause |
|---------------|---------------|
| `GATE 0 FAILED` | New runtime error vs Mu `runtime_errors.txt` — see `$out/runtime_errors.txt` |
| `GATE 1 FAILED` | Wrong graph/properties — logic, property writes, mode lifecycle |
| `GATE 2 FAILED` | Visual regression — layout, GL render, widget state |
| `SANITY FAILED` | Real-display behavioral drift (same class as gate 1) |
| `NEEDS_AI_REVIEW` | Pixel diff on real display — inspect PNGs; see [GUI sanity](#gui-sanity-real-display) |
| `GATE 3 FAILED` | Broken default launch path — PACKAGE wiring, mode registration, preload |
| `GATE 4 FAILED` | Harness or golden corruption — re-capture Mu; never hand-edit goldens |
| `GATE 5 FAILED` | Missing/incomplete `unit/test_*.py`, pytest failure, or ⬜ rows in COVERAGE.md Mu-methods table |

Package-specific fix hints live in that package's `COVERAGE.md`.

**Debug one scenario** (exception to full loop):

```bash
python3 src/test/golden/harness/run_scenario.py \
  --scenario src/test/golden/<package>/scenarios/<id>.py \
  --out /tmp/golden_debug --impl python \
  --mode <modeName> [--package <pkgDir>]   # when dir ≠ mode name
cat /tmp/golden_debug/diag.txt
python3 src/test/golden/harness/compare.py \
  --golden-dir src/test/golden/<package>/golden-mac/<id> \
  --actual-dir /tmp/golden_debug --dmax 0
```

(use `golden/` instead of `golden-mac/` on Linux; add `--no-xvfb` on macOS)

---

## Code review agent

The [six gates](#the-six-gates) check *behavior* — graph, pixels, runtime, defaults, and
method-level unit tests. None read the *code* holistically. Before treating a loop iteration
as done, a fresh independent agent reviews
the actual diff for correctness — defects that pass every gate because the suite did not
exercise them.

**Scope: one iteration, not the whole branch.** Review the diff between the current commit
and its immediate parent (`git diff <parent>..HEAD`), not the full branch-vs-`main` history —
the latter is almost always far larger than what any one iteration actually touched, and
reviewing it wastes the agent's attention on code nobody just changed. Exclude binary/generated
artifacts (`golden/`, `golden-mac/` PNGs and `session.rv` files) — review the code that
produced them, not the artifacts themselves.

**Mechanism:** spawn a fresh `general-purpose` agent (there is no dedicated `code-reviewer`
agent type in this environment) with a prompt that gives it the specific commit range, the
context of what changed and why (a fresh agent has none of the implementing agent's context —
it must be given enough to make real judgment calls, not just told to "review this"), and
specific things to check per file. Have it report via the `ReportFindings` tool, ranked
most-severe first. **Do not** use the `/code-review` slash command for this — it's gated to
explicit user invocation only (`disable-model-invocation`) and cannot be called
programmatically by a loop.

**Enforcement:** blocking findings (wrong logic, unsafe assumptions, incomplete fixes) require
another fix-and-retry cycle. Non-blocking findings (style, minor nits) are reported but do
not fail the run.

Conditional step after gates 0–2 pass in the migration loop — agent procedure in
[mu-python-migration skill §5](../../.agents/skills/mu-python-migration/SKILL.md).
The orchestrator prints a reminder; the implementing agent spawns the reviewer and acts on
blocking findings.

---

## Definition of done

A package migration is accepted when:

1. Every coverage item in `COVERAGE.md` is ✅ (a passing golden scenario pins it).
2. **Primary outcomes** (top of `COVERAGE.md`) are all ✅ — each has behavioral pin and,
   when user-visible, pixel before/after (see [Primary outcomes](#primary-outcomes-required-per-package)).
3. [The six gates](#the-six-gates) pass via `./run_migration_loop*.sh` on the target
   platform(s) — including gate 5 (every Mu method recorded; Python unit tests green).
4. [GUI sanity](#gui-sanity-real-display) has run: behavioral matches; pixel report reviewed
   and judged acceptable.
5. On macOS (if in scope): compare against `golden-mac/` — see [Mac-native capture](#mac-native-capture).
6. No inventory item is left 🟡 without an explicit, recorded justification (primary
   outcomes may not stay 🟡).
7. Any cross-package API the package exposes (callable from other Mu/Python packages)
   remains callable — verified by a scenario or integration check before the Mu source is
   removed.
8. The [code review agent](#code-review-agent) has run against this iteration's diff with no
   unresolved blocking findings.

## Allowed Operations

1. No file under golden/ or golden-mac/ shall be modified by hand — only
   capture_golden.sh / capture_golden_mac.sh may write them, and only when the
   determinism check passes
2. No more than 15 attempts at running all tests is allowed (15 iterations)
3. [GUI sanity](#gui-sanity-real-display) (`run_gui_sanity_gate.sh`) must run before a
   migration is considered done — behavioral is hard pass/fail; pixel is report-only and must
   be reviewed each run
4. golden/ and golden-mac/ are separate pixel spaces and must never be compared against each
   other or merged — a Mac capture failing against `golden/` (or a Linux capture failing
   against `golden-mac/`) is not a real signal, just a platform mismatch; always compare Mac
   output to `golden-mac/` and Linux output to `golden/`
5. Before calling an iteration done, run the [code review agent](#code-review-agent) on this
   iteration's diff (not the whole branch); blocking findings require another fix-and-retry
   cycle
