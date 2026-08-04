"""Scenario: back/forward toolbar history (COVERAGE §C4, §C5).

Navigates start → ``commands`` → ``addSources``, back to module page (``browser.png``).
"""

import os

import _db_common as db

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


db.activate_doc_browser(log=log)
window = db.find_browser_window(log=log)
browser = db.find_doc_browser_widget(window, log=log)
assert window is not None and browser is not None

db.select_symbol_path(browser, [db.MODULE_SYMBOL], log=log)
db.select_symbol_path(browser, [db.MODULE_SYMBOL, "addSources"], log=log)
db.click_toolbar_action(window, "backButton", log=log)

column_view = db.find_column_view(browser, log=log)
current = column_view.currentIndex().data()
assert current == db.MODULE_SYMBOL, f"expected {db.MODULE_SYMBOL!r} after back, got {current!r}"
if log:
    log("after back, selection", current)

db.grab_browser_png(out_dir, log=log)
db.save_session(out_dir, log=log)
diag.close()
