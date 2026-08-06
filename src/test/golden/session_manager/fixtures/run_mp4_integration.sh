#!/usr/bin/env bash
# Optional mp4 integration suite — NOT part of run_all_goldens.sh.
# Runs sm_mp4_all in isolation with verbose output.
#
# Usage:
#   ./fixtures/run_mp4_integration.sh
#   SM_MERIDIAN_DIR=/path/to/clips ./fixtures/run_mp4_integration.sh
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG="$(cd "$HERE/.." && pwd)"
REPO_ROOT="$(cd "$PKG/../../../.." && pwd)"
RUNNER="$REPO_ROOT/src/test/golden/harness/run_scenario.py"
COMPARE="$REPO_ROOT/src/test/golden/harness/compare.py"
RV="${RV:-$REPO_ROOT/_build/stage/app/RV.app/Contents/MacOS/RV}"

export SM_MERIDIAN_DIR="${SM_MERIDIAN_DIR:-/Users/termev/Documents/media/Meridian-PS-Cloth/Meridian-Cloth-PS-V001}"
GOLDEN="$PKG/golden-mac/sm_mp4_all"
OUT="/tmp/sm_mp4_integration"

echo "session_manager mp4 integration suite"
echo "SM_MERIDIAN_DIR=$SM_MERIDIAN_DIR"
rm -rf "$OUT"; mkdir -p "$OUT"

python3 "$RUNNER" \
    --scenario "$PKG/scenarios/sm_mp4_all.py" \
    --out "$OUT" --rv "$RV" \
    --impl python --no-xvfb --timeout 600

echo "--- diag ---"
cat "$OUT/diag.txt" 2>/dev/null || true

if [ -f "$GOLDEN/session.rv" ]; then
    echo "--- compare vs golden-mac ---"
    python3 "$COMPARE" --golden-dir "$GOLDEN" --actual-dir "$OUT" --dmax 0
fi
