#!/bin/bash
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Symbolicate RV crash dumps using Breakpad
#
# Usage: ./symbolicate_crash.sh <crash.dmp> [output.txt]
#

set -e

# Get the directory where this script lives
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Paths
MINIDUMP_STACKWALK="${SCRIPT_DIR}/minidump_stackwalk"

# Default Breakpad symbols directory (symbols/MODULE_NAME/MODULE_ID/), resolved
# relative to this script's shipped location. On macOS the script lives inside
# RV.app/Contents/MacOS/ (3 levels deep); on Linux it lives directly in bin/
# (1 level deep). Override with --symbols <dir> to symbolicate a customer dump
# against an unpacked symbols archive (symbols are not shipped inside the app).
if [[ "$OSTYPE" == "darwin"* ]]; then
    DEFAULT_SYMBOLS_DIR="${SCRIPT_DIR}/../../../symbols"
else
    DEFAULT_SYMBOLS_DIR="${SCRIPT_DIR}/../symbols"
fi

usage() {
    echo "Usage: $0 [--symbols <dir>] <crash.dmp> [output.txt]"
    echo ""
    echo "Options:"
    echo "  --symbols <dir>   Breakpad symbols tree to symbolicate against."
    echo "                    Defaults to the 'symbols' directory shipped next to"
    echo "                    this script. Point it at an unpacked symbols archive"
    echo "                    when symbolicating a customer-provided crash dump."
    echo "  -h, --help        Show this help and exit."
    echo ""
    echo "Examples:"
    echo "  $0 ~/Library/Logs/ASWF/Crashes/pending/crash.dmp"
    echo "  $0 crash.dmp crash_report.txt"
    echo "  $0 --symbols ./RV-3.1.0-linux-symbols/symbols crash.dmp report.txt"
}

# Parse options (must precede positional args). --symbols overrides the
# script-relative default above; everything else is positional.
SYMBOLS_DIR=""
POSITIONAL=()
while [ $# -gt 0 ]; do
    case "$1" in
        --symbols)
            if [ -z "$2" ]; then
                echo "ERROR: --symbols requires a directory argument"
                exit 1
            fi
            SYMBOLS_DIR="$2"
            shift 2
            ;;
        --symbols=*)
            SYMBOLS_DIR="${1#*=}"
            shift
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        --)
            shift
            while [ $# -gt 0 ]; do
                POSITIONAL+=("$1")
                shift
            done
            ;;
        -*)
            echo "ERROR: unknown option: $1"
            usage
            exit 1
            ;;
        *)
            POSITIONAL+=("$1")
            shift
            ;;
    esac
done
set -- "${POSITIONAL[@]}"

# Fall back to the script-relative default when no override was given
SYMBOLS_DIR="${SYMBOLS_DIR:-$DEFAULT_SYMBOLS_DIR}"

