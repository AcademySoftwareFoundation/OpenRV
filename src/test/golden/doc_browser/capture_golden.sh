#!/usr/bin/env bash
# Capture Mu baselines for doc_browser into golden/<id>/ (Linux Xvfb).
#
# Usage:
#   ./capture_golden.sh [scenario_id ...]
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG="$HERE"
REPO_ROOT="$(cd "$PKG/../../../.." && pwd)"
RUNNER="$REPO_ROOT/src/test/golden/harness/run_scenario.py"
RUNTIME_CHECK="$REPO_ROOT/src/test/golden/harness/runtime_log_check.py"
RMS_IMAGE_DIFF="${RMS_IMAGE_DIFF:-$REPO_ROOT/_build/stage/app/bin/rmsImageDiff}"
RV="${RV:-$REPO_ROOT/_build/stage/app/bin/rv}"
SCENARIOS="$PKG/scenarios"
GOLDEN="$PKG/golden"
TIMEOUT="${TIMEOUT:-600}"

DB_MODE="${DB_MODE:-doc_browser}"
HELP_MODE="${HELP_MODE:-help}"

runner_extra_flags() {
    local id="$1"
    if [ "$id" = "db_activate_help" ]; then
        echo "--mode" "${DB_MODE},${HELP_MODE}" "--menu-bar"
    else
        echo "--mode" "$DB_MODE"
    fi
}

all_scenario_ids() {
    find "$SCENARIOS" -maxdepth 1 -name '*.py' ! -name '_db_common.py' -exec basename {} \; \
        | sed 's/\.py$//' | sort
}

ids=("$@")
if [ ${#ids[@]} -eq 0 ]; then
    while IFS= read -r id; do
        [ -f "$GOLDEN/$id/session.rv" ] && continue
        ids+=("$id")
    done < <(all_scenario_ids)
fi

if [ ${#ids[@]} -eq 0 ]; then
    echo "Nothing to capture (all golden baselines present)."
    exit 0
fi

pngs_identical() {
    local a="$1" b="$2"
    local out
    out="$("$RMS_IMAGE_DIFF" -m "$a" "$b" 2>&1)" || {
        echo "rmsImageDiff failed: $out"
        return 1
    }
    if echo "$out" | grep -q "max diff at"; then
        echo "$out" | grep "max diff at"
        return 1
    fi
    return 0
}

echo "Capturing ${#ids[@]} doc_browser scenario(s) --impl mu under Xvfb, 2x determinism"

fail=0
fail_list=""

for id in "${ids[@]}"; do
    read -ra RUNNER_EXTRA <<< "$(runner_extra_flags "$id")"
    scenario="$SCENARIOS/${id}.py"
    if [ ! -f "$scenario" ]; then
        echo "ERROR: missing scenario $scenario" >&2
        exit 2
    fi
    out1="/tmp/golden_capture_${id}_a"
    out2="/tmp/golden_capture_${id}_b"
    dest="$GOLDEN/$id"
    echo "==> $id (run 1/2)"
    rm -rf "$out1"
    mkdir -p "$out1"
    if ! python3 "$RUNNER" \
        --scenario "$scenario" --out "$out1" --rv "$RV" \
        --impl mu --timeout "$TIMEOUT" \
        --allow-runtime-errors \
        "${RUNNER_EXTRA[@]}"; then
        echo "FAIL $id (run 1)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    if [ ! -f "$out1/session.rv" ]; then
        echo "FAIL $id (no session.rv run 1)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    echo "==> $id (run 2/2)"
    rm -rf "$out2"
    mkdir -p "$out2"
    if ! python3 "$RUNNER" \
        --scenario "$scenario" --out "$out2" --rv "$RV" \
        --impl mu --timeout "$TIMEOUT" \
        --allow-runtime-errors \
        "${RUNNER_EXTRA[@]}"; then
        echo "FAIL $id (run 2)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    if [ ! -f "$out2/session.rv" ]; then
        echo "FAIL $id (no session.rv run 2)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi

    det_ok=1
    if ! diff -q "$out1/session.rv" "$out2/session.rv" >/dev/null 2>&1; then
        echo "FAIL $id: session.rv not deterministic"
        det_ok=0
    fi
    tmp1="$(mktemp)" tmp2="$(mktemp)"
    python3 "$RUNTIME_CHECK" "$out1" --write-baseline "$tmp1" >/dev/null
    python3 "$RUNTIME_CHECK" "$out2" --write-baseline "$tmp2" >/dev/null
    if ! diff -q "$tmp1" "$tmp2" >/dev/null 2>&1; then
        echo "FAIL $id: runtime_errors.txt not deterministic"
        det_ok=0
    fi
    rm -f "$tmp1" "$tmp2"
    for png in "$out1"/*.png; do
        [ -f "$png" ] || continue
        name="$(basename "$png")"
        if [ ! -f "$out2/$name" ]; then
            echo "FAIL $id: $name missing run 2"
            det_ok=0
            continue
        fi
        if ! pngs_identical "$png" "$out2/$name"; then
            echo "FAIL $id: $name not deterministic"
            det_ok=0
        fi
    done
    if [ "$det_ok" -ne 1 ]; then
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi

    rm -rf "$dest"
    mkdir -p "$dest"
    cp "$out1/session.rv" "$dest/"
    python3 "$RUNTIME_CHECK" "$out1" --write-baseline "$dest/runtime_errors.txt"
    for png in "$out1"/*.png; do
        [ -f "$png" ] || continue
        cp "$png" "$dest/"
    done
    echo "    -> $dest ($(ls "$dest" | tr '\n' ' '))"
done

echo "---"
if [ "$fail" -gt 0 ]; then
    echo "FAILED:$fail_list"
    exit 1
fi
echo "Capture complete."
