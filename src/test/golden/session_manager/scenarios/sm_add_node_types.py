"""Scenario: Add menu creates Layout / Retime / Color / OCIO (COVERAGE C4, C5, C6, C7).\n\nDriven through the real Add-button menu actions rather than the mode's slots, so the\nmenu wiring is pinned too and the scenario runs against either implementation."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "NtSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "NtSrc2")

sm.activate_session_manager(log=log)
pump(400)

add = sm.find_add_button(log=log)
assert add is not None, "addButton not found"

WANTED = {"Layout": "RVLayoutGroup", "Retime": "RVRetimeGroup", "Color": "RVColor"}
created = {}

for label, nodeType in WANTED.items():
    sm.select_tree_item_for_node(None, src1, log=log)
    pump(150)
    before = set(rvc.nodes())
    sm.trigger_menu_action(add, label, log=log)
    made = [n for n in sorted(set(rvc.nodes()) - before) if rvc.nodeType(n) == nodeType]
    assert made, "Add > %s created no %s (new: %s)" % (
        label, nodeType, sorted(set(rvc.nodes()) - before))
    created[nodeType] = made[0]
    log("Add >", label, "->", made[0], "uiName", sm.get_ui_name(made[0]))

for nodeType, node in created.items():
    assert rvc.nodeExists(node) and rvc.nodeType(node) == nodeType

#
#  C7, Add > OCIO, is deliberately NOT exercised here. Both implementations pass
#  "RVOCIO" to newNode and this build has no such node type (it ships OCIO /
#  OCIODisplay / OCIOFile / OCIOLook), so the action raises in either one — a
#  pre-existing package defect, not a port regression.
#
#  It cannot be pinned by a golden either: the raise produces a traceback naming
#  session_manager.py frames under Python and Mu frames under Mu, so gate 0 sees a
#  new signature whichever implementation captured the baseline. Recorded as a known
#  defect in COVERAGE.md instead.
#
assert "RVOCIO" not in rvc.nodeTypes(True), (
    "RVOCIO exists in this build now — Add > OCIO can work, so C7 should be pinned "
    "properly and this note removed")

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
