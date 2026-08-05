"""Scenario: Folder > From Copy of Selection (COVERAGE D3).\n\nThe copy variant wraps copies, so the original parent's connections stay intact —\nthat is the discriminant against From Selection, which moves the nodes out."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "FcSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "FcSrc2")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "FcSequence")
rvc.setViewNode(seq)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

before = list(rvc.nodeConnections(seq, False)[0])
log("sequence inputs before:", before)

sm.select_tree_items_for_nodes(None, [src1, src2], log=log)
folderBtn = sm.find_folder_button(log=log)
assert folderBtn is not None, "folderButton not found"
sm.trigger_menu_action(folderBtn, "From Copy of Selection", log=log)

folders = [n for n in rvc.nodes() if rvc.nodeType(n) == "RVFolderGroup"]
assert folders, "no folder created"
folder = folders[0]
log("folder", folder, "inputs", rvc.nodeConnections(folder, False)[0])

after = list(rvc.nodeConnections(seq, False)[0])
assert after == before, (
    "From Copy must not disturb the original parent: %s -> %s" % (before, after))
assert rvc.nodeConnections(folder, False)[0], "the folder must wrap something"

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
