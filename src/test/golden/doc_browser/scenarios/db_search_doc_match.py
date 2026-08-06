"""Scenario: search matches symbol documentation text (COVERAGE §C1 doc-match branch)."""

import os

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")

# Appears in asciidoc_to_html module documentation, not necessarily in symbol names alone.
SEARCH_DOC_TERM = "asciidoc"


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
assert window is not None

db.run_search(window, SEARCH_DOC_TERM, log=log)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
