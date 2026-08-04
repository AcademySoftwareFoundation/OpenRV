#!/usr/bin/env bash
# Full-folder thumbnail check: load EVERY clip in SM_MERIDIAN_DIR (83 at the time
# of writing), wait for all thumbnails and filmstrips to be generated, and compare
# the panel against the committed baseline pixel-for-pixel.
#
# Kept out of the per-gate suite (GATE_EXCLUDED_IDS in run_all_goldens_mac.sh)
# because it is 2 rvio jobs per clip at MAX_WORKERS=2; it is a mandatory check
# after all six gates pass, run automatically at the end of
# run_migration_loop_mac.sh.
#
# Usage:
#   ./run_folder_thumbnails_all.sh              # verify the Python port
#   IMPL=mu ./run_folder_thumbnails_all.sh      # Mu baseline integrity
#   CAPTURE=1 ./run_folder_thumbnails_all.sh    # (re)capture the Mu baseline, 2x
#
set -euo pipefail

if [ -z "${CAFFEINATED:-}" ]; then
    export CAFFEINATED=1
    exec caffeinate -d -i "$0" "$@"
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG="$HERE"
REPO_ROOT="$(cd "$PKG/../../../.." && pwd)"
ID="sm_folder_thumbnails_all"
IMPL="${IMPL:-python}"
CAPTURE="${CAPTURE:-0}"
# 83 clips through a 2-worker rvio pool, plus per-clip load: minutes, not seconds.
TIMEOUT="${TIMEOUT:-3600}"

export SM_MERIDIAN_DIR="${SM_MERIDIAN_DIR:-/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001}"

clips=$(find "$SM_MERIDIAN_DIR" -maxdepth 1 -name '*.mp4' | wc -l | tr -d ' ')
echo "full-folder thumbnail check: $clips clips from $SM_MERIDIAN_DIR"

if [ "$CAPTURE" = "1" ]; then
    echo "capturing Mu baseline for $ID (2 runs, determinism enforced)"
    exec env FORCE=1 TIMEOUT="$TIMEOUT" "$PKG/capture_golden_mac.sh" "$ID"
fi

exec env IMPL="$IMPL" TIMEOUT="$TIMEOUT" GATE="${GATE:-both}" DMAX="${DMAX:-0}" \
    "$PKG/run_all_goldens_mac.sh" "$ID"
