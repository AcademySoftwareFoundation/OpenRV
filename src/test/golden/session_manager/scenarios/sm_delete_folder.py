"""Scenario: delete folder via delete button (COVERAGE E3, E4)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump, click_button

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "FolderChild")
folder = rvc.newNode("RVFolderGroup", "")
sm.set_ui_name(folder, "DeleteMe")
rvc.setNodeInputs(folder, [src1])
rvc.setViewNode(src1)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None

ok = sm.select_tree_item_for_node(None, folder, log=log)
assert ok, f"could not select folder {folder}"
pump(300)

del_btn = sm.find_delete_button(log=log)
assert del_btn is not None
click_button(del_btn, settle_ms=600)
pump(600)

assert not rvc.nodeExists(folder), f"folder {folder} should be deleted"
# Child source should still exist (folder deletion only removes the container).
assert rvc.nodeExists(src1), f"source {src1} should survive folder deletion"
log("folder deleted", folder, "child still exists:", rvc.nodeExists(src1))

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
