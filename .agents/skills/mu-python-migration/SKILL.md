---
name: mu-python-migration
description: >-
  Orchestrate migrating an RV plugin package from Mu to Python via golden tests.
  Use when the user asks to port, migrate, or convert a Mu package to Python,
  build coverage, capture baselines, or run the migration loop.
---

# Mu → Python migration

One **migration loop** per package — port the **whole package**. Mu sources stay until
everything in `COVERAGE.md` satisfies [Definition of done](../../src/test/golden/VERIFICATION.md#definition-of-done).

Follow these phases in order. Do not skip user checkpoints.

## 0. Read VERIFICATION.md (once per session — mandatory)

Before any inventory, scenarios, capture, or loop run, read
[`src/test/golden/VERIFICATION.md`](../../src/test/golden/VERIFICATION.md) **in full**
(not just the loop section): six gates, Primary outcomes, harness, Definition of done,
capture workflow, orchestrator mechanics. **Loop procedure** lives in this skill §5 only.

The shell orchestrator **does not** parse VERIFICATION.md or COVERAGE.md. You are the
enforcement layer.

## 1. Confirm package

Ask the user which package to migrate (`src/plugins/rv-packages/<name>/`).

Open `src/test/golden/<package>/COVERAGE.md` (create from
[`COVERAGE.template.md`](../../src/test/golden/COVERAGE.template.md) if missing).

## 2. File inventory

Identify:

- Mu sources (`.mu`, `.mu.in`, generated outputs)
- Existing Python in the package
- Assets to keep unchanged (`.ui`, icons, `PACKAGE`, images)
- Files to create (Python port, scenarios under `src/test/golden/<package>/scenarios/`)
- **External callers** — other packages or scripts that depend on this package

Present the list to the user. After they approve, record it in the package's `COVERAGE.md`.

## 3. Behavior coverage

Find every observable behavior in the package. Be extremely thorough — launch parallel
agents for large packages if needed.

### 3a. Primary outcomes first (mandatory — do not skip)

Before the detailed inventory, add **`## Primary outcomes`** at the top of `COVERAGE.md`.
Read [`VERIFICATION.md` § Primary outcomes](../../src/test/golden/VERIFICATION.md#primary-outcomes-required-per-package).

For each primary user-visible outcome (what the user notices if the port broke):

1. Name the **graph signal** (property / node change).
2. Name the **pixel discriminant** if the screen should change (before/after or A vs B).
3. Assign scenario id(s) that will **assert** both — not merely exercise UI chrome.
4. Post the table to the user and **wait for approval** before writing scenarios or capturing.

**Stop conditions — do not mark coverage complete or capture baselines if:**

- A fixture has visual variants (e.g. colored layers) but no scenario uses them to prove selection.
- The only outcome pin is command-API while the real-click scenario golden still shows default/empty state.
- Every viewport golden shows the same image color/state; only the margin widget differs.

**Discovery checklist:**

- Event bindings and handlers (what fires on graph/UI changes)
- Menus, buttons, shortcuts, dialogs
- Property reads/writes and cross-node effects
- Qt subclass overrides (especially drag/drop and custom widgets)
- Toggle combinations — exercise on/off pairs that change visual or behavioral outcomes
- **Trigger vs outcome** — command-API scenarios pin outcomes; real UI triggers are
  separate scenarios. For **primary outcomes**, both B and (if visible) P must be pinned;
  see VERIFICATION.md Primary outcomes rules — API + click-without-outcome is not enough.

**Real media:** if a behavior needs loading files (thumbnails, codecs, paths), ask the user
how to supply fixtures — do not hardcode machine-specific paths in the repo.

**Scenarios:** one file = one linear scripted run. A scenario may chain many steps and write
multiple PNG artifacts; `compare.py` diffs all of them. Behaviors with no deterministic
outcome belong in COVERAGE.md's **Dropped** section, not as eternal ⬜ items.

Map each behavior to gate(s) **B** / **P**, status (✅/🟡/⬜), and scenario id in
`COVERAGE.md`.

### 3b. Mu methods → Python unit tests (mandatory — gate 5)

After the behavior inventory (§3), walk **every Mu method and function** in the package:

1. List each symbol in `COVERAGE.md` § Mu methods → Python unit tests (use the template).
2. **Record behavior** from the Mu implementation: parameters, return values, side effects,
   property/graph mutations, error paths.
3. **Write Python unit tests** in `src/test/golden/<package>/unit/test_*.py` that assert the
   same behavior on the port.
4. **Chain tests:** when Mu logic only makes sense as a sequence (shared state, internal
   helpers never called alone), one test may cover the whole chain — name it in the inventory
   and list every Mu symbol the chain replaces.

Run gate 5 locally while iterating:

```bash
GOLDEN_PKG_DIR=src/test/golden/<package> src/test/golden/harness/run_unit_tests.sh
```

Do not call migration done while any Mu-methods row is ⬜ or pytest fails.

## 4. Baselines

Ask the user for permission before capturing or committing goldens.

**Pre-capture checklist** (agent must verify aloud):

- [ ] Primary outcomes table approved by user
- [ ] Each primary-outcome scenario has `assert` on graph state before `saveSession`
- [ ] User-visible primary outcomes capture at least two viewport PNGs that must differ
- [ ] No scenario commits a golden after logging "outcome unchanged" unless that is the behavior under test

Use the capture workflow in `VERIFICATION.md` — never hand-edit files under `golden/` or
`golden-mac/`.

## 5. Migration loop

**Purpose:** drive this phase with Cursor **`/loop`** so the agent re-runs the migration
prompt on a schedule until [Definition of done](../../src/test/golden/VERIFICATION.md#definition-of-done)
is satisfied. Example:

```text
/loop 5m Migrate <package> Mu→Python: read COVERAGE.md in full, fix Python port under src/plugins/rv-packages/<package>/, run ./run_migration_loop_mac.sh, pass all six gates, complete Mu-methods unit tests, sanity pixel review, code review.
```

Ask the user whether `COVERAGE.md` is complete, Mu baselines are committed, Mu-methods
inventory is started, and you may start the loop.

Orchestrator scripts and env vars:
[`VERIFICATION.md` § Migration loop](../../src/test/golden/VERIFICATION.md#migration-loop).
Pass criteria when done:
[Definition of done](../../src/test/golden/VERIFICATION.md#definition-of-done).

### Reading protocol (mandatory)

| When | Read | Why |
|------|------|-----|
| **Once per migration session** (§0, before inventory or first loop run) | [`VERIFICATION.md`](../../src/test/golden/VERIFICATION.md) **in full** | Gates, Primary outcomes rules, Definition of done, harness — the verification contract. |
| **Every loop iteration** (before each `./run_migration_loop*.sh`, and after every failure or pass before declaring progress) | `<package>/COVERAGE.md` **in full** | Primary outcomes table, ✅/🟡/⬜ statuses, scenario map — stay aligned with what “done” means. |

Each `./run_migration_loop*.sh` run prints a reminder via
`harness/migration_loop_agent_reminder.sh`. **Exit code 0 ≠ migration done** — cross-check
Definition of done against `COVERAGE.md` every iteration.

### Loop algorithm

```
# Once per migration session (§0, before first loop):
read VERIFICATION.md in full

attempts = 0
while attempts < 15:
    attempts += 1
    read <package>/COVERAGE.md in full   # Primary outcomes + statuses EVERY iteration
    fix Python port under src/plugins/rv-packages/<package>/
    run: ./run_migration_loop_mac.sh   # or run_migration_loop.sh on Linux
    if exit 0:
        re-read COVERAGE.md — confirm Definition of done (not just gate pass)
        judge NEEDS_AI_REVIEW pixel reports (if any) — real regression → keep looping
        run code review agent — blocking findings → fix and keep looping
        if both satisfied AND COVERAGE complete → DONE
    else:
        read which GATE failed from script output
        diagnose (diag.txt under /tmp, compare.py on failing scenario, pytest for gate 5)
        map failure to COVERAGE.md rows — missing primary outcome test?
        if root cause is still unclear → keep investigating; do not hand off yet
report failure after 15 attempts
```

Gate failure meanings and debug-one-scenario commands:
[`VERIFICATION.md` § On failure](../../src/test/golden/VERIFICATION.md#on-failure--which-gate).
Gate 5: `harness/run_unit_tests.sh` and `COVERAGE.md` Mu-methods table.

### Do not

- Run gates or scenarios individually except when debugging one failure
- Use iteration counters or env vars (`ITERATION=`, etc.)
- Call migration done until the orchestrator exits 0 **and** sanity pixel review **and**
  code review (no blocking findings) are satisfied — see Definition of done
- **Stop early because of a blocker** — see below

### When you hit a blocker — keep going until unblocked

A **blocker** is anything that prevents the next gate from passing even though the
immediate Python diff looks “done”: harness wiring, mode activation/registration, Mu helper
not loading, segfaults in a render path, optional-package preload under `-noPrefs`, stale
`rvpkg`/`rvload2`, missing fixtures, etc.

**Do not** treat a blocker as a reason to pause the loop, summarize, or ask the user
“what next?” unless you genuinely need a product decision or credentials you cannot infer
from the repo.

**Do** stay in the loop until the blocker is removed:

1. **Name the blocker precisely** — e.g. `LayerSelectRender` inactive (`isModeActive`
   false), not vague “pixel mismatch”.
2. **Debug in isolation** — one scenario, `diag.txt`, minimal repro, Mu vs Python, with/without
   harness flags; read RV stderr and mode-manager messages.
3. **Try the next fix** — PACKAGE/`rvload2`, preload, separate rvpkg, harness env, Mu bridge,
   alternate architecture; rebuild staged artifacts when needed.
4. **Re-run the orchestrator** after each meaningful change (`./run_migration_loop*.sh`, or
   the single failing gate/scenario while iterating).
5. **Repeat** until the gate passes or you hit the 15-run cap.

Stopping with “here’s the blocker” without exhausting reasonable fixes counts as an
**incomplete loop run**. The user expects the agent to **keep doing what it takes** to
unblock — same session, same task — not defer infrastructure work back to them.

Only escalate to the user when:

- You need an explicit product/architecture choice (e.g. modify a core cpp file)
- You need assets or credentials not in the repo
- 15 full loop attempts failed and you can document what was tried

### Execute

If the user approves starting the loop:

1. Implement the Python port for the **entire package** (tackle areas in any order).
2. **Run the migration loop** until it passes (max 15 failed runs):
   ```bash
   cd src/test/golden/<package>
   ./run_migration_loop_mac.sh   # or run_migration_loop.sh on Linux
   ```
3. Package-specific harness notes and failure hints: `COVERAGE.md` for that package.
4. Do not remove Mu sources until the user approves, after Definition of done is satisfied.
5. Update `COVERAGE.md` statuses as behaviors go green — keep the doc in sync every iteration.
6. When Definition of done is satisfied, ask the user about removing Mu sources and updating `PACKAGE`.
