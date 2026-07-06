# Crash Reporting in RV

Status: **Draft / source of truth in progress** — describes the intended architecture of the
minidump crash-reporting capability and the contracts that all crash-reporting code MUST follow.
Sections marked **Known gaps** track where the current implementation diverges from this spec.

This document is the authoritative design for crash reporting. When the code and this document
disagree, treat the disagreement as a bug in one of them and reconcile it — do not let a new code
path quietly re-invent a capability that already exists elsewhere.

## 1. Purpose

RV must capture crash dumps from end users so that crashes can be symbolicated and debugged
offline. A crash report must carry enough context (which Mu/Python script was executing, GPU info,
the session log) to be actionable without a reproduction.

## 2. Two distinct libraries — keep them disjoint

Crash reporting uses two **independent** Google projects. Conflating them is a recurring source of
bugs (see Known gaps D).

| Library | Role | Built on | Linked into the app? |
| --- | --- | --- | --- |
| **Crashpad** | In-process crash **capture** and the out-of-process `crashpad_handler` | macOS, Linux, Windows | Yes (statically) |
| **Breakpad** | Offline **symbolication tooling**: `dump_syms`, `minidump_stackwalk`, `minidump_dump` | macOS, Linux | No — build/CLI tools only |

Rules:

- Crashpad and Breakpad MUST NOT gate each other. A build condition for one must never be keyed on
  the other's version variable.
- Breakpad is not a runtime dependency. No application code includes Breakpad headers.
- Windows symbolication uses native PDBs (WinDbg/CDB), so Breakpad is intentionally not built on
  Windows. This is by design and MUST stay documented here so it is not mistaken for a gap.

## 3. Components and responsibilities

- `src/lib/base/TwkUtil/CrashHandler.{h,cpp}`, `CrashHandler_internal.h` — the singleton crash
  handler. Owns Crashpad client setup, the annotation table, log-file attachment, and the crash-dump
  directory.
- `src/bin/apps/rv/crashpad_handler_linux.sh.in`, `crashpad_handler_macos.sh.in` — thin per-platform
  wrappers around `crashpad_handler` that set the dump/log directory and redirect handler stderr to a
  log. The Linux wrapper additionally applies `ulimit -t 30` to defuse a known handler CPU-spin bug.
- `cmake/dependencies/crashpad.cmake`, `breakpad.cmake` — fetch/build the two libraries.
- `cmake/macros/rv_generate_symbols.cmake` + `src/bin/apps/rv/organize_symbols.sh` — build-time
  symbol generation; produce `.sym` files under `stage/app/symbols/<module>/<id>/`.
- `cmake/macros/rv_archive_symbols.cmake` (+ `rv_collect_pdbs.cmake`) — defines the `symbols_archive`
  build target that packages a versioned, per-platform symbol archive for offline symbolication; the
  install `pre_install*.cmake` filters strip symbols from the customer (Release) package (see §7).
- `src/bin/apps/rv/symbolicate_crash.sh` — developer-facing tool that symbolicates a `.dmp` against a
  Breakpad `symbols/` tree (its `--symbols <dir>` option points at an unpacked symbol archive).

## 4. Process model and lifecycle

1. As early as possible after `QApplication` construction — before any Mu/Python code runs — each
   executable calls the shared `TwkApp::initializeCrashHandler()` helper (C4), which resolves the
   platform handler wrapper next to the executable and calls `CrashHandler::initialize()`, so that
   even startup crashes are captured.
2. `initialize()` locates the platform handler, starts the Crashpad client, registers the static
   annotations, and sets the crash-dump directory.
3. Static, process-wide annotations (e.g. `platform`, `qt_version`) are set once at init.
4. Dynamic annotations (script/interpreter context, GPU info) are updated continuously while the app
   runs, so a dump reflects the program state at the moment of the crash.
5. The session log is attached via `attachLogFile()`. On macOS this requires restarting the handler
   (a Crashpad limitation) and is therefore a single post-init call.

### Crash-dump directories (per current code)

