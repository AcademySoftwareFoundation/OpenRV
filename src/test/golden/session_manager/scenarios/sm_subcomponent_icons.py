"""Scenario: changing request.imageComponent updates the sub-component icons\n(COVERAGE M6).\n\nThe panel is never told to refresh here — the property is written straight onto the\nsource and graph-state-change has to drive the icon update. The discriminant is both\nthe committed property and the panel PNG differing between the two states."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

clips = sm.meridian_clips()
group = sm.add_source_verbose_group(clips[0]) if clips else None
if group is None:
    group = sm.add_bars_source(log=log)
sm.set_ui_name(group, "IcSrc")
pump(400)

rvc.setViewNode(group)
pump(200)
sm.activate_session_manager(log=log)
pump(600)

src = sm.source_node_of_group(group)
assert src is not None, "no source node in %s" % group
prop = src + ".request.imageComponent"
assert rvc.propertyExists(prop), "source has no request.imageComponent"

rvc.setStringProperty(prop, [], True)
pump(700)
before = list(rvc.getStringProperty(prop))
log("imageComponent before:", before)
sm.grab_panel_png(out_dir, "panel_unset.png", log=log)

rvc.setStringProperty(prop, ["view", "left"], True)
pump(900)
after = list(rvc.getStringProperty(prop))
log("imageComponent after:", after)
sm.grab_panel_png(out_dir, "panel_set.png", log=log)

assert after != before, "the request property must change (M6): %s -> %s" % (before, after)
assert after == ["view", "left"]

sm.save_session(out_dir, log=log)
diag.close()
