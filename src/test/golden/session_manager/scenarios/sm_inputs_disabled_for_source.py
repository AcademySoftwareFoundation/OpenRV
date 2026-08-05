"""Scenario: the inputs panel is disabled for source nodes (COVERAGE G9).\n\nA source has no editable inputs, so selecting one must leave the inputs view\ndisabled, while a sequence re-enables it. Both states are captured as PNGs so the\ndifference is pinned visually as well as by the widget flag."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "IdSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "IdSrc2")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "IdSequence")
rvc.setViewNode(seq)
pump(200)

sm.activate_session_manager(log=log)
pump(400)
sm.select_inputs_tab(log=log)

iv = sm.find_inputs_view(log=log)
assert iv is not None, "inputs view not found"

# A sequence has editable inputs.
rvc.setViewNode(seq)
pump(600)
enabledForSequence = iv.isEnabled()
log("inputs view enabled for sequence:", enabledForSequence)
sm.grab_panel_png(out_dir, "panel_sequence.png", log=log)

# A source does not.
rvc.setViewNode(src1)
pump(600)
enabledForSource = iv.isEnabled()
log("inputs view enabled for source:", enabledForSource)
sm.grab_panel_png(out_dir, "panel_source.png", log=log)

assert enabledForSequence, "the inputs panel should be usable for a sequence"
assert not enabledForSource, (
    "the inputs panel must be disabled for an RVSourceGroup (G9)")

sm.save_session(out_dir, log=log)
diag.close()
