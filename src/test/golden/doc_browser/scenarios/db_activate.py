"""Scenario: activate doc browser — start/legend page (COVERAGE primary #1, §A1).

Captures behavioral gate (empty session.rv) + browser.png pixel baseline.
"""

import os

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
assert window is not None and window.isVisible(), "doc browser window not shown"

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
