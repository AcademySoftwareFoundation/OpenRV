"""Scenario: Add > Switch creates RVSwitchGroup (COVERAGE C3)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src1 = sm.add_black_source(log=log)
sm.set_ui_name(src1, "SwSrc1")
src2 = sm.add_bars_source(log=log)
sm.set_ui_name(src2, "SwSrc2")

sw = rvc.newNode("RVSwitchGroup", "")
rvc.setNodeInputs(sw, [src1, src2])
sm.set_ui_name(sw, "Switch of SwSrc1 and SwSrc2")
rvc.setViewNode(sw)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

assert rvc.nodeExists(sw)
assert rvc.nodeType(sw) == "RVSwitchGroup"
inputs = rvc.nodeConnections(sw, False)[0]
assert src1 in inputs
log("switch created", sw, "inputs", inputs)

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
