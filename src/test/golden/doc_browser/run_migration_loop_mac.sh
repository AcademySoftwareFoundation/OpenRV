#!/usr/bin/env bash
# Full migration loop — doc_browser (macOS, golden-mac/).
#
# Usage:
#   ./run_migration_loop_mac.sh
#   SKIP_SANITY=1 ./run_migration_loop_mac.sh
#   SKIP_REVIEW=1 ./run_migration_loop_mac.sh
#
set -euo pipefail

if [ -z "${CAFFEINATED:-}" ]; then
    export CAFFEINATED=1
    exec caffeinate -d -i "$0" "$@"
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HERE/../../../.." && pwd)"
GOLDEN_PKG_DIR="$HERE"
# shellcheck disable=SC1091
source "$REPO_ROOT/src/test/golden/harness/migration_loop_agent_reminder.sh"
SKIP_SANITY="${SKIP_SANITY:-0}"
SKIP_REVIEW="${SKIP_REVIEW:-0}"

echo "=============================================="
echo "doc_browser migration loop (Mac)"
echo "See: $REPO_ROOT/src/test/golden/VERIFICATION.md"
echo "=============================================="

echo
echo ">>> GATE 0 (MANDATORY): Runtime clean"
if ! GATE=runtime IMPL=python "$HERE/run_all_goldens_mac.sh"; then
    echo "GATE 0 FAILED"
    exit 1
fi

echo
echo ">>> GATE 1 (MANDATORY): Behavioral"
if ! GATE=behavioral IMPL=python "$HERE/run_all_goldens_mac.sh"; then
    echo "GATE 1 FAILED"
    exit 1
fi

echo ">>> GATE 2 (MANDATORY): Pixel"
if ! GATE=pixel IMPL=python "$HERE/run_all_goldens_mac.sh"; then
    if [ "${SKIP_PIXEL_GATE:-0}" = "1" ]; then
        echo "GATE 2 FAILED — SKIP_PIXEL_GATE=1, continuing"
    else
        echo "GATE 2 FAILED"
        exit 1
    fi
else
    echo "GATE 2 PASSED"
fi

if [ "${SKIP_PIXEL_GATE:-0}" != "1" ]; then
    if [ "$SKIP_SANITY" = "1" ]; then
        echo "SKIP sanity (SKIP_SANITY=1)"
    else
        echo
        echo ">>> SANITY (conditional): GUI real-display gate"
        if ! "$HERE/run_gui_sanity_gate.sh"; then
            echo "SANITY FAILED (behavioral)"
            exit 1
        fi
    fi

    if [ "$SKIP_REVIEW" = "1" ]; then
        echo "SKIP review agent (SKIP_REVIEW=1)"
    else
        echo
        echo ">>> REVIEW AGENT (conditional)"
        PARENT="$(git -C "$REPO_ROOT" rev-parse HEAD~1 2>/dev/null || echo "<parent>")"
        HEAD="$(git -C "$REPO_ROOT" rev-parse HEAD 2>/dev/null || echo "<head>")"
        echo "Review diff: $PARENT..$HEAD"
        echo "Scope: src/plugins/rv-packages/doc_browser/*.py"
        git -C "$REPO_ROOT" diff --name-only "$PARENT" HEAD -- \
            'src/plugins/rv-packages/doc_browser/*.py' 2>/dev/null || true
    fi
fi

echo
echo ">>> GATE 3 (MANDATORY): Default launch"
if ! GATE=default "$HERE/run_all_goldens_mac.sh"; then
    echo "GATE 3 FAILED"
    exit 1
fi

echo
echo ">>> GATE 4 (MANDATORY): Mu baseline integrity"
if ! GATE=both IMPL=mu "$HERE/run_all_goldens_mac.sh"; then
    echo "GATE 4 FAILED"
    exit 1
fi

echo
echo ">>> GATE 5 (MANDATORY): Python unit tests"
if ! GOLDEN_PKG_DIR="$HERE" "$REPO_ROOT/src/test/golden/harness/run_unit_tests.sh"; then
    echo "GATE 5 FAILED"
    exit 1
fi

echo
echo "=============================================="
echo "MIGRATION LOOP PASSED"
echo "Confirm: sanity pixel review + code review (no blocking findings)."
echo "Update COVERAGE.md; ask user about removing doc_browser.mu."
echo "=============================================="
