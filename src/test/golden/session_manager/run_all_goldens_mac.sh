#!/usr/bin/env bash
# Run session_manager golden scenarios against golden-mac/ (macOS native display).
#
# Usage:
#   ./run_all_goldens_mac.sh              # verify Python port (default)
#   IMPL=mu ./run_all_goldens_mac.sh      # Mu determinism check
#   ./run_all_goldens_mac.sh sm_select_node  # single scenario
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
DMAX="${DMAX:-0}"
GATE="${GATE:-both}"

export SM_MERIDIAN_DIR="${SM_MERIDIAN_DIR:-/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001}"

# Not part of the per-gate suite: 83 clips x 2 rvio jobs at MAX_WORKERS=2 would be
# paid once per gate. run_folder_thumbnails_all.sh runs it after the gates pass.
# Still runnable by naming it explicitly.
GATE_EXCLUDED_IDS="${GATE_EXCLUDED_IDS:-sm_folder_thumbnails_all}"

all_required_ids() {
    find "$SCENARIOS" -maxdepth 1 -name '*.py' ! -name '_*.py' -exec basename {} \; \
        | sed 's/\.py$//' | sort
}

is_gate_excluded() {
    for skip in $GATE_EXCLUDED_IDS; do
        [ "$1" = "$skip" ] && return 0
    done
    return 1
}

ids=("$@")
if [ ${#ids[@]} -eq 0 ]; then
    ids=()
    while IFS= read -r id; do
        is_gate_excluded "$id" && continue
        ids+=("$id")
    done < <(all_required_ids)
fi

pass=0
fail=0
fail_list=""

if [ "$GATE" = "default" ]; then
    IMPL=default
fi

gate_note="behavioral+pixel dmax=$DMAX"
case "$GATE" in
    behavioral) gate_note="behavioral-only" ;;
    pixel)      gate_note="pixel-only dmax=$DMAX" ;;
    default)    gate_note="default-launch behavioral-only" ;;
    runtime)    gate_note="runtime delta vs Mu golden (runtime_errors.txt)" ;;
esac

echo "session_manager goldens (Mac): impl=$IMPL gate=$GATE ($gate_note) timeout=${TIMEOUT}s (${#ids[@]} scenarios)"

for id in "${ids[@]}"; do
    golden_dir="$GOLDEN/$id"
    scenario="$SCENARIOS/${id}.py"
    out="/tmp/golden_sm_${id}"
    if [ ! -f "$golden_dir/session.rv" ]; then
        echo "FAIL $id (no golden-mac baseline — run capture_golden_mac.sh $id)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    rm -rf "$out"
    mkdir -p "$out"
    runtime_golden_flag=(--runtime-golden-dir "$golden_dir")
    if ! python3 "$RUNNER" \
        --scenario "$scenario" --out "$out" --rv "$RV" \
        --impl "$IMPL" --no-xvfb --timeout "$TIMEOUT" \
        "${runtime_golden_flag[@]}" >/dev/null 2>&1; then
        if [ "$GATE" = "runtime" ]; then
            echo "FAIL $id (runtime — new errors vs golden; see $out/runtime_errors.txt)"
        else
            echo "FAIL $id (run_scenario)"
        fi
        fail=$((fail + 1)); fail_list="$fail_list $id"
        pkill -f "${RV} " 2>/dev/null || true
        sleep 0.3
        continue
    fi
    if [ "$GATE" = "runtime" ]; then
        echo "PASS $id"
        pass=$((pass + 1))
        pkill -f "${RV} " 2>/dev/null || true
        sleep 0.3
        continue
    fi
    compare_rc=0
    compare_out="$(python3 "$COMPARE" \
        --golden-dir "$golden_dir" --actual-dir "$out" --dmax "$DMAX" 2>&1)" || compare_rc=$?
    compare_rc=${compare_rc:-0}
    behavioral_ok=0
    pixel_ok=0
    if echo "$compare_out" | grep -q "^behavioral: MATCH"; then
        behavioral_ok=1
    fi
    if echo "$compare_out" | grep -q "pixel: MATCH"; then
        pixel_ok=1
    fi
    has_png_golden=0
    if compgen -G "$golden_dir/*.png" >/dev/null 2>&1; then
        has_png_golden=1
    fi
    case "$GATE" in
        behavioral|default)
            if [ "$behavioral_ok" -eq 1 ]; then
                echo "PASS $id"
                pass=$((pass + 1))
            else
                echo "FAIL $id (behavioral)"
                echo "$compare_out" | head -10
                fail=$((fail + 1)); fail_list="$fail_list $id"
            fi
            ;;
        pixel)
            # A scenario with no PNG baseline is a hole in the pixel gate, not a
            # pass: every scenario must pin its outcome in pixels.
            if [ "$has_png_golden" -eq 0 ]; then
                echo "FAIL $id (no PNG baseline — scenario must capture at least one image)"
                fail=$((fail + 1)); fail_list="$fail_list $id"
            elif [ "$pixel_ok" -eq 1 ]; then
                echo "PASS $id"
                pass=$((pass + 1))
            else
                echo "FAIL $id (pixel)"
                echo "$compare_out" | head -10
                fail=$((fail + 1)); fail_list="$fail_list $id"
            fi
            ;;
        both)
            if [ "$compare_rc" -eq 0 ]; then
                echo "PASS $id"
                pass=$((pass + 1))
            else
                echo "FAIL $id (compare)"
                echo "$compare_out" | head -10
                fail=$((fail + 1)); fail_list="$fail_list $id"
            fi
            ;;
    esac
    pkill -f "${RV} " 2>/dev/null || true
    sleep 0.3
done

echo "--- PASS=$pass FAIL=$fail"
if [ "$fail" -gt 0 ]; then
    echo "Failed:$fail_list"
    exit 1
fi
