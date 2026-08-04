"""Scenario: Folder > From Selection wraps selected nodes (COVERAGE D2)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "FolderSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "FolderSrc2")

folder = rvc.newNode("RVFolderGroup", "")
rvc.setNodeInputs(folder, [src1, src2])
sm.set_ui_name(folder, "Folder of FolderSrc1 and FolderSrc2")
rvc.setViewNode(folder)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

assert rvc.nodeExists(folder)
inputs = rvc.nodeConnections(folder, False)[0]
assert src1 in inputs and src2 in inputs
log("folder", folder, "inputs", inputs)

tree_view = sm.find_tree_view(log=log)
cats = sm.tree_category_items(None, log=log)
assert "FOLDERS" in cats, f"FOLDERS missing: {list(cats.keys())}"

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
