"""Scenario: tree categorises nodes — SOURCES / SEQUENCES / STACKS (COVERAGE A1, A2, A7, A9, primary #1).

Pins the categorisation itself, not just "a tree rendered": panel_before.png is
captured with sources only, then a sequence and a stack are added live and
panel_after.png must show the new SEQUENCES / STACKS headers. The two captures
must differ (VERIFICATION.md Primary outcomes rule 2).
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

# Build graph first — session_manager auto-deactivates when viewNodes() is empty.
src1 = sm.add_black_source(log=log)
src2 = sm.add_bars_source(log=log)

rvc.setViewNode(src1)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None, "tree view not found"

# RV seeds every session with a Default Sequence / Stack / Layout, so the
# discriminant is the category's child rows, not the presence of the header.
cats_before = sm.tree_category_items(None, log=log)
log("categories before", cats_before)
assert "SOURCES" in cats_before, f"SOURCES missing from tree; got: {list(cats_before.keys())}"
assert "TestSeq" not in cats_before.get("SEQUENCES", []), (
    f"TestSeq present before it is created: {cats_before}"
)
assert "TestStack" not in cats_before.get("STACKS", []), (
    f"TestStack present before it is created: {cats_before}"
)
before_png = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

# Add a sequence and a stack while the panel is live: the new-node events must
# route each node into its own category (COVERAGE M1).
seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "TestSeq")

stk = rvc.newNode("RVStackGroup", "")
rvc.setNodeInputs(stk, [src1, src2])
sm.set_ui_name(stk, "TestStack")
pump(600)

cats_after = sm.tree_category_items(None, log=log)
log("categories after", cats_after)

assert "SOURCES" in cats_after, f"SOURCES missing from tree; got: {list(cats_after.keys())}"
assert "SEQUENCES" in cats_after, f"SEQUENCES missing from tree; got: {list(cats_after.keys())}"
assert "STACKS" in cats_after, f"STACKS missing from tree; got: {list(cats_after.keys())}"
assert "TestSeq" in cats_after["SEQUENCES"], f"sequence miscategorised: {cats_after}"
assert "TestStack" in cats_after["STACKS"], f"stack miscategorised: {cats_after}"
assert rvc.viewNode() == src1, f"viewNode should be {src1}, got {rvc.viewNode()}"

after_png = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(before_png, after_png, "new category headers", log=log)
sm.save_session(out_dir, log=log)
diag.close()
