"""Scenario: tree columns resize to their contents (COVERAGE A10).\n\nresizeColumns() runs after every tree rebuild. The discriminant is that a much longer\nnode name widens column 0, so the two PNGs and the two widths must differ."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src = sm.add_black_source(log=log)
sm.set_ui_name(src, "Ab")
pump(200)

sm.activate_session_manager(log=log)
pump(600)

tv = sm.find_tree_view(log=log)
assert tv is not None, "tree view not found"
narrow = tv.columnWidth(0)
log("column 0 width with a short name:", narrow)
sm.grab_panel_png(out_dir, "panel_short.png", log=log)

sm.set_ui_name(src, "A" * 60)
pump(900)
wide = tv.columnWidth(0)
log("column 0 width with a long name:", wide)
sm.grab_panel_png(out_dir, "panel_long.png", log=log)

assert wide > narrow, (
    "column 0 should widen for a longer name (A10): %d -> %d" % (narrow, wide))

sm.save_session(out_dir, log=log)
diag.close()
