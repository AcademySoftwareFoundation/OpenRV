#!/usr/bin/env bash
# Gate 5 — run Python unit tests for a migrated package.
#
# Usage (from run_migration_loop*.sh):
#   GOLDEN_PKG_DIR="$HERE" source .../run_unit_tests.sh
# or:
#   GOLDEN_PKG_DIR=src/test/golden/<package> src/test/golden/harness/run_unit_tests.sh
#
# Interpreter: the tests import the ported modules for real, and those import
# PySide6, so they have to run under an interpreter that has the same PySide6 the
# port runs against — in practice the one RV bundles. A stock python3 normally has
# no PySide6, and letting the run continue there would turn gate 5 into a pass made
# entirely of skips, which is no gate at all. Override with GOLDEN_PYTHON=<path>.
#
set -euo pipefail

_PKG_DIR="${GOLDEN_PKG_DIR:-$(pwd)}"
_UNIT_DIR="$_PKG_DIR/unit"
_HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_REPO_ROOT="$(cd "$_HERE/../../../.." && pwd)"

if [ ! -d "$_UNIT_DIR" ]; then
    echo "GATE 5 FAILED: missing unit test dir $_UNIT_DIR"
    echo "Create unit/ with test_*.py — see VERIFICATION.md Gate 5."
    exit 1
fi

shopt -s nullglob
_tests=( "$_UNIT_DIR"/test_*.py )
shopt -u nullglob

if [ "${#_tests[@]}" -eq 0 ]; then
    echo "GATE 5 FAILED: no test_*.py files in $_UNIT_DIR"
    exit 1
fi

_has_pyside6() {
    "$1" -c "import PySide6" >/dev/null 2>&1
}

_PY=""
if [ -n "${GOLDEN_PYTHON:-}" ]; then
    # The override is checked too. Pointing it at an interpreter without PySide6
    # would make every module skip itself and the gate pass on zero tests.
    if ! _has_pyside6 "$GOLDEN_PYTHON"; then
        echo "GATE 5 FAILED: GOLDEN_PYTHON=$GOLDEN_PYTHON has no PySide6."
        echo "The unit tests import the ported modules, which import PySide6."
        exit 1
    fi
    _PY="$GOLDEN_PYTHON"
else
    for _cand in \
        "$_REPO_ROOT/_build/stage/app/RV.app/Contents/MacOS/python3" \
        "$_REPO_ROOT/_build/stage/app/bin/python3" \
        "$(command -v python3 || true)"
    do
        [ -n "$_cand" ] && [ -x "$_cand" ] || continue
        if _has_pyside6 "$_cand"; then
            _PY="$_cand"
            break
        fi
    done
fi

if [ -z "$_PY" ]; then
    echo "GATE 5 FAILED: no interpreter with PySide6 found."
    echo "The unit tests import the ported modules, which import PySide6."
    echo "Build the staged app, or set GOLDEN_PYTHON=<python with PySide6>."
    exit 1
fi

echo "Gate 5: running ${#_tests[@]} unit test module(s) in $_UNIT_DIR"
echo "Gate 5: interpreter $_PY"

# Headless: the tests build real widgets, so Qt needs a platform plugin that does
# not require a display. "offscreen" rather than "minimal" — minimal has no window
# surface, and QDialog.show() segfaults on it, which would put the Create Image and
# New Node by Type dialogs out of reach of gate 5 for no good reason.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

_run_and_check() {
    # A run whose every test was skipped is not a pass either: skips are legitimate
    # per test, but a suite that never reached the port has proved nothing. So this
    # checks the exit status AND that a positive number of tests actually ran.
    #
    # The output is captured rather than streamed so it can be inspected, which
    # needs care under `set -e`: a bare `_out="$(...)"` of a failing command aborts
    # the script immediately and the diagnostics never get printed. `|| _rc=$?`
    # keeps the failure in-band and preserves the real status — note `if ! cmd`
    # would not, because the negation makes $? read 0 inside the branch.
    local _out _rc=0

    _out="$("$@" 2>&1)" || _rc=$?

    echo "$_out"

    if [ "$_rc" -ne 0 ]; then
        echo "GATE 5 FAILED: the test run exited $_rc (see above)."
        return "$_rc"
    fi

    # pytest: "N passed", possibly with skips; unittest: "Ran N tests".
    local _ran
    _ran="$(printf '%s\n' "$_out" \
        | grep -oE '[0-9]+ passed|^Ran [0-9]+ test' \
        | grep -oE '[0-9]+' \
        | tail -1)"

    if [ -z "$_ran" ] || [ "$_ran" -eq 0 ]; then
        echo "GATE 5 FAILED: no tests actually ran (all skipped, or none collected)."
        echo "A suite that skips itself reports success as loudly as one that works."
        return 1
    fi

    echo "Gate 5: $_ran test(s) executed."
    return 0
}

if "$_PY" -m pytest --version >/dev/null 2>&1; then
    _run_and_check "$_PY" -m pytest "$_UNIT_DIR" -q "$@"
else
    echo "(pytest not found — falling back to unittest discover)"
    _run_and_check "$_PY" -m unittest discover -s "$_UNIT_DIR" -p "test_*.py" -v
fi
