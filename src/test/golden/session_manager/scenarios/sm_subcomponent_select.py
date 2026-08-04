"""Scenario: sub-component radio selection (COVERAGE B3, B4, A3, A4)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump, QtCore

clip = sm.SM_CLIP_1
assert os.path.exists(clip), f"media fixture not found: {clip}"

# Load source BEFORE activating.
snode = rvc.addSourceVerbose([clip])
sm.wait_for_progressive_loading(log=log)
pump(400)
group = rvc.nodeGroup(snode)
rvc.setViewNode(group)
pump(200)
log("loaded", clip, "group", group)

sm.activate_session_manager(log=log)
pump(400)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None, "tree view not found"
model = tree_view.model()
assert model is not None, "tree model is None"

sm.select_tree_item_for_node(None, group, log=log)
pump(300)
root = model.invisibleRootItem()

def find_group_item(parent):
    for row in range(parent.rowCount()):
        item = parent.child(row, 0)
        if item is None:
            continue
        node_data = item.data(QtCore.Qt.UserRole + 2)
        if node_data == group:
            return item
        result = find_group_item(item)
        if result is not None:
            return result
    return None

group_item = find_group_item(root)
assert group_item is not None, f"group item for {group} not found in tree"
group_idx = model.indexFromItem(group_item)
tree_view.setExpanded(group_idx, True)
pump(400)

log("group item children:", group_item.rowCount())

sub_found = False
for child_row in range(group_item.rowCount()):
    child = group_item.child(child_row, 0)
    if child is None:
        continue
    sub_type = child.data(QtCore.Qt.UserRole + 4)
    sub_value = child.data(QtCore.Qt.UserRole + 5)
    log("sub-component row", child_row, "type", sub_type, "value", sub_value)
    if sub_type in (2, 3, 4) and sub_value:
        radio_idx = model.indexFromItem(group_item.child(child_row, 1))
        if radio_idx.isValid():
            from qt_scenario_utils import QTest
            rect = tree_view.visualRect(radio_idx)
            QTest.mouseClick(
                tree_view.viewport(),
                QtCore.Qt.LeftButton,
                QtCore.Qt.NoModifier,
                rect.center(),
            )
            pump(400)
            sub_found = True
            log("clicked sub-component radio for", sub_value)
            break

if sub_found:
    img_comp = rvc.getStringProperty(snode + ".request.imageComponent")
    log("imageComponent after click", img_comp)
    assert len(img_comp) >= 2, f"imageComponent should have type+value: {img_comp}"
else:
    log("NOTE: no sub-components found (single-view mp4) — behavioral check only")

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
