"""Shared helpers for conan/scripts/*.

Centralizes the few values that the conan/scripts/* tools all need:
the repo root, the packages.yml path, the conan user namespace, and the
windows_only filter rule. Keeping these in one place prevents the kind
of silent drift that previously had two scripts each spelling out the
same filter against different env signals.
"""

import os
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("error: pyyaml is required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGES_YML = REPO_ROOT / "conan" / "packages.yml"
PROFILES_DIR = REPO_ROOT / "conan" / "profiles"

CONAN_USER = "openrv"


def conan_exec() -> str:
    """Return the conan binary path (CONAN_EXEC env var, defaulting to "conan")."""
    return os.environ.get("CONAN_EXEC", "conan")


def load_packages() -> dict:
    """Load and return the packages.yml document as a dict."""
    pkgs = yaml.safe_load(PACKAGES_YML.read_text())
    if not isinstance(pkgs, dict):
        print(f"error: {PACKAGES_YML} is empty or not a mapping", file=sys.stderr)
        sys.exit(1)
    return pkgs


def find_dep(pkgs: dict, name: str, version: str) -> dict | None:
    """Find the dep entry in packages.yml matching the given name and version."""
    for dep in pkgs["deps"]:
        if dep["name"] == name and dep["version"] == version:
            return dep
    return None


def is_active_dep(dep: dict, *, is_windows: bool) -> bool:
    """Return True if `dep` should participate in the build on this platform.

    A dep flagged windows_only is only active on Windows; everything else
    is active everywhere. This is the single source of truth for the
    filter. openrvcore-conanfile.py mirrors the same key check at recipe
    parse time (it cannot import this module due to Conan's recipe
    sandboxing), so any rule change here must be reflected there.
    """
    if dep.get("windows_only") and not is_windows:
        return False
    return True
