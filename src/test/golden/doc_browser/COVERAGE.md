# `doc_browser` — Migration Coverage Contract

**Purpose.** Exhaustive list of `doc_browser` behaviors that MUST keep working when the
package is ported from Mu to Python. Each item maps to verification gate(s) and scenario id.
The port is done when every item here passes per [`../VERIFICATION.md`](../VERIFICATION.md).

**Source of truth.** Mu implementation:
`src/plugins/rv-packages/doc_browser/doc_browser.mu` (~1,794 lines), icon PNGs, `PACKAGE`,
`CMakeLists.txt`. Line numbers below refer to `doc_browser.mu` unless noted.

**Harness note (pixel-primary).** This mode does not mutate the RV node graph. Gate **1**
(behavioral) pins a stable empty `session.rv` across scenarios; correctness is enforced by
in-scenario `assert`s and gate **2** pixel PNGs of the doc browser window (`browser.png`).

---

## Primary outcomes

Approved 2026-07-29; expanded so **primary scenarios ≥ 50%** of the suite (8 / 15).

| # | User-visible outcome | Graph / property signal | Pixel discriminant | Scenario(s) | B | P |
|---|----------------------|-------------------------|--------------------|-------------|---|---|
| 1 | Doc browser opens showing the Mu API legend/start page | `isModeActive("doc_browser")` (assert; `session.rv` unchanged) | `browser.png`: start page with title + icon legend | `db_activate` | assert | req |
| 2 | Help → **Mu Command API Browser…** opens the doc browser | Deactivate then re-activate; `isModeActive` (assert); `session.rv` unchanged | `browser.png`: start page (same discriminant as #1, distinct trigger) | `db_activate_help` | assert | req |
| 3 | Selecting a module in the tree shows module documentation | Assert tree path `commands` (diag); `session.rv` unchanged | `browser.png`: module doc vs start page (must differ) | `db_select_symbol` | assert | req |
| 4 | Selecting a function shows signature + description HTML | Assert path `commands/addSources` (diag); `session.rv` unchanged | `browser.png`: function doc vs module doc (must differ) | `db_select_function` | assert | req |
| 5 | Selecting a type shows class/type layout HTML | Assert path `rvtypes/MinorMode` (diag); `session.rv` unchanged | `browser.png`: type page vs function/module pages (must differ) | `db_select_type` | assert | req |
| 6 | Search finds and lists matching symbols | Assert search query submitted (diag); `session.rv` unchanged | `browser.png`: search results vs start page | `db_search` | assert | req |
| 7 | `mudoc://` link navigates to the linked symbol | Assert navigated to `commands.addSources` (diag); `session.rv` unchanged | `browser.png`: function doc after link (vs prior page) | `db_link_nav` | assert | req |
| 8 | Back toolbar returns to the previous doc page | Assert after back, tree still on `commands` (diag); `session.rv` unchanged | `browser.png`: module page after back from function page (must differ from function PNG) | `db_back_forward` | assert | req |

**Secondary scenarios (7):** lifecycle, method/constant pages, asciidoc markup module, package
internals, search doc-text match — listed below; required for definition of done but not
primary-outcome rows.

---

## File inventory (approved 2026-07-29)

| Path | Role | Migration action |
|---|---|---|
| `doc_browser.mu` | Original Mu mode (monolithic) | Remove after full package passes |
| `doc_browser_mu.mu` | Mu bridge: symbol tree, HTML, `DocBrowser` widget | Keep until native Python symbol API |
| `doc_browser.py` | Python `DocBrowserMode` + `createMode()` | **Drafted** — `RV_MODE_IMPL_doc_browser=python` |
| `asciidoc_to_html.py` | Python asciidoc→HTML | Drafted (Mu copy still used by bridge) |
| `*.png` | Tree/start-page icons | Keep unchanged |
| `PACKAGE` | Mode `doc_browser`, `load: delay`, `system: true`, `hidden: true` | ✅ `modes:` → `doc_browser.py` |
| `CMakeLists.txt` | RVPKG target `doc_browser` | Unchanged (RVPKG picks up `.py`) |

**External callers:**

| Package | Usage |
|---|---|
| `openrv_help_menu` | Help → **Mu Command API Browser…** → `modeManager.activateMode("doc_browser", true)` |

**Files to create:**

| Path | Role |
|---|---|
| `doc_browser.py` | Python port + `createMode()` |
| `src/test/golden/doc_browser/scenarios/*.py` | Golden scenarios (15 total) |
| `src/test/golden/doc_browser/run_all_goldens*.sh` | Gate runners |
| `src/test/golden/doc_browser/run_migration_loop*.sh` | Migration loop orchestrator |
| `src/test/golden/doc_browser/capture_golden*.sh` | Mu baseline capture |
| `src/test/golden/doc_browser/run_gui_sanity_gate.sh` | GUI sanity step |

**Harness:** mode name == package dir (`doc_browser`). `db_activate_help` also preloads
`help` and passes `--menu-bar`; deactivates doc browser first, then re-activates via Help
menu (or `activateMode` fallback when the menu bar is hidden under `-nomb`).

**Port risks:**

- Mu runtime symbol introspection has no native Python API — expect Mu bridge via
  `runtime.eval` or a small retained Mu helper.
- `QWebEngineView` rendering — capture on macOS real display (`golden-mac/`).

---

## Verification method

Gates (**B** / **P**), migration loop, capture, definition of done:
[`../VERIFICATION.md`](../VERIFICATION.md).

Coverage legend: **✅** = pinned by committed Mu golden; **🟡** = partial; **⬜** = awaiting capture.

Mu baselines committed in `golden-mac/` (15 scenarios, 2026-07-29). Gate 4 Mu integrity ✅.
Gates 0–2 pass with `IMPL=python` using `doc_browser.py` + `doc_browser_mu.mu` bridge.

---

## Migration loop (this package)

```bash
cd src/test/golden/doc_browser
./run_migration_loop_mac.sh
```

**Gate failure hints:**

| Output | Fix focus |
|--------|-----------|
| Gate 0 | QWebEngine / symbol bridge runtime errors |
| Gate 1 | Unexpected `session.rv` drift (should stay default empty session) |
| Gate 2 | HTML layout, CSS, icons, QWebEngine render, column view |
| Gate 3 | Optional `system: true` package + `ModeManagerPreload=doc_browser` under `-noPrefs` |

---

## A. Activation & lifecycle

| # | Behavior | Ref | Gate | Status | Scenario |
|---|---|---|---|---|---|
| A1 | `activateMode("doc_browser")` opens browser window | 1779-1786, 1726-1771 | B+P | ✅ | `db_activate` |
| A2 | Help menu **Mu Command API Browser…** (or `modeManager` equivalent) | openrv_help_menu:64-68 | B+P | ✅ | `db_activate_help` |
| A3 | Mode loads delayed (`load: delay`) — inactive until toggled | PACKAGE:13 | B | ✅ | `db_activate` |
| A4 | `deactivate` hides window / mode inactive | 1774-1777 | B | ✅ | `db_deactivate` |
| A5 | Window `hideEvent` toggles mode off when user closes window | 1694-1697 | B | ✅ | `db_window_hide` |
| A6 | `before-session-deletion` closes browser | 1717-1718 | — | — | Dropped — no deterministic session artifact |

## B. Symbol tree & documentation

| # | Behavior | Ref | Gate | Status | Scenario |
|---|---|---|---|---|---|
| B1 | Start page HTML (legend table, icons) | 277-294, 1643 | P | ✅ | `db_activate` |
| B2 | Column view lists filtered/sorted Mu symbols | 651-864, 574-620 | P | ✅ | `db_activate`, `db_doc_browser_internals` |
| B3 | Selecting module updates web view | 1367-1419, 1305-1365 | B+P | ✅ | `db_select_symbol` |
| B4 | Function info (signature, params, mudoc links) | 916-989 | B+P | ✅ | `db_select_function`, `db_link_nav` |
| B5 | Method info page | 916-989, 1378 | B+P | ✅ | `db_select_method` |
| B6 | Type/class info (fields, constructors, methods tables) | 1109-1303 | B+P | ✅ | `db_select_type` |
| B7 | Symbolic constant / variable info | 909-914, 1382-1383 | B+P | ✅ | `db_select_constant` |
| B8 | `mudoc://` navigation updates tree + HTML | 1422-1507, 632-647 | B+P | ✅ | `db_link_nav` |
| B9 | Package-internal symbols (`DocModel`, `DocPage`, …) | 651-864, 622-649 | P | ✅ | `db_doc_browser_internals` |

## C. Search & navigation

| # | Behavior | Ref | Gate | Status | Scenario |
|---|---|---|---|---|---|
| C1 | Search box: plain text → `musearch:///` results | 1700-1712, 1527-1604 | B+P | ✅ | `db_search` |
| C2 | Search matches documentation text (subtext row) | 1582-1585 | P | ✅ | `db_search_doc_match` |
| C3 | Search box: `mudoc://` URL navigates directly | 1704-1707 | B+P | ✅ | `db_link_nav` |
| C4 | Back toolbar walks `_backHistory` | 1509-1525, 1766 | B+P | ✅ | `db_back_forward` |
| C5 | Forward toolbar (exercised before capture in scenario flow) | 1606-1620, 1767 | B | ✅ | `db_back_forward` |
| C6 | External `http(s)://` load | 1486-1495 | — | — | Dropped — network |

## D. `asciidoc_to_html`

| # | Behavior | Ref | Gate | Status | Scenario |
|---|---|---|---|---|---|
| D1 | Inline formatting: bold/italic/mono/pass/URLs | 73-124 | P | ✅ | `db_asciidoc_module` |
| D2 | Paragraph toggle on blank lines | 159-163 | P | ✅ | `db_asciidoc_module` |
| D3 | Bullet lists | 184-189 | P | ✅ | `db_asciidoc_module` |
| D4 | Listing/example `<pre>` blocks | 190-212 | P | ✅ | `db_asciidoc_module` |
| D5 | Tables with width/class attributes | 216-258 | P | ✅ | `db_asciidoc_module` |
| D6 | URL autolink | 113-116 | P | ✅ | `db_asciidoc_module` |

Module `asciidoc_to_html` documentation (embedded in `doc_browser.mu` lines 8-50) contains
all markup variants above — one scenario pins the rendered HTML via `db_asciidoc_module`.

---

## Dropped (no equivalent golden test)

| Behavior | Reason |
|---|---|
| External `http(s)://` page load in web view | Network/non-deterministic |
| Scroll position restore on back/forward | Code commented out in Mu |
| `before-session-deletion` close handler | No stable `session.rv` artifact |
| Print-on-exception diagnostics | Not user-visible |

---

## Scenarios (15 total — 8 primary, 7 secondary)

### Primary (8 — 53% of suite)

| Id | Gates | Trigger / outcome pinned |
|---|---|---|
| `db_activate` | B+P | `activateMode`; start/legend page |
| `db_activate_help` | B+P | Help menu / re-activate path (`openrv_help_menu` caller) |
| `db_select_symbol` | B+P | tree → `commands` module |
| `db_select_function` | B+P | tree → `commands` / `addSources` |
| `db_select_type` | B+P | tree → `rvtypes` / `MinorMode` |
| `db_search` | B+P | search → `commands` |
| `db_link_nav` | B+P | `mudoc:///commands.addSources` via search box |
| `db_back_forward` | B+P | module → function → toolbar back (module page PNG) |

### Secondary (7)

| Id | Gates | Trigger / outcome pinned |
|---|---|---|
| `db_deactivate` | B+P | active browser → `deactivateMode` |
| `db_window_hide` | B+P | close window → mode inactive |
| `db_select_method` | B+P | tree → `rvtypes` / `MinorMode` / `init` |
| `db_select_constant` | B+P | tree → `math` / `pi` |
| `db_asciidoc_module` | B+P | tree → `asciidoc_to_html` (markup-rich module doc) |
| `db_doc_browser_internals` | B+P | tree → `doc_browser` / `DocModel` |
| `db_search_doc_match` | B+P | search → `asciidoc` (doc-text match subtext) |

Runners: `run_all_goldens_mac.sh`, `capture_golden_mac.sh`. Mu baselines in
`golden-mac/` (15 scenarios, 2026-07-29). Gate 4 (`IMPL=mu`) verified ✅.

---

## Status summary

**15** scenarios scripted (**8 primary**, 7 secondary); Mu baselines committed in
`golden-mac/` ✅; Python port **drafted** (`doc_browser.py` + `doc_browser_mu.mu` bridge).

**Next checkpoint:** implement Python port (`doc_browser.py`) → `./run_migration_loop_mac.sh`
