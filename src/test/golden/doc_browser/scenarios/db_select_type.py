"""Scenario: select ``rvtypes.MinorMode`` type page (COVERAGE §B6)."""

import os

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
browser = db.find_doc_browser_widget(window, log=log)
assert browser is not None

db.select_symbol_path(browser, [db.TYPE_MODULE, db.TYPE_SYMBOL], log=log)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
