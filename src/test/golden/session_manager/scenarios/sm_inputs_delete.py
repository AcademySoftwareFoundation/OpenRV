"""Scenario: delete-from-inputs removes an input from a node (COVERAGE G6, G7)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump, click_button

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "Keep")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "Remove")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "InputDelSeq")
rvc.setViewNode(seq)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

inputs_view = sm.find_inputs_view(log=log)
assert inputs_view is not None

sm.select_inputs_tab(log=log)
sm.select_inputs_item(None, src2, log=log)
pump(200)

inputs_del_btn = sm.find_inputs_delete_button(log=log)
assert inputs_del_btn is not None, "inputsDeleteButton not found"
click_button(inputs_del_btn, settle_ms=400)
pump(400)

remaining = rvc.nodeConnections(seq, False)[0]
log("inputs after delete", remaining)
assert src2 not in remaining, f"{src2} should be removed from inputs"
assert src1 in remaining, f"{src1} should remain in inputs"

assert rvc.nodeExists(src2), "source node itself should not be deleted"

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
