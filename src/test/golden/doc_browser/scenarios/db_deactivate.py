"""Scenario: deactivate doc browser (COVERAGE §A4)."""

import os

import rv.commands as rvc

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
assert window is not None and window.isVisible()

db.grab_browser_png(out_dir, log=log)
db.deactivate_doc_browser(log=log)
assert not rvc.isModeActive(db.MODE_RUNTIME_NAME)

db.save_session(out_dir, log=log)
diag.close()
