"""Scenario: order-up/down buttons reorder inputs (COVERAGE G4, G5, primary #7).

The inputs panel row order is the visible outcome, so panel_before.png is taken
with the original order and panel_after.png after Input2 has been moved up.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump, click_button

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "Input1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "Input2")
src3 = sm.add_white_source(log=log)
sm.set_ui_name(src3, "Input3")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2, src3])
sm.set_ui_name(seq, "ReorderSeq")
rvc.setViewNode(seq)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

inputs_view = sm.find_inputs_view(log=log)
assert inputs_view is not None, "inputs view not found"

orig = sm.get_inputs_node_list(None, log=log)
log("original inputs", orig)
assert orig == [src1, src2, src3], f"unexpected initial order: {orig}"
orig_conn = rvc.nodeConnections(seq, False)[0]
assert orig_conn == [src1, src2, src3], f"unexpected initial connections: {orig_conn}"

sm.select_inputs_tab(log=log)
panel_before = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

# Select src2 and move it up.
sm.select_inputs_item(None, src2, log=log)
pump(200)

up_btn = sm.find_order_up_button(log=log)
assert up_btn is not None
click_button(up_btn, settle_ms=400)
pump(400)

after_up = sm.get_inputs_node_list(None, log=log)
log("after move-up", after_up)
new_inputs = rvc.nodeConnections(seq, False)[0]
log("node connections after up", new_inputs)
assert new_inputs.index(src2) < new_inputs.index(src1), (
    f"src2 should be before src1 after move-up: {new_inputs}"
)
panel_after_up = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after_up, "inputs rows reordered", log=log)

down_btn = sm.find_order_down_button(log=log)
assert down_btn is not None
click_button(down_btn, settle_ms=400)
pump(400)

after_down = sm.get_inputs_node_list(None, log=log)
log("after move-down", after_down)
final_inputs = rvc.nodeConnections(seq, False)[0]
log("node connections after down", final_inputs)
assert final_inputs == orig_conn, (
    f"move-down should restore the original order: {orig_conn} -> {final_inputs}"
)

sm.grab_panel_png(out_dir, "panel_restored.png", log=log)
sm.save_session(out_dir, log=log)
diag.close()
