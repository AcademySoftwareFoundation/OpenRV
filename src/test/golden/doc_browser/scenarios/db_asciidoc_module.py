"""Scenario: ``asciidoc_to_html`` module docs (COVERAGE §D1–D6 markup in module docstring)."""

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

db.select_symbol_by_display_name(browser, db.ASCIIDOC_MODULE, log=log)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
