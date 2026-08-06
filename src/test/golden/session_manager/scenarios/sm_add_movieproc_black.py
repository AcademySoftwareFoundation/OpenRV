"""Scenario: Add > Black… creates a black movieproc source (COVERAGE C9, primary #4).

A neutral base source keeps the panel alive for panel_before.png; panel_after.png
must show the extra SOURCES row for the new "Black" movieproc.
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

group = sm.add_black_source(log=log)
rvc.setViewNode(group)
pump(600)

assert rvc.nodeExists(group)
assert rvc.nodeType(group) == "RVSourceGroup"

def find_source_node(grp):
    for node in rvc.nodesInGroup(grp):
        if rvc.nodeType(node) in ("RVFileSource", "RVImageSource"):
            return node
    return None

snode = find_source_node(group)
assert snode is not None, f"no source node in group {group}"
media = rvc.getStringProperty(snode + ".media.movie")
log("media.movie", media)
assert len(media) > 0
assert "black" in media[0].lower() or "movieproc" in media[0].lower(), (
    f"expected black movieproc URL, got: {media}"
)

assert sm.get_ui_name(group) == "Black", f"expected 'Black', got '{sm.get_ui_name(group)}'"

tree_view = sm.find_tree_view(log=log)
cats = sm.tree_category_items(None, log=log)
assert "SOURCES" in cats
assert len(cats["SOURCES"]) == len(sources_before) + 1, (
    f"SOURCES should gain one row: {len(sources_before)} -> {len(cats['SOURCES'])}"
)

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "new Black source row", log=log)
sm.save_session(out_dir, log=log)
diag.close()
