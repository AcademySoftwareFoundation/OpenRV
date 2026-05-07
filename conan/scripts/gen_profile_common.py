#!/usr/bin/env python3
"""Generate (or validate) the [replace_requires] section of conan/profiles/common.

The section is derived entirely from conan/packages.yml, which is the single
source of truth for Conan-managed dependency versions and channels.

Usage:
    # Regenerate profiles/common in-place (run after editing packages.yml):
    python3 conan/scripts/gen_profile_common.py

    # CI validation mode - exits non-zero if profiles/common is out of sync:
    python3 conan/scripts/gen_profile_common.py --check
"""

import argparse
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("error: pyyaml is required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGES_YML = REPO_ROOT / "conan" / "packages.yml"
PROFILE_COMMON = REPO_ROOT / "conan" / "profiles" / "common"

REPLACE_REQUIRES_HEADER = "[replace_requires]"


def build_replace_requires(pkgs: dict) -> list[str]:
    """Return lines for the [replace_requires] section from packages.yml."""
    lines = [REPLACE_REQUIRES_HEADER]
    for dep in pkgs["deps"]:
        name = dep["name"]
        version = dep["version"]
        channel = dep["channel"]
        line = f"{name}/*: {name}/{version}@openrv/{channel}"
        # libjpeg-turbo needs an extra alias so that deps that require
        # "libjpeg" (without the -turbo suffix) are also redirected.
        if name == "libjpeg-turbo":
            lines.append(f"libjpeg/*: {name}/{version}@openrv/{channel}")
        lines.append(line)
    return lines


def read_profile_preamble(profile_path: Path) -> str:
    """Return everything in profiles/common before the [replace_requires] section."""
    text = profile_path.read_text()
    idx = text.find(REPLACE_REQUIRES_HEADER)
    if idx == -1:
        # No existing section; return the whole file as preamble.
        return text.rstrip("\n")
    return text[:idx].rstrip("\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate that profiles/common matches packages.yml; exit 1 if not.",
    )
    args = parser.parse_args()

    pkgs = yaml.safe_load(PACKAGES_YML.read_text())
    new_section_lines = build_replace_requires(pkgs)
    new_section = "\n".join(new_section_lines) + "\n"

    preamble = read_profile_preamble(PROFILE_COMMON)
    new_content = preamble + "\n\n" + new_section

    if args.check:
        current = PROFILE_COMMON.read_text()
        if current == new_content:
            print("profiles/common is in sync with packages.yml.")
            sys.exit(0)
        else:
            import difflib

            diff = difflib.unified_diff(
                current.splitlines(keepends=True),
                new_content.splitlines(keepends=True),
                fromfile="conan/profiles/common (current)",
                tofile="conan/profiles/common (expected from packages.yml)",
            )
            sys.stdout.writelines(diff)
            print(
                "\nerror: conan/profiles/common is out of sync with conan/packages.yml.\n"
                "Run: python3 conan/scripts/gen_profile_common.py",
                file=sys.stderr,
            )
            sys.exit(1)
    else:
        PROFILE_COMMON.write_text(new_content)
        print(f"Wrote {PROFILE_COMMON.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()
