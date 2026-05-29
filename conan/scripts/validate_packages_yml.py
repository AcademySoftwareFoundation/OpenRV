#!/usr/bin/env python3
"""Validate the schema of conan/packages.yml.

Checks that every dep entry has the required fields with correct types,
valid channel values, and consistent optional field pairing.

Usage:
    python3 conan/scripts/validate_packages_yml.py
"""

import sys

import yaml

from _common import PACKAGES_YML

REQUIRED = {"name": str, "version": str, "channel": str, "cci_folder": str}
OPTIONAL = {"options": dict, "windows_only": bool, "source_url": str, "source_sha256": str}
VALID_CHANNELS = {"common", "vfx2025"}
ALL_KNOWN_FIELDS = set(REQUIRED) | set(OPTIONAL)


def validate(pkgs) -> list[str]:
    if not isinstance(pkgs, dict):
        return ["packages.yml is empty or not a mapping"]

    errors = []

    if "deps" not in pkgs:
        return ["missing top-level 'deps' key"]

    if not isinstance(pkgs["deps"], list):
        return ["'deps' must be a list"]

    seen: set[tuple[str, str]] = set()
    for i, dep in enumerate(pkgs["deps"]):
        label = dep.get("name") or f"deps[{i}]"

        for field, typ in REQUIRED.items():
            if field not in dep:
                errors.append(f"{label}: missing required field '{field}'")
            elif not isinstance(dep[field], typ):
                errors.append(f"{label}: '{field}' must be {typ.__name__}, got {type(dep[field]).__name__}")
            elif typ is str and not dep[field]:
                errors.append(f"{label}: '{field}' must not be empty")

        for field, typ in OPTIONAL.items():
            if field in dep and not isinstance(dep[field], typ):
                errors.append(f"{label}: '{field}' must be {typ.__name__}, got {type(dep[field]).__name__}")

        unknown = set(dep.keys()) - ALL_KNOWN_FIELDS
        if unknown:
            errors.append(f"{label}: unknown field(s): {', '.join(sorted(unknown))}")

        if dep.get("channel") not in VALID_CHANNELS:
            errors.append(f"{label}: channel must be one of {sorted(VALID_CHANNELS)}, got '{dep.get('channel')}'")

        has_url = "source_url" in dep
        has_sha = "source_sha256" in dep
        if has_url != has_sha:
            errors.append(f"{label}: source_url and source_sha256 must both be set or both absent")

        for k in dep.get("options") or {}:
            if ":" not in k:
                errors.append(f"{label}: option key '{k}' missing pattern prefix (expected 'pkg/*:opt')")

        name = dep.get("name")
        version = dep.get("version")
        if isinstance(name, str) and isinstance(version, str):
            key = (name, version)
            if key in seen:
                errors.append(f"{label}: duplicate (name, version) entry '{name}/{version}'")
            seen.add(key)

    return errors


def main() -> None:
    pkgs = yaml.safe_load(PACKAGES_YML.read_text())
    errors = validate(pkgs)
    if errors:
        print(f"conan/packages.yml validation failed ({len(errors)} error(s)):", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        sys.exit(1)
    print("conan/packages.yml schema is valid.")


if __name__ == "__main__":
    main()
