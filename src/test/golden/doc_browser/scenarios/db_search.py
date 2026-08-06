"""Scenario: search for ``commands`` (COVERAGE primary #3, §C1).

Pixel: search results page must differ from start page.
"""

import os

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
assert window is not None

db.run_search(window, db.MODULE_SYMBOL, log=log)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
