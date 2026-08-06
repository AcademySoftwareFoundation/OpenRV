#!/usr/bin/env bash
# Full-folder thumbnail check: load EVERY clip in SM_MERIDIAN_DIR (83 at the time
# of writing) and wait for all thumbnails and filmstrips to be generated.
#
# Behavioral gate only, unlike every other scenario. At 83 rows the panel PNG is
# not bit-reproducible between two runs of the *same* implementation: Qt
# re-rasterises the row labels with different subpixel weights under the font-cache
# churn of that many rows, and the per-pixel delta on a glyph edge reaches 167/255,
# so no dmax that still means anything can absorb it. Verified by capturing twice
# and diffing -- same glyphs, same layout, different rasterisation, in both the
# fallback and the generated half.
#
# Nothing is lost by dropping the pixel half here. What this scenario is for is
# asserted inside the scenario, on the graph and the cache rather than on pixels:
# 83 thumbnails and 83 filmstrips written, 83 rows off the fallback icon, and 83
# *distinct* row images (identical ones would mean rows sharing one thumbnail).
# The pixel-exact version of the same panel is sm_folder_thumbnails, at 12 clips,
# which does capture deterministically and is gated at dmax 0 like everything else.
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

exec env IMPL="$IMPL" TIMEOUT="$TIMEOUT" GATE="${GATE:-behavioral}" DMAX="${DMAX:-0}" \
    "$PKG/run_all_goldens_mac.sh" "$ID"