- macOS: `~/Library/Logs/ASWF/Crashes/`
- Linux: `~/.local/share/ASWF/OpenRV/Crashes/` (via `QStandardPaths::AppDataLocation`)
- Windows: `%APPDATA%\ASWF\OpenRV\Crashes\` (via `QStandardPaths::AppDataLocation`)

The `*.sh.in` wrappers MUST resolve the same directory the C++ handler uses. Any divergence between
the two is a bug.

## 5. Contracts (normative)

These are the invariants that keep the capability from drifting. New code MUST satisfy all of them.

### C1 — `crash()` is a test trigger only

The `crash` Mu command (`src/lib/ip/IPMu/CommandsModule.cpp`) exists solely to trigger a crash for
testing. It MUST contain only the fault that triggers the crash (a deliberate null dereference) and
MUST NOT contain any production logic such as annotation enrichment. Enrichment that needs to appear
in real crashes belongs in the live crash path (see C2), not in a test command. (This is the general
"Right Altitude for Logic" principle from `CLAUDE.md`.)

### C2 — Script context comes from a continuous trace hook, never a command

Interpreter execution context (which script/line/function is running) MUST be kept current by a
language-level trace hook that fires during normal execution, so it is accurate for *any* crash:

- Python: installed via `PyEval_SetTrace` (`src/lib/app/PyTwkApp/PyInterface.cpp`); line-level.
- Mu: `MuCrashObserver` (`src/lib/app/MuTwkApp`), registered as the interpreter's `ExecutionObserver`
  (`src/lib/mu/Mu/Mu/ExecutionObserver.h`). The interpreter invokes it around every function
  activation and at the external `Thread::call()` boundary, keeping `mu_function` / `mu_script_file`
  current. The Mu-core hook is dependency-free (no TwkUtil/Qt); the crash-specific logic lives in the
  app layer. A capability implemented for only one language, or only for the artificial `crash()`
  path, does not satisfy this contract.

  Mu fidelity is **function-level, not line-level**. Mu's `Node` hierarchy is non-polymorphic, so an
  arbitrary activation node cannot be safely identified as an `AnnotatedNode` (several activation paths
  — `dynamicActivation`, `invokeInterface`, tail-fuse continuations — pass plain `Node`s); casting them
  to read a source line dereferences out-of-bounds memory and crashes. The observer therefore tracks
  only the `Function` and derives the file from `Function::globalModule()->location()`, never casting
  execution nodes. The native (C++) stack in the minidump already pinpoints the faulting frame; the Mu
  function + file identify the script context. Recovering the Mu *line* safely (e.g. lazily via
  `Thread::backtrace()` at crash time, or a verified node discriminator) is possible future work.

### C3 — Every annotation key has exactly one table entry

`CrashHandler::addAnnotation(key, value)` only delivers a value to a live crash if `key` has a
matching entry in `g_annotationMappings[]` (`CrashHandler.cpp`). Keys without an entry are silently
dropped from runtime crashes.

Therefore:

- Every key passed to `addAnnotation()` anywhere in the codebase MUST have a corresponding
  `g_annotationMappings[]` entry (and backing `crashpad::Annotation` + buffer).
- `addAnnotation()` SHOULD warn (in debug builds) when called with an unmapped key, so this failure
  is never silent again.
- The annotation reference table (Section 6) MUST be kept in sync with the code.
- Annotation delivery MUST be init-order-independent: `addAnnotation()` calls made before
  `initialize()` are buffered (last value per key wins) and applied on successful init. This matters
  for context discovered during early startup — e.g. `gpu_vendor`/`gpu_renderer`, set during GL setup,
  which in `rvio` runs before crash-handler init.

### C4 — One shared crash-handler init path

All executables that initialize crash reporting (`rv`, the macOS `RV` bundle app, `rvio`, and any
future binary) MUST go through a single shared init helper. Per-executable copies of the init
sequence and handler-path strings are prohibited — they are how the three current entry points
drifted apart.

### C5 — Per-platform handler naming is fixed and used everywhere

The handler path is resolved from a single convention:

- macOS: `crashpad_handler_macos.sh`
- Linux: `crashpad_handler_linux.sh`
- Windows: `crashpad_handler.exe`

Every executable MUST use the wrapper (not the bare `crashpad_handler` binary) on macOS and Linux,
so the log redirection and the Linux `ulimit` safety cap always apply.

### C6 — Build guards are correct and symmetric

- The Crashpad handler/wrapper install MUST be guarded on the Crashpad version variable; Breakpad
  tooling MUST be guarded on the Breakpad version variable (see Section 2).
- Equivalent install rules MUST use equivalent guards across platforms — do not guard the macOS rule
  and leave the Linux rule unguarded (or vice versa).

### C7 — Version pinning lives in the defaults files

Dependency versions/commits MUST be pinned in `cmake/defaults/CYCOMMON.cmake` (platform-common) or
the relevant `CYxxxx.cmake`, following the existing Breakpad pattern — not hardcoded inside the
individual `dependencies/*.cmake` files.

## 6. Annotation reference

Annotations attached to every crash dump. This table is normative for contract C3 and MUST be
updated whenever an annotation is added, removed, or its delivery mechanism changes.

| Key | Meaning | Set by | Mechanism |
| --- | --- | --- | --- |
| `platform` | OS name | `initialize()` | Static (once at init) |
| `qt_version` | Qt version string | `initialize()` | Static (once at init) |
| `mu_script_file` / `mu_function` | Current Mu execution context (function-level) | `MuCrashObserver` (C2) | Dynamic, in mapping table |
| `py_script_file` / `py_script_line` / `py_function` | Current Python execution context | `PyEval_SetTrace` | Dynamic, in mapping table |
| `gpu_vendor` / `gpu_renderer` | GPU info | `ImageRenderer` | Dynamic, in mapping table |
| `python_caller` | Python frame calling into Mu | `PyMuSymbolType` | Dynamic, in mapping table |

Note — `mu_function` is the single key for the current Mu function, kept current for *all* Mu
execution by `MuCrashObserver` (including the Python→Mu path via `Thread::call()`). The earlier
`current_mu_function` key (a Python→Mu-only duplicate) has been retired. There is no `mu_script_line`
key: Mu context is function-level (see C2 for why line-level cannot be captured safely).

## 7. Symbolication workflow

1. At build time, `rv_generate_symbols.cmake` runs `dump_syms` on each staged binary/library
   (after `dsymutil` on macOS) and `organize_symbols.sh` places the `.sym` file under
   `stage/app/symbols/<module>/<id>/`. On Windows the MSVC build instead emits a `.pdb` beside each
   staged binary (native symbolication; no Breakpad `.sym`).
2. Symbols are NOT shipped in the customer package. The install `pre_install*.cmake` filters strip
   them from Release installs — the macOS/Linux `symbols/` tree and the Windows `.pdb` files. (Debug
   installs keep them for local debugging; the strip is gated on `CMAKE_INSTALL_CONFIG_NAME`.)
   Customers never symbolicate, so shipping symbols only bloats the package.
3. The `symbols_archive` build target (`rv_archive_symbols.cmake`) instead packages a versioned,
   per-platform `RV-<version>-<os>-<arch>-symbols.zip` under `stage/packages/`:
   - macOS/Linux: the Breakpad `symbols/` tree plus the symbolication tools (`minidump_stackwalk`,
     `minidump_dump`) and `symbolicate_crash.sh`, so the archive is self-contained.
   - Windows: the collected `.pdb` files (symbolicate with WinDbg/CDB).
   Run it after a full build: `cmake --build <build-dir> --target symbols_archive`. OpenRV only
   *produces* the archive; uploading it to a symbol store is the job of the external build/release
   pipeline — no such infrastructure is assumed in this repo (see the upstream rule in `CLAUDE.md`).
4. To symbolicate a customer-provided `.dmp` on macOS/Linux: unpack the matching archive and run
   `symbolicate_crash.sh --symbols <unpacked>/symbols <crash.dmp>`. The `--symbols` override points
   the script at the archived tree (its default is the now-unshipped script-relative `symbols/`);
   `minidump_stackwalk` is resolved next to the script.
5. `organize_symbols.sh` is a build-time tool only and is never shipped in the app.

## 8. Testing

Crash reporting is covered by OpenRV's doctest + CTest harness (run in CI on all platforms), plus a
manual flow for the parts that require a live display:

- `src/test/CrashHandlerTest` (doctest, cross-platform, runs in CI) unit-tests the `CrashHandler`
  public contract that needs no live handler: the singleton, the not-initialized state, and the
  pre-init annotation buffering (C3 / init-order independence). It does not start Crashpad.
- `src/test/CrashDumpSmokeTest` launches RV with the test-only `crash()` command (C1) and asserts a
  minidump is produced — exercising handler init (C4/C5) and capture end-to-end. It is enabled by
  default on every platform that builds the Crashpad handler (toggle with
  `RV_BUILD_CRASH_DUMP_SMOKE_TEST`). Where Breakpad's `minidump_dump` is available (macOS/Linux) it
  also asserts the `platform` annotation; on Windows (no `minidump_dump`) it only verifies a dump was
  produced. It runs headless (`QT_QPA_PLATFORM=offscreen`); under offscreen Qt the Mu
  execution-context annotation (`mu_function`, C2) is not populated, so it is reported for
  information only.
- Manual (display-backed) verification of the execution-context annotations (C2): trigger `crash()`
  from inside a Mu/Python function (e.g. `rv -eval 'function: f (void;) { crash(); } f();'`), then
  symbolicate the dump with `symbolicate_crash.sh` or inspect it with `minidump_dump`, and confirm
  the expected `mu_function` / `mu_script_file` / `py_*` values plus a symbolicated native stack.

## 9. Known gaps (current divergences from this spec)

These are the outstanding items from the convergence audit. Each references the contract it
violates. This list is a remediation tracker, not part of the permanent design.

- [x] **A — `crash()` carried production logic** (violated C1/C2). Fixed: added a dependency-free
      `ExecutionObserver` hook in the Mu core (`functionActivationFunc` + `Thread::call`) with the
      crash-specific `MuCrashObserver` registered from MuTwkApp; `crash()` is now a bare null-deref.
      Fidelity is function-level (`mu_function` + `mu_script_file`); line-level was dropped because
      Mu's non-polymorphic `Node` makes recovering a call-site line from an arbitrary activation node
      unsafe — see C2. Safe line recovery is possible future work.
- [x] **A2 — `mu_function` / `current_mu_function` redundancy** (tied to fix A). Fixed: the observer
      keeps `mu_function` current for all Mu execution, so `current_mu_function` was retired and
      `PyMuSymbolType` now contributes only `python_caller`.
- [x] **B1 — `current_mu_function`, `python_caller` dropped** (violated C3). Fixed: added to
      `g_annotationMappings[]`; `addAnnotation()` now warns (debug builds) on any unmapped key.
- [x] **B3 — redundant post-init `platform`/`qt_version` writes** removed from both `main.cpp`
      entry points (values are set correctly at init via `StartHandler`).
- [x] **C1 — three diverging init paths** for `rv` / `RV` / `rvio` (violated C4). Fixed: all three
      now call one shared helper, `TwkApp::initializeCrashHandler(appName, version, executableDir)`
      (`src/lib/app/MuTwkApp/CrashHandlerInit.{h,cpp}`), which resolves the handler, initializes, and
      enables Mu debugging. The per-`main.cpp` `#ifdef` path blocks are gone.
- [x] **C2 — `rvio` used the bare `crashpad_handler`** (violated C5) and never called
      `setDebugging(true)`. Fixed by C1's shared helper: every binary now uses the platform wrapper
      (`crashpad_handler_{macos,linux}.sh` / `.exe`) and enables Mu debugging. The leftover
      `rvio` `addAnnotation("platform", …)` dead write (B3) was removed in the same change.
- [x] **D1 — Crashpad wrapper install guarded on `RV_DEPS_BREAKPAD_VERSION`** in
      `src/bin/nsapps/RV/CMakeLists.txt` (violated C6). Fixed: split by tool ownership — the Crashpad
      wrapper is now guarded on `RV_DEPS_CRASHPAD_VERSION`, `symbolicate_crash.sh` on the Breakpad var.
- [x] **D2 — guard asymmetry**: Linux install block in `src/bin/apps/rv/CMakeLists.txt` was unguarded
      (violated C6). Fixed: mirrored the macOS structure with matching Breakpad/Crashpad guards.
- [x] **D3 — `dump_syms` missing from Linux `BUILD_BYPRODUCTS`** in `breakpad.cmake` (Ninja race).
      Fixed: added `${_dump_syms_tool}` to the Linux ExternalProject byproducts (macOS already listed it).
- [x] **D4 — Windows Crashpad staging used a `POST_BUILD` + `DEPENDS`-by-path anti-pattern** in
      `crashpad.cmake` (governed by C6). Fixed: the Windows staging block now mirrors the non-Windows
      `ELSE()` branch — a single `ADD_CUSTOM_COMMAND(OUTPUT <staged crashpad_client.lib> <staged
      crashpad_handler.exe> ... DEPENDS ${_target})` producer with the `ADD_CUSTOM_TARGET ... ALL
      DEPENDS` consuming those OUTPUT files. The Windows-only `editbin /SUBSYSTEM:WINDOWS` step on the
      staged handler moved into that OUTPUT-based command. Verified on a Windows host: configure
      resolves `RV_DEPS_CRASHPAD_GIT_TAG` from `CYCOMMON.cmake`; building the stage target stages
      `crashpad_handler.exe` and `crashpad_client.lib` and applies `editbin` (the staged handler
      differs byte-for-byte from the install copy, the `editbin` rewrite signature).
- [x] **D5 — Crashpad commit pinned inside `crashpad.cmake`** rather than `CYCOMMON.cmake`
      (violated C7). Fixed: `RV_DEPS_CRASHPAD_GIT_TAG` now lives in `CYCOMMON.cmake` next to the
      Breakpad pin; `crashpad.cmake` just consumes it. Verified configure resolves the tag.
- [x] **E1 — `symbolicate_crash.sh` used GNU `sed -i`** (which fails on macOS BSD `sed`, where the
      script ships — verified: old idiom errors, new one works). Fixed: portable temp-file rewrite
      (`sed ... > tmp && mv tmp out`), no `-i`.
- [x] **E2 — stale `TweakSoftware` path** in the help text. Fixed: now `~/Library/Logs/ASWF/...`.
- [x] **E3 — hardcoded `_build_debug`** in the `minidump_dump` dev-fallback search. Fixed: globbed
      `_build*` so debug/release build dirs match; shipping path (`${SCRIPT_DIR}/minidump_dump`) is tried first.
- [x] **E4 — `organize_symbols.sh` missing execute bit**. Fixed: mode now `100755`, matching `symbolicate_crash.sh`.
- [x] **G — GPU annotations dropped when set before init** (violated C3). Found during C verification:
      `gpu_vendor`/`gpu_renderer` are set during GL setup, which in `rvio` runs before crash-handler
      init, so they were silently dropped (with a warning). Fixed: `CrashHandler::addAnnotation()` now
      buffers pre-init annotations and `initialize()` flushes them; delivery is init-order-independent.

## 10. References

- Handler: `src/lib/base/TwkUtil/CrashHandler.{h,cpp}`, `CrashHandler_internal.h`
- Test command: `src/lib/ip/IPMu/CommandsModule.cpp` (`crash`)
- Python trace hook (reference for C2): `src/lib/app/PyTwkApp/PyInterface.cpp`
- Wrappers: `src/bin/apps/rv/crashpad_handler_{linux,macos}.sh.in`
- Build: `cmake/dependencies/{crashpad,breakpad}.cmake`, `cmake/macros/rv_generate_symbols.cmake`,
  `cmake/defaults/CYCOMMON.cmake`
- Tooling: `src/bin/apps/rv/{symbolicate_crash,organize_symbols}.sh`
- Entry points: `src/bin/apps/rv/main.cpp`, `src/bin/nsapps/RV/main.cpp`,
  `src/bin/imgtools/rvio/main.cpp`
