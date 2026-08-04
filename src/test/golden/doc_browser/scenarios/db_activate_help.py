"""Scenario: Help menu activates doc browser (COVERAGE §A2).

Uses ``--menu-bar`` when Help is on the QMenuBar; otherwise falls back to the
``modeManager.activateMode`` path from ``openrv_help_menu_mode.mu``.
"""

import os

import rv.commands as rvc

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


if rvc.isModeActive(db.MODE_RUNTIME_NAME):
    db.deactivate_doc_browser(log=log)

db.activate_via_help_menu(log=log)
assert rvc.isModeActive(db.MODE_RUNTIME_NAME)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
