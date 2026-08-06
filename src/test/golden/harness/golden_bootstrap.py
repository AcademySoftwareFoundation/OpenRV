#
# In-RV bootstrap for golden scenarios (imported from run_scenario.py -pyeval wrapper).
#
# Ensures immediate modes used by session_manager goldens are active before the
# scenario script runs.  RV loads these at state-initialized but they start
# inactive in headless -pyeval runs, so color setup and other handlers never fire.
#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
import os


def ensure_source_setup() -> None:
    """Activate the source_setup minor mode (idempotent)."""
    try:
        import source_setup

        source_setup.createMode()
    except Exception:
        pass


def ensure_local_thumbnail_gen() -> None:
    """Ensure local_thumbnail_gen is registered and active (idempotent)."""
    try:
        import local_thumbnail_gen

        local_thumbnail_gen.createMode()
        import rv.commands as rvc

        if not rvc.isModeActive("local_thumbnail_gen"):
            rvc.activateMode("local_thumbnail_gen")
    except Exception:
        pass


def bootstrap_from_env() -> None:
    if os.environ.get("GOLDEN_SOURCE_SETUP", "0") == "1":
        ensure_source_setup()
    # Default off: session_manager.activate() ensures local_thumbnail_gen.
    # Set GOLDEN_THUMBNAIL_GEN=1 only to test the legacy bootstrap path.
    if os.environ.get("GOLDEN_THUMBNAIL_GEN", "0") == "1":
        ensure_local_thumbnail_gen()


bootstrap_from_env()
