#!/usr/bin/env bash
# Full migration loop — doc_browser (Linux, golden/).
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HERE/../../../.." && pwd)"
# shellcheck disable=SC1091
source "$REPO_ROOT/src/test/golden/harness/migration_loop_agent_reminder.sh"
SKIP_SANITY="${SKIP_SANITY:-0}"
SKIP_REVIEW="${SKIP_REVIEW:-0}"

echo "=============================================="
echo "doc_browser migration loop (Linux)"
echo "=============================================="

echo
echo ">>> GATE 0 (MANDATORY): Runtime clean"
if ! GATE=runtime IMPL=python "$HERE/run_all_goldens.sh"; then
    echo "GATE 0 FAILED"
    exit 1
fi

echo
echo ">>> GATE 1 (MANDATORY): Behavioral"
if ! GATE=behavioral IMPL=python "$HERE/run_all_goldens.sh"; then
    echo "GATE 1 FAILED"
    exit 1
fi

echo ">>> GATE 2 (MANDATORY): Pixel"
if ! GATE=pixel IMPL=python "$HERE/run_all_goldens.sh"; then
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
    if [ "$SKIP_SANITY" != "1" ]; then
        echo "NOTE: GUI sanity is macOS-only for doc_browser; use run_gui_sanity_gate.sh on Mac."
    fi
    if [ "$SKIP_REVIEW" != "1" ]; then
        echo ">>> REVIEW AGENT (conditional)"
    fi
fi

echo
echo ">>> GATE 3 (MANDATORY): Default launch"
if ! GATE=default "$HERE/run_all_goldens.sh"; then
    echo "GATE 3 FAILED"
    exit 1
fi

echo
echo ">>> GATE 4 (MANDATORY): Mu baseline integrity"
if ! GATE=both IMPL=mu "$HERE/run_all_goldens.sh"; then
    echo "GATE 4 FAILED"
    exit 1
fi

echo
echo ">>> GATE 5 (MANDATORY): Python unit tests"
if ! GOLDEN_PKG_DIR="$HERE" "$REPO_ROOT/src/test/golden/harness/run_unit_tests.sh"; then
    echo "GATE 5 FAILED"
    exit 1
fi

echo "MIGRATION LOOP PASSED (Linux)"
