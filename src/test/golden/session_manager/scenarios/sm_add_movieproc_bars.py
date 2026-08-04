"""Scenario: Add > Color Bars… creates smptebars movieproc source (COVERAGE C11, primary #4).

Before/after pair around adding the bars source pins the new SOURCES row.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

base = sm.add_base_source(log=log)
rvc.setViewNode(base)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

sources_before = sm.tree_category_items(None, log=log).get("SOURCES", [])
log("SOURCES rows before", len(sources_before))
panel_before = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

group = sm.add_bars_source(log=log)
rvc.setViewNode(group)
pump(600)

def find_source_node(grp):
    for node in rvc.nodesInGroup(grp):
        if rvc.nodeType(node) in ("RVFileSource", "RVImageSource"):
            return node
    return None

snode = find_source_node(group)
assert snode is not None
media = rvc.getStringProperty(snode + ".media.movie")
log("media.movie", media)
assert "smptebars" in media[0].lower()
assert sm.get_ui_name(group) == "SMPTEBars"

cats = sm.tree_category_items(None, log=log)
assert len(cats.get("SOURCES", [])) == len(sources_before) + 1, (
    f"SOURCES should gain one row: {len(sources_before)} -> {len(cats.get('SOURCES', []))}"
)

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "new SMPTEBars source row", log=log)
sm.save_session(out_dir, log=log)
diag.close()