# Check arguments
if [ $# -lt 1 ]; then
    usage
    exit 1
fi

CRASH_DMP="$1"
OUTPUT="${2:-crash_report.txt}"

# Check if crash dump exists
if [ ! -f "$CRASH_DMP" ]; then
    echo "ERROR: Crash dump not found: $CRASH_DMP"
    exit 1
fi

# Check if minidump_stackwalk exists
if [ ! -f "$MINIDUMP_STACKWALK" ]; then
    echo "ERROR: minidump_stackwalk not found at: $MINIDUMP_STACKWALK"
    echo "Make sure Breakpad is built as part of the RV build."
    exit 1
fi

# Check if symbols directory exists
if [ ! -d "$SYMBOLS_DIR" ]; then
    echo "ERROR: Symbols directory not found at: $SYMBOLS_DIR"
    echo "Make sure RV was built with Breakpad symbol generation enabled."
    exit 1
fi

# Try to find minidump_dump tool
MINIDUMP_DUMP=""
# Check common locations relative to the script
# The first path is the shipping location (next to the script); the rest are
# dev-tree fallbacks. The build-dir segment is globbed (_build*) so it matches
# release/debug build directories alike rather than hardcoding _build_debug.
for path in \
    "${SCRIPT_DIR}/minidump_dump" \
    "${SCRIPT_DIR}"/../../../../RV_DEPS_BREAKPAD/install/bin/minidump_dump \
    "${SCRIPT_DIR}"/../../../_build*/RV_DEPS_BREAKPAD/install/bin/minidump_dump \
    "${SCRIPT_DIR}"/../../../../_build*/RV_DEPS_BREAKPAD/install/bin/minidump_dump \
    "${SCRIPT_DIR}"/../../../../../_build*/RV_DEPS_BREAKPAD/install/bin/minidump_dump
do
    if [ -f "$path" ]; then
        MINIDUMP_DUMP="$path"
        break
    fi
done

if [ -z "$MINIDUMP_DUMP" ]; then
    echo "WARNING: minidump_dump not found - Crashpad annotations will not be extracted"
    echo ""
fi

# Symbolicate
echo "Symbolicating crash dump..."
echo "  Input:   $CRASH_DMP"
echo "  Output:  $OUTPUT"
echo "  Symbols: $SYMBOLS_DIR"
echo ""

"$MINIDUMP_STACKWALK" "$CRASH_DMP" "$SYMBOLS_DIR" > "$OUTPUT"

# Replace "GPU: UNKNOWN" with actual GPU info from crashpad annotations if available
if [ -n "$MINIDUMP_DUMP" ] && [ -f "$MINIDUMP_DUMP" ]; then
    GPU_RENDERER=$("$MINIDUMP_DUMP" "$CRASH_DMP" 2>/dev/null | \
        grep -E 'crashpad_annotations\["gpu_renderer"\]' | \
        sed 's/.*= //' | tr -d '"')
    GPU_VENDOR=$("$MINIDUMP_DUMP" "$CRASH_DMP" 2>/dev/null | \
        grep -E 'crashpad_annotations\["gpu_vendor"\]' | \
        sed 's/.*= //' | tr -d '"')
    if [ -n "$GPU_RENDERER" ]; then
        GPU_INFO="${GPU_VENDOR:+${GPU_VENDOR} }${GPU_RENDERER}"
        # Escape characters special to sed's replacement text and the `|`
        # delimiter, so GPU strings containing them (e.g. "AMD ... | Render D129")
        # don't corrupt the substitution.
        GPU_INFO_ESC=$(printf '%s' "$GPU_INFO" | sed 's/[\\&|]/\\&/g')
        # Portable in-place edit: GNU `sed -i` and BSD/macOS `sed -i ''` are
        # incompatible, and this script ships on macOS. Rewrite via a temp file,
        # cleaned up on exit so a sed failure under `set -e` leaves nothing behind.
        TMP_OUTPUT="$(mktemp)"
        trap 'rm -f "$TMP_OUTPUT"' EXIT
        sed "s|^GPU: UNKNOWN|GPU: ${GPU_INFO_ESC}|" "$OUTPUT" > "$TMP_OUTPUT" && mv "$TMP_OUTPUT" "$OUTPUT"
    fi
fi

# Append Crashpad annotations if minidump_dump is available
if [ -n "$MINIDUMP_DUMP" ] && [ -f "$MINIDUMP_DUMP" ]; then
    echo "" >> "$OUTPUT"
    echo "========================================" >> "$OUTPUT"
    echo "Crashpad Annotations" >> "$OUTPUT"
    echo "========================================" >> "$OUTPUT"
    echo "" >> "$OUTPUT"

    # Extract crashpad_annotations from minidump
    ANNOTATIONS=$("$MINIDUMP_DUMP" "$CRASH_DMP" 2>/dev/null | \
        grep -E 'crashpad_annotations\[' | \
        sed 's/.*crashpad_annotations\["\([^"]*\)"\].*= \(.*\)/\1: \2/')

    if [ -n "$ANNOTATIONS" ]; then
        echo "$ANNOTATIONS" >> "$OUTPUT"
    else
        echo "No Crashpad annotations found in minidump" >> "$OUTPUT"
    fi

    # Also extract simple_annotations for context
    echo "" >> "$OUTPUT"
    echo "Simple Annotations:" >> "$OUTPUT"
    "$MINIDUMP_DUMP" "$CRASH_DMP" 2>/dev/null | \
        grep -E 'simple_annotations\[' | \
        sed 's/.*simple_annotations\["\([^"]*\)"\] = \(.*\)/\1: \2/' >> "$OUTPUT"
fi

echo "✅ Symbolication complete!"
echo ""
echo "View the report:"
echo "  cat $OUTPUT"
echo ""
echo "Or search for your crash:"
echo "  grep -A 20 'Thread 0 (crashed)' $OUTPUT"
echo ""
echo "View Mu script info:"
echo "  grep -A 10 'Crashpad Annotations' $OUTPUT"
