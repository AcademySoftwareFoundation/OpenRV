"""Scenario: the Delete button removes every selected row (COVERAGE E5)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

names = []
for i in range(3):
    n = sm.add_black_source(log=log)
    sm.set_ui_name(n, "DmSrc%d" % i)
    names.append(n)

sm.activate_session_manager(log=log)
pump(400)

sm.select_tree_items_for_nodes(None, names[:2], log=log)
selected = sm.tree_selected_nodes(log=log)
assert len(selected) == 2, "expected a two-row selection, got %s" % (selected,)

delete = sm.find_delete_button(log=log)
assert delete is not None, "deleteButton not found"
delete.click()
pump(600)

gone = [n for n in names[:2] if not rvc.nodeExists(n)]
assert len(gone) == 2, "both selected sources should be gone, missing: %s" % (gone,)
assert rvc.nodeExists(names[2]), "the unselected source must survive"
log("remaining:", [n for n in names if rvc.nodeExists(n)])

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
