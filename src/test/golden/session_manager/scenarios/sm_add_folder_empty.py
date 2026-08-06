"""Scenario: Folder > Empty Folder creates an empty RVFolderGroup (COVERAGE D1)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

# Add one source to populate viewNodes() before activating session_manager.
src1 = sm.add_black_source(log=log)
rvc.setViewNode(src1)
pump(200)

sm.activate_session_manager(log=log)
pump(300)

# Create empty folder (mirrors newFolderSlot(which=1)).
folder = rvc.newNode("RVFolderGroup", "")
sm.set_ui_name(folder, "Empty Folder")
pump(300)

assert rvc.nodeExists(folder), "folder node not created"
assert rvc.nodeType(folder) == "RVFolderGroup"
inputs = rvc.nodeConnections(folder, False)[0]
assert inputs == [], f"empty folder should have no inputs, got: {inputs}"

ui_name = sm.get_ui_name(folder)
log("folder ui name", ui_name, "inputs", inputs)
assert "Folder" in ui_name, f"unexpected folder name: {ui_name}"

tree_view = sm.find_tree_view(log=log)
cats = sm.tree_category_items(None, log=log)
assert "FOLDERS" in cats, f"FOLDERS not in tree: {list(cats.keys())}"

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
