#!/usr/bin/env bash
# GUI sanity gate for doc_browser — real display, behavioral hard, pixel report-only.
#
set -euo pipefail

if [ -z "${CAFFEINATED:-}" ]; then
    export CAFFEINATED=1
    exec caffeinate -d -i "$0" "$@"
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG="$HERE"
REPO_ROOT="$(cd "$PKG/../../../.." && pwd)"
RUNNER="$REPO_ROOT/src/test/golden/harness/run_scenario.py"
COMPARE="$REPO_ROOT/src/test/golden/harness/compare.py"
RV="${RV:-$REPO_ROOT/_build/stage/app/RV.app/Contents/MacOS/RV}"
SCENARIOS="$PKG/scenarios"
GOLDEN="$PKG/golden-mac"
IMPL="${IMPL:-python}"
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

all_required_ids() {
    find "$SCENARIOS" -maxdepth 1 -name '*.py' ! -name '_db_common.py' -exec basename {} \; \
        | sed 's/\.py$//' | sort
}

ids=("$@")
if [ ${#ids[@]} -eq 0 ]; then
    ids=()
    while IFS= read -r id; do
        ids+=("$id")
    done < <(all_required_ids)
fi

pass=0
fail=0
fail_list=""
review_ids=""

echo "doc_browser GUI sanity: impl=$IMPL real-display (${#ids[@]} scenarios)"

for id in "${ids[@]}"; do
    golden_dir="$GOLDEN/$id"
    scenario="$SCENARIOS/${id}.py"
    out="/tmp/db_gui_sanity_${id}"
    if [ ! -f "$golden_dir/session.rv" ]; then
        echo "FAIL $id (no golden-mac baseline)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    rm -rf "$out"
    mkdir -p "$out"
    read -ra RUNNER_EXTRA <<< "$(runner_extra_flags "$id")"
    if ! python3 "$RUNNER" \
        --scenario "$scenario" --out "$out" --rv "$RV" \
        --impl "$IMPL" --no-xvfb --timeout "$TIMEOUT" \
        "${RUNNER_EXTRA[@]}" \
        --runtime-golden-dir "$golden_dir" >/dev/null 2>&1; then
        echo "FAIL $id (run_scenario)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    compare_rc=0
    compare_out="$(python3 "$COMPARE" \
        --golden-dir "$golden_dir" \
        --behavioral-golden-dir "$golden_dir" \
        --actual-dir "$out" \
        --pixel-mode report 2>&1)" || compare_rc=$?
    if [ "$compare_rc" -ne 0 ]; then
        echo "FAIL $id (behavioral mismatch or missing artifact)"
        echo "$compare_out" | head -8
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    if echo "$compare_out" | grep -q "pixel: INFO"; then
        review_ids="$review_ids $id"
        echo "PASS $id (behavioral OK — pixel report needs review)"
    else
        echo "PASS $id"
    fi
    pass=$((pass + 1))
done

echo "---"
echo "PASS=$pass FAIL=$fail"
if [ -n "$review_ids" ]; then
    echo "NEEDS_AI_REVIEW:$review_ids"
fi

if [ "$fail" -gt 0 ]; then
    echo "Failed:$fail_list"
    exit 1
fi
