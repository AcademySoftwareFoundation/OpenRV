"""Scenario: closing browser window toggles mode off (COVERAGE §A5)."""

import os

import rv.commands as rvc

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
db.grab_browser_png(out_dir, log=log)
db.hide_browser_window(log=log)
assert not rvc.isModeActive(db.MODE_RUNTIME_NAME), "mode still active after window close"

db.save_session(out_dir, log=log)
diag.close()
