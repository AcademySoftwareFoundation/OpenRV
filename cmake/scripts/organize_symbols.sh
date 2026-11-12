#!/bin/bash
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Organize Breakpad .sym file into proper directory structure
#
# Usage: ./organize_symbols.sh <rv.sym> <output_dir>
#

set -e

SYM_FILE="$1"
OUTPUT_DIR="$2"
MODULE_NAME_OVERRIDE="${3:-}"  # optional: rename the module (e.g. rv → rv.bin)

if [ ! -f "$SYM_FILE" ]; then
    echo "ERROR: Symbol file not found: $SYM_FILE"
    exit 1
fi

# Extract MODULE_ID and MODULE_NAME from first line of .sym file
# Format: MODULE <os> <arch> MODULE_ID MODULE_NAME
MODULE_ID=$(head -1 "$SYM_FILE" | awk '{print $4}')
MODULE_NAME=$(head -1 "$SYM_FILE" | awk '{print $5}')

# On Linux the binary is renamed to .bin by RV_STAGE after dump_syms runs, so
# the sym file records the pre-rename name.  Patch line 1 so minidump_stackwalk
# can match the module name reported in the crash dump.
if [ -n "$MODULE_NAME_OVERRIDE" ] && [ "$MODULE_NAME" != "$MODULE_NAME_OVERRIDE" ]; then
    # Rewrite field 5 (the module name) of the MODULE header line with awk rather
    # than sed, so names containing regex metacharacters (e.g. "libssl.so.1.1")
    # are matched literally.
    awk -v ovr="$MODULE_NAME_OVERRIDE" 'NR==1{$5=ovr} 1' "$SYM_FILE" > "${SYM_FILE}.tmp"
    mv "${SYM_FILE}.tmp" "$SYM_FILE"
    MODULE_NAME="$MODULE_NAME_OVERRIDE"
fi

# Create directory structure
SYMBOL_DIR="$OUTPUT_DIR/$MODULE_NAME/$MODULE_ID"
mkdir -p "$SYMBOL_DIR"

# Copy symbol file
cp "$SYM_FILE" "$SYMBOL_DIR/$MODULE_NAME.sym"

echo "Organized symbols: $SYMBOL_DIR/$MODULE_NAME.sym"
