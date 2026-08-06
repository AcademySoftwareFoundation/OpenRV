#!/usr/bin/env bash
# Printed at the start of every ./run_migration_loop*.sh invocation.
# The shell does not parse VERIFICATION.md or COVERAGE.md — this reminds the agent to.
#
# Usage (from run_migration_loop*.sh):
#   GOLDEN_PKG_DIR="$HERE" source "$REPO_ROOT/src/test/golden/harness/migration_loop_agent_reminder.sh"
#
set -euo pipefail

_GOLDEN_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
_PKG_DIR="${GOLDEN_PKG_DIR:-$(pwd)}"
_VERIFICATION="$_GOLDEN_ROOT/VERIFICATION.md"
_COVERAGE="$_PKG_DIR/COVERAGE.md"

echo
echo ">>> AGENT (mandatory — scripts do NOT enforce this; you must):"
echo "    0. Loop purpose: drive migration with /loop — re-run the migration prompt each tick"
echo "       until VERIFICATION.md Definition of done (see skill §5)."
echo "    1. Start of migration session: read ALL of $_VERIFICATION once."
echo "    2. EVERY loop iteration (including this run): read $_COVERAGE"
echo "       — Primary outcomes table first, then statuses (no unjustified 🟡/⬜)."
echo "       — Mu methods → Python unit tests table: no ⬜ at migration done (gate 5)."
echo "    3. Loop procedure: .agents/skills/mu-python-migration/SKILL.md §5"
echo "    4. Loop exit 0 ≠ migration done until VERIFICATION.md Definition of done."
echo

if [ ! -f "$_COVERAGE" ]; then
    echo "WARNING: missing $_COVERAGE — create from COVERAGE.template.md before looping."
else
    if ! grep -q '^## Primary outcomes' "$_COVERAGE" 2>/dev/null; then
        echo "WARNING: $_COVERAGE has no '## Primary outcomes' section — see VERIFICATION.md."
    fi
    if ! grep -q '^## Mu methods' "$_COVERAGE" 2>/dev/null; then
        echo "WARNING: $_COVERAGE has no '## Mu methods' section — gate 5 requires it (see VERIFICATION.md)."
    fi
fi

echo "    COVERAGE: $_COVERAGE"
echo "    VERIFY:   $_VERIFICATION"
echo
