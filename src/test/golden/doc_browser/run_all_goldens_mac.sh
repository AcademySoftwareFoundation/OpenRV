#!/usr/bin/env bash
# Run doc_browser golden scenarios against golden-mac/ (macOS native display).
#
# Usage:
#   ./run_all_goldens_mac.sh              # verify Python port (default)
#   IMPL=mu ./run_all_goldens_mac.sh      # Mu determinism check
#   ./run_all_goldens_mac.sh db_activate  # single scenario
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

echo "doc_browser goldens (Mac): impl=$IMPL gate=$GATE ($gate_note) timeout=${TIMEOUT}s (${#ids[@]} scenarios)"

for id in "${ids[@]}"; do
    golden_dir="$GOLDEN/$id"
    scenario="$SCENARIOS/${id}.py"
    out="/tmp/golden_${id}"
    if [ ! -f "$golden_dir/session.rv" ]; then
        echo "FAIL $id (no golden-mac baseline — run capture_golden_mac.sh $id)"
        fail=$((fail + 1)); fail_list="$fail_list $id"
        continue
    fi
    rm -rf "$out"
    mkdir -p "$out"
    read -ra RUNNER_EXTRA <<< "$(runner_extra_flags "$id")"
    runtime_golden_flag=(--runtime-golden-dir "$golden_dir")
    if ! python3 "$RUNNER" \
        --scenario "$scenario" --out "$out" --rv "$RV" \
        --impl "$IMPL" --no-xvfb --timeout "$TIMEOUT" \
        "${RUNNER_EXTRA[@]}" \
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
            if [ "$has_png_golden" -eq 0 ]; then
                echo "PASS $id (no PNG baseline)"
                pass=$((pass + 1))
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
