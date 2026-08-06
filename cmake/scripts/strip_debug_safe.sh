#!/bin/bash
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Safely strip DWARF debug info from an ELF binary.
#
# GNU strip has a known bug: for binaries whose layout makes it emit
#   "allocated section `.dynstr' not in segment"
# (observed on Google Crashpad's crashpad_handler and some Qt tools such as
# lupdate), it rewrites the program headers incorrectly and produces an
# UNLOADABLE binary (the loader fails with exit 127 / "Connection reset by
# peer" for crashpad_handler). strip can still exit 0 while emitting that
# warning, so a plain in-place `strip` silently corrupts the shipped binary.
#
# This wrapper strips to a temporary copy and only replaces the original when
# strip completed with NO warnings/errors on stderr and a zero exit code.
# Otherwise the original binary is left untouched (it keeps its debug info, but
# it stays loadable). Such binaries typically carry little or no DWARF anyway,
# so the size cost of skipping them is negligible.
#
# Usage: ./strip_debug_safe.sh <elf-binary>
#

TARGET="$1"

if [ -z "${TARGET}" ]; then
    echo "strip_debug_safe: usage: $0 <elf-binary>" >&2
    exit 2
fi

if [ ! -f "${TARGET}" ]; then
    # Nothing to strip (e.g. target produced no file); do not fail the build.
    echo "strip_debug_safe: no such file, skipping: ${TARGET}" >&2
    exit 0
fi

# Strip to a temp copy in the same directory (same filesystem => atomic mv).
TMP="$(mktemp "${TARGET}.stripXXXXXX")" || exit 1

# Capture stderr while discarding stdout. The command-substitution exit status
# is strip's exit code, which the following test consumes.
if STRIP_ERR="$(strip --strip-debug -o "${TMP}" "${TARGET}" 2>&1 >/dev/null)" && [ -z "${STRIP_ERR}" ]; then
    # Clean strip: preserve permissions and replace the original atomically.
    chmod --reference="${TARGET}" "${TMP}" 2>/dev/null || true
    mv -f "${TMP}" "${TARGET}"
else
    rm -f "${TMP}"
    echo "strip_debug_safe: keeping ${TARGET} unstripped (strip reported: ${STRIP_ERR:-nonzero exit})" >&2
fi

exit 0
