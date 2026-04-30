#!/usr/bin/env python3
#
# Sanitize Homebrew Links for UTV
# Converts /opt/homebrew/Cellar/... links to /opt/homebrew/opt/...
# Copyright (C) 2026 Makai Systems. All Rights Reserved.
#

import os
import sys
import subprocess
import pathlib
import re

BREW_PREFIX = "/opt/homebrew"  # Default for Apple Silicon
CELLAR_PATTERN = re.compile(rf"{BREW_PREFIX}/Cellar/([^/]+)/[^/]+/(.+)")


def get_dependencies(binary_path):
    try:
        output = subprocess.check_output(["otool", "-L", binary_path], text=True)
        # Skip first line (the binary itself)
        deps = []
        for line in output.splitlines()[1:]:
            parts = line.strip().split()
            if parts:
                deps.append(parts[0])
        return deps
    except Exception:
        return []


def sanitize_binary(binary_path):
    print(f"--- Sanitizing: {os.path.basename(binary_path)}")
    deps = get_dependencies(binary_path)
    changed = False

    for dep in deps:
        match = CELLAR_PATTERN.match(dep)
        if match:
            pkg_name = match.group(1)
            remaining_path = match.group(2)
            stable_path = f"{BREW_PREFIX}/opt/{pkg_name}/{remaining_path}"

            # Verify stable path exists
            if os.path.exists(stable_path):
                print(f"  Mapping: {dep} -> {stable_path}")
                try:
                    subprocess.check_call(["install_name_tool", "-change", dep, stable_path, binary_path])
                    changed = True
                except subprocess.CalledProcessError as e:
                    print(f"  Error changing link: {e}")
            else:
                print(f"  Warning: Stable path {stable_path} not found for {dep}")

    return changed


def main():
    if len(sys.argv) < 2:
        print("Usage: sanitize_homebrew_links.py <path_to_search>")
        sys.exit(1)

    search_path = pathlib.Path(sys.argv[1])
    if not search_path.exists():
        print(f"Error: Path {search_path} does not exist.")
        sys.exit(1)

    print(f"Scanning for binaries in: {search_path}")

    count = 0
    for root, _, files in os.walk(search_path):
        for f in files:
            full_path = os.path.join(root, f)

            # Basic check for Mach-O binaries
            try:
                file_info = subprocess.check_output(["file", "-b", full_path], text=True)
                if "Mach-O" in file_info:
                    if sanitize_binary(full_path):
                        count += 1
            except Exception:
                continue

    print(f"Done. Sanitized {count} binaries.")


if __name__ == "__main__":
    main()
