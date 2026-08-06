"""Scenario: the tree and inputs panel follow graph events (COVERAGE M1, M4).\n\nNothing here asks the panel to refresh — the graph is changed directly and the panel\nhas to react to new-node and graph-node-inputs-changed on its own."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

sm.activate_session_manager(log=log)
pump(400)

# M1 — new-node grows the tree with no explicit refresh.
nBefore = len(sm.tree_category_items(None, log=log).get("SOURCES", []))
src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "EvSrc1")
pump(600)
nAfter = len(sm.tree_category_items(None, log=log).get("SOURCES", []))
log("SOURCES rows", nBefore, "->", nAfter)
assert nAfter > nBefore, "new-node did not grow the tree (M1)"

src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "EvSrc2")
pump(600)

# M4 — graph-node-inputs-changed refreshes the inputs panel for the view node.
seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1])
sm.set_ui_name(seq, "EvSequence")
rvc.setViewNode(seq)
pump(600)
sm.select_inputs_tab(log=log)
inputsBefore = sm.get_inputs_node_list(log=log)
log("inputs before:", inputsBefore)

rvc.setNodeInputs(seq, [src1, src2])
pump(800)
inputsAfter = sm.get_inputs_node_list(log=log)
log("inputs after:", inputsAfter)
assert len(inputsAfter) > len(inputsBefore), (
    "graph-node-inputs-changed did not refresh the inputs panel (M4): %s -> %s"
    % (inputsBefore, inputsAfter))

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
