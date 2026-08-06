"""Scenario: after-clear-session empties the tree (COVERAGE M3).\n\nKept separate from the other event rows: clearSession tears the whole graph down, and\ncombining it with inputs-panel work in one scenario segfaulted RV."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "ClSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "ClSrc2")

sm.activate_session_manager(log=log)
pump(400)

before = sm.tree_category_items(None, log=log)
log("categories before:", sorted(before.keys()))
assert "SOURCES" in before, "expected SOURCES before the clear: %s" % list(before)
assert len(before["SOURCES"]) >= 2

sm.grab_panel_png(out_dir, "panel_before.png", log=log)

rvc.clearSession()
pump(1000)

after = sm.tree_category_items(None, log=log)
log("categories after:", sorted(after.keys()))
assert "SOURCES" not in after, (
    "after-clear-session must drop the SOURCES category (M3); still: %s" % list(after))

sm.save_session(out_dir, log=log)
diag.close()
