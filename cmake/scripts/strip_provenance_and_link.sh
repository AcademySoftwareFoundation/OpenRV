#!/bin/bash
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# strip_provenance_and_link.sh — macOS 26+ provenance-safe linker wrapper
#
# On macOS 26+, com.apple.provenance is kernel-enforced and irremovable. When multiple
# processes write signed Mach-O files to the same directory concurrently (parallel linker
# invocations, linker + file-copy custom commands, etc.), the kernel blocks the open()
# with EPERM, causing "ld: open() failed, errno=1".
#
# Fix: link to a private temporary directory (no contention), then atomically move the
# output to the real destination. Each link step gets its own temp dir, so there is zero
# concurrent write contention regardless of Ninja parallelism.
#
# The -install_name / @rpath embedded in the binary comes from the -install_name flag,
# not from the -o path, so moving the file afterward is safe.
#
# Usage: strip_provenance_and_link.sh <linker> [args...] -o <output> [more args...]

# Find the -o argument and rewrite it to a temp directory.
args=("$@")
real_output=""

for ((i=0; i<${#args[@]}; i++)); do
  if [[ "${args[$i]}" == "-o" ]]; then
    j=$((i+1))
    real_output="${args[$j]}"
    fname=$(basename "$real_output")

    # Create a private temp dir for this link step.
    tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/rv_link.XXXXXX")
    trap "rm -rf '$tmpdir'" EXIT

    # Rewrite -o to point to the temp dir.
    args[$j]="${tmpdir}/${fname}"
    break
  fi
done

# Debug: verify this script version is actually running.
echo "PROVENANCE_WRAPPER_V3: real_output=${real_output} tmpdir=${tmpdir} fname=${fname}" >&2

# Run the linker, outputting to the private temp dir.
"${args[@]}"
status=$?

if [[ $status -eq 0 && -n "$real_output" ]]; then
  # Atomically move the output to the real destination.
  # mv within the same filesystem is atomic; across filesystems it copies+deletes.
  mv -f "${tmpdir}/${fname}" "${real_output}"
  mv_status=$?
  if [[ $mv_status -ne 0 ]]; then
    echo "PROVENANCE_WRAPPER_V3: mv failed (status=$mv_status) from ${tmpdir}/${fname} to ${real_output}" >&2
    status=$mv_status
  fi
fi

exit $status
