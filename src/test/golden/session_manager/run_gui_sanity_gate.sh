#!/usr/bin/env bash
# GUI sanity gate for session_manager — real display, behavioral hard, pixel report-only.
#
# Known limitation: sm_meridian_mp4_load, sm_media_add_sources, sm_mp4_all use
# addSources() + waitForProgressiveLoading() which hangs forever under a real display
# (confirmed 2026-07-24 on macOS native). Skipped here only; all three have golden
# baselines and run in run_all_goldens_mac.sh under --no-xvfb.
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

export SM_MERIDIAN_DIR="${SM_MERIDIAN_DIR:-/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001}"

# The mp4 scenarios used to be skipped here: they called
# rvc.waitForProgressiveLoading(), which deadlocks on a real display because the
# loader needs the event loop the blocking wait stops. The helpers now poll
# loadTotal() while pumping and load folders with addSourceVerbose, so every mp4
# scenario runs under this gate (verified 2026-08-03: sm_meridian_mp4_load 4.9s).
# Only the full-folder scenario stays out, on cost grounds.
SKIP_IDS="${SKIP_IDS:-sm_folder_thumbnails_all}"

should_skip() {
    local id="$1"
    for skip in $SKIP_IDS; do
        [ "$id" = "$skip" ] && return 0
    done
    return 1
}

all_required_ids() {
    find "$SCENARIOS" -maxdepth 1 -name '*.py' ! -name '_*.py' -exec basename {} \; \
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
skipped=0

echo "session_manager GUI sanity: impl=$IMPL real-display (${#ids[@]} scenarios)"
echo "Skipping (covered by run_folder_thumbnails_all.sh after the gates): $SKIP_IDS"

for id in "${ids[@]}"; do
    if should_skip "$id"; then
        echo "SKIP $id (run separately by run_folder_thumbnails_all.sh)"
        skipped=$((skipped + 1))
        continue
    fi
    golden_dir="$GOLDEN/$id"
    scenario="$SCENARIOS/${id}.py"
    out="/tmp/sm_gui_sanity_${id}"
    if [ ! -f "$golden_dir/session.rv" ]; then
        echo "FAIL $id (no golden-mac baseline)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    rm -rf "$out"
    mkdir -p "$out"
    if ! python3 "$RUNNER" \
        --scenario "$scenario" --out "$out" --rv "$RV" \
        --impl "$IMPL" --no-xvfb --timeout "$TIMEOUT" \
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
echo "PASS=$pass FAIL=$fail SKIP=$skipped"
if [ -n "$review_ids" ]; then
    echo "NEEDS_AI_REVIEW:$review_ids"
fi

if [ "$fail" -gt 0 ]; then
    echo "Failed:$fail_list"
    exit 1
fi
