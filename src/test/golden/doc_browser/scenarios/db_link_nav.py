"""Scenario: ``mudoc://`` link to ``commands.addSources`` (COVERAGE primary #4, §B7).

Selects the commands module first, then navigates via handleLink to a function doc page.
"""

import os

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")
MUDOC_URL = f"mudoc:///{db.FUNCTION_SYMBOL}"


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
assert window is not None
browser = db.find_doc_browser_widget(window, log=log)
assert browser is not None

db.select_symbol_by_display_name(browser, db.MODULE_SYMBOL, log=log)
db.navigate_mudoc_link(window, MUDOC_URL, log=log)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
