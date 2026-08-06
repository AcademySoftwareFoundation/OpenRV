"""Scenario: Delete on a node that is in more than one folder removes the input
(COVERAGE E2).

The rule is narrower than "has other parents". session_manager.mu.in's
deleteViewableSlot counts how many of the node's *outputs are RVFolderGroups* and only
calls removeInput when the selected row's parent is a folder AND that count is > 1:

    if (parentType == "RVFolderGroup" && nfolders > 1) removeInput(parent, node);
    else                                               deleteNode(node);

So a source in one folder plus a sequence is still deleted outright. Both arms are
pinned here, because the discriminant is the whole point of the row.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

# shared: lives in two folders     |     lone: one folder plus a sequence
shared = sm.add_black_source(log=log)
sm.set_ui_name(shared, "DfShared")
lone = sm.add_bars_source(log=log)
sm.set_ui_name(lone, "DfLone")

folderA = rvc.newNode("RVFolderGroup", "")
rvc.setNodeInputs(folderA, [shared, lone])
sm.set_ui_name(folderA, "DfFolderA")

folderB = rvc.newNode("RVFolderGroup", "")
rvc.setNodeInputs(folderB, [shared])
sm.set_ui_name(folderB, "DfFolderB")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [lone])
sm.set_ui_name(seq, "DfSequence")

rvc.setViewNode(folderA)
pump(200)
sm.activate_session_manager(log=log)
pump(400)

delete = sm.find_delete_button(log=log)
assert delete is not None, "deleteButton not found"

# --- arm 1: two folders -> removeInput, the node survives ------------------------
assert sm.select_tree_item_under_parent(folderA, shared, log=log), (
    "could not select %s under %s" % (shared, folderA))
delete.click()
pump(600)

log("shared exists:", rvc.nodeExists(shared))
log("folderA inputs:", rvc.nodeConnections(folderA, False)[0])
log("folderB inputs:", rvc.nodeConnections(folderB, False)[0])
assert rvc.nodeExists(shared), "a node in two folders must not be deleted outright"
assert shared not in rvc.nodeConnections(folderA, False)[0], "it must leave folderA"
assert shared in rvc.nodeConnections(folderB, False)[0], "folderB must still hold it"

# --- arm 2: one folder plus a sequence -> deleted outright -----------------------
assert sm.select_tree_item_under_parent(folderA, lone, log=log), (
    "could not select %s under %s" % (lone, folderA))
delete.click()
pump(600)

log("lone exists:", rvc.nodeExists(lone))
log("sequence inputs:", rvc.nodeConnections(seq, False)[0])
assert not rvc.nodeExists(lone), (
    "only one folder holds it, so deleteViewableSlot deletes the node")

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
