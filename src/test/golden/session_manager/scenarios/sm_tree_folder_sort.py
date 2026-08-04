"""Scenario: folder sort order persistence (COVERAGE A5, A6)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

# Build graph first.
src_a = sm.add_black_source(log=log)
sm.set_ui_name(src_a, "Alpha")
src_b = sm.add_bars_source(log=log)
sm.set_ui_name(src_b, "Beta")
src_c = sm.add_white_source(log=log)
sm.set_ui_name(src_c, "Gamma")

folder = rvc.newNode("RVFolderGroup", "")
sm.set_ui_name(folder, "SortedFolder")
rvc.setNodeInputs(folder, [src_a, src_b, src_c])
rvc.setViewNode(folder)
pump(200)

# Set sort keys.
sort_key_prop_parent = folder + ".sm_state.sortKeyParent"
sort_key_prop_key = folder + ".sm_state.sortKey"
if not rvc.propertyExists(sort_key_prop_parent):
    rvc.newProperty(sort_key_prop_parent, rvc.StringType, 3)
if not rvc.propertyExists(sort_key_prop_key):
    rvc.newProperty(sort_key_prop_key, rvc.IntType, 3)
rvc.setStringProperty(sort_key_prop_parent, [src_c, src_a, src_b], True)
rvc.setIntProperty(sort_key_prop_key, [0, 1, 2], True)

sm.activate_session_manager(log=log)
pump(300)

parents = rvc.getStringProperty(sort_key_prop_parent)
keys = rvc.getIntProperty(sort_key_prop_key)
log("sort key parents", parents)
log("sort keys", keys)
assert len(parents) == 3, f"expected 3 sortKeyParent entries, got {len(parents)}"
assert len(keys) == 3, f"expected 3 sortKey entries, got {len(keys)}"
assert src_c in parents, "Gamma should be in sortKeyParent"

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
