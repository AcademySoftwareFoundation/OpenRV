"""Scenario: a blank movieproc source (COVERAGE C14).

Follows the same shape as the other movieproc scenarios: the source is created through
addSourceVerbose with the URL the Add > Blank dialog builds, so the outcome is pinned
without depending on a modal dialog. C15 (the dialog's FPS defaulting from
General/fps) is covered by unit/test_mode.py instead — a modal has no deterministic
golden, per VERIFICATION.md.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

group = sm.add_movieproc_source(
    "blank", "width=1280,height=720,fps=24,start=1,end=24", "Blank", log=log)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

src = sm.source_node_of_group(group)
assert src is not None, "no source node inside %s" % group
media = rvc.getStringProperty(src + ".media.movie")[0]
log("media:", media)

assert media.startswith("blank,"), "expected a blank movieproc, got %r" % media
assert media.endswith(".movieproc"), "expected a movieproc URL, got %r" % media
assert rvc.nodeType(group) == "RVSourceGroup"
assert sm.get_ui_name(group) == "Blank", "name should be Blank, got %r" % sm.get_ui_name(group)

cats = sm.tree_category_items(None, log=log)
assert "SOURCES" in cats, "SOURCES missing: %s" % list(cats)
log("SOURCES rows:", cats["SOURCES"])

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
