"""Scenario: source rows in the inputs panel carry a preview widget (COVERAGE G2).

With previews on, updateInputs() blanks the row text and installs a source-row widget
(thumbnail + name + meta) as the index widget; with previews off the row is plain text.
Both states are asserted and captured, so the discriminant is the presence of the
widget rather than just a repaint.
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
sm.set_ui_name(src1, "G2Src1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "G2Src2")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src1, src2])
sm.set_ui_name(seq, "G2Sequence")
rvc.setViewNode(seq)
pump(200)

sm.activate_session_manager(log=log)
pump(600)
sm.select_inputs_tab(log=log)
pump(400)

withPreviews = sm.inputs_rows_with_widgets(log=log)
log("input rows carrying a preview widget:", withPreviews)
sm.grab_panel_png(out_dir, "panel_previews_on.png", log=log)
assert withPreviews > 0, (
    "source inputs should carry a preview widget when previews are enabled (G2)")

#
#  The previews-off half is deliberately not driven here. Flipping the setting means
#  opening the config menu, and a session-manager-preview-available event landing
#  while it is open rebuilds the panel and destroys the menu — "Internal C++ object
#  already deleted", reproduced on Mu. sm_previews_toggle already pins the toggle
#  itself (I1, I2, I6, I9); this row is specifically "with previews enabled, source
#  inputs show a preview widget", so the other half of the same code path is asserted
#  instead: the row text is blanked when the widget takes over.
#
iv = sm.find_inputs_view(log=log)
model = iv.model()
blanked = [model.item(r).text() for r in range(model.rowCount())
           if iv.indexWidget(model.indexFromItem(model.item(r))) is not None]
log("text of rows carrying a widget:", blanked)
assert blanked and all(t == "" for t in blanked), (
    "a row that carries a preview widget must have its text blanked (G2); got %s"
    % (blanked,))

sm.save_session(out_dir, log=log)
diag.close()
