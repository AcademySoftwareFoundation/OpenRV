"""Scenario: the per-type editor tab loads on a view change (COVERAGE J4).

Selecting a node fires view-edit-mode-activated, which the matching sibling edit mode
answers by building its .ui and calling addEditor(). Nothing in the session manager
asks for this — it is the mode manager activating the per-type mode — so the
discriminant is the editor tree gaining a named editor for the new view type.
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
sm.set_ui_name(src1, "J4Src1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "J4Src2")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "J4Sequence")

stack = rvc.newNode("RVStackGroup", "")
rvc.setNodeInputs(stack, [src1, src2])
sm.set_ui_name(stack, "J4Stack")

rvc.setViewNode(src1)
pump(200)
sm.activate_session_manager(log=log)
pump(600)

editorsForSource = sm.editor_tab_names(log=log)
log("editors with a source in view:", editorsForSource)
sm.grab_panel_png(out_dir, "panel_source.png", log=log)

rvc.setViewNode(seq)
pump(1200)
editorsForSequence = sm.editor_tab_names(log=log)
log("editors with a sequence in view:", editorsForSequence)
sm.grab_panel_png(out_dir, "panel_sequence.png", log=log)

rvc.setViewNode(stack)
pump(1200)
editorsForStack = sm.editor_tab_names(log=log)
log("editors with a stack in view:", editorsForStack)

assert editorsForSequence != editorsForSource or editorsForStack != editorsForSequence, (
    "view-edit-mode-activated must load a per-type editor (J4); editors never "
    "changed: source=%s sequence=%s stack=%s"
    % (editorsForSource, editorsForSequence, editorsForStack))
assert any("Sequence" in n for n in editorsForSequence) or \
       any("Stack" in n for n in editorsForStack), (
    "expected a Sequence or Stack editor to appear; got %s / %s"
    % (editorsForSequence, editorsForStack))

sm.save_session(out_dir, log=log)
diag.close()
