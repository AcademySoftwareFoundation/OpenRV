"""Scenario: Add > Stack wraps selected sources (COVERAGE C2, primary #5).

Before/after pair: loose sources only, then the stack exists and is the active
view, so the new STACKS row is the pixel discriminant.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "StkSrc1")
src2 = sm.add_white_source(log=log)
sm.set_ui_name(src2, "StkSrc2")

rvc.setViewNode(src1)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

stacks_before = sm.tree_category_items(None, log=log).get("STACKS", [])
log("STACKS rows before", stacks_before)
panel_before = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

stk = rvc.newNode("RVStackGroup", "")
rvc.setNodeInputs(stk, [src1, src2])
sm.set_ui_name(stk, "Stack of StkSrc1 and StkSrc2")
rvc.setViewNode(stk)
pump(600)

assert rvc.nodeExists(stk)
assert rvc.nodeType(stk) == "RVStackGroup"
inputs = rvc.nodeConnections(stk, False)[0]
assert src1 in inputs and src2 in inputs

tree_view = sm.find_tree_view(log=log)
cats = sm.tree_category_items(None, log=log)
assert "STACKS" in cats, f"STACKS not in tree: {list(cats.keys())}"
assert len(cats["STACKS"]) == len(stacks_before) + 1, (
    f"STACKS should gain one row: {stacks_before} -> {cats['STACKS']}"
)
log("stack created", stk, "inputs", inputs)

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "new stack row", log=log)
sm.save_session(out_dir, log=log)
diag.close()
