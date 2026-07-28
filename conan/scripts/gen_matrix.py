#!/usr/bin/env python3
"""Emit the GitHub Actions build matrix for conan-upload-deps.yml.

Reads conan/packages.yml and prints, in `key=value` form suitable for
appending to ``$GITHUB_OUTPUT``:

    matrix_nonwindows  - JSON array of dep entries to build on non-Windows.
    matrix_windows     - JSON array of dep entries to build on Windows.
    count_nonwindows   - Length of matrix_nonwindows.
    count_windows      - Length of matrix_windows.

Human-readable rows are written to stderr so the action log still shows
what's about to run.

Usage:
    python3 conan/scripts/gen_matrix.py [--only NAME] >> "$GITHUB_OUTPUT"
"""

import argparse
import json
import sys

from _common import CONAN_USER, is_active_dep, load_packages


def build_rows(pkgs: dict, *, is_windows: bool, only: str) -> list[dict]:
    rows = []
    for dep in pkgs["deps"]:
        if not is_active_dep(dep, is_windows=is_windows):
            continue
        if only and dep["name"] != only:
            continue
        rows.append(
            {
                "name": dep["name"],
                "version": dep["version"],
                "channel": dep["channel"],
                "cci_folder": dep["cci_folder"],
            }
        )
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--only", default="", help="Restrict matrix to a single dep name.")
    args = parser.parse_args()

    pkgs = load_packages()
    rows_nonwin = build_rows(pkgs, is_windows=False, only=args.only)
    rows_win = build_rows(pkgs, is_windows=True, only=args.only)

    if args.only and not (rows_nonwin or rows_win):
        print(f"::error::--only={args.only} matched no entry in conan/packages.yml", file=sys.stderr)
        sys.exit(1)

    print(f"matrix_nonwindows rows ({len(rows_nonwin)}):", file=sys.stderr)
    for r in rows_nonwin:
        print(f"  {r['name']}/{r['version']}@{CONAN_USER}/{r['channel']} ({r['cci_folder']})", file=sys.stderr)
    print(f"matrix_windows rows ({len(rows_win)}):", file=sys.stderr)
    for r in rows_win:
        print(f"  {r['name']}/{r['version']}@{CONAN_USER}/{r['channel']} ({r['cci_folder']})", file=sys.stderr)

    sys.stdout.write(f"matrix_nonwindows={json.dumps(rows_nonwin)}\n")
    sys.stdout.write(f"matrix_windows={json.dumps(rows_win)}\n")
    sys.stdout.write(f"count_nonwindows={len(rows_nonwin)}\n")
    sys.stdout.write(f"count_windows={len(rows_win)}\n")


if __name__ == "__main__":
    main()
