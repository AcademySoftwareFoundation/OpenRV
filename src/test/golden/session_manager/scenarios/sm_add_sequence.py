"""Scenario: Add > Sequence wraps selected sources (COVERAGE C1, G1, G3, primary #5).

panel_before.png is the panel with the two loose sources; panel_after.png is
taken once the sequence exists and is the active view, so the pair pins both the
new SEQUENCES row and the inputs panel listing the wrapped sources.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

# Add sources BEFORE activating.
src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "SeqSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "SeqSrc2")

rvc.setViewNode(src1)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

seqs_before = sm.tree_category_items(None, log=log).get("SEQUENCES", [])
log("SEQUENCES rows before", seqs_before)
panel_before = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "Sequence of SeqSrc1 and SeqSrc2")
rvc.setViewNode(seq)
pump(600)

log("sequence node", seq)
log("sequence inputs", rvc.nodeConnections(seq, False)[0])

assert rvc.nodeExists(seq), "RVSequenceGroup not created"
assert rvc.nodeType(seq) == "RVSequenceGroup", f"wrong type: {rvc.nodeType(seq)}"
inputs = rvc.nodeConnections(seq, False)[0]
assert src1 in inputs, f"{src1} not in sequence inputs: {inputs}"
assert src2 in inputs, f"{src2} not in sequence inputs: {inputs}"

tree_view = sm.find_tree_view(log=log)
cats = sm.tree_category_items(None, log=log)
assert "SEQUENCES" in cats, f"SEQUENCES not in tree: {list(cats.keys())}"
assert len(cats["SEQUENCES"]) == len(seqs_before) + 1, (
    f"SEQUENCES should gain one row: {seqs_before} -> {cats['SEQUENCES']}"
)

inputs_view = sm.find_inputs_view(log=log)
assert inputs_view is not None, "inputs view not found"
inp_nodes = sm.get_inputs_node_list(None, log=log)
log("inputs panel nodes", inp_nodes)
assert src1 in inp_nodes, f"{src1} not in inputs panel: {inp_nodes}"
assert src2 in inp_nodes, f"{src2} not in inputs panel: {inp_nodes}"

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "new sequence row + inputs list", log=log)
sm.save_session(out_dir, log=log)
diag.close()
