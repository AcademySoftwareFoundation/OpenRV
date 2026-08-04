"""Scenario: Add > Color… creates a solid-color movieproc source (COVERAGE C10, C16, primary #4).

Before/after pair around adding the chosen-colour source, so the new SOURCES row
is pinned in pixels and not just in the graph.
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

group = sm.add_movieproc_source(
    "solid",
    "width=1280,height=720,fps=24,start=1,end=24,red=0.502,green=0.502,blue=0.502",
    "SolidColor",
    log=log,
)
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
assert snode is not None
media = rvc.getStringProperty(snode + ".media.movie")
log("media.movie", media)
assert "solid" in media[0].lower()
assert "red=0.502" in media[0], f"chosen colour not in media URL: {media[0]}"
assert sm.get_ui_name(group) == "SolidColor"

cats = sm.tree_category_items(None, log=log)
assert len(cats.get("SOURCES", [])) == len(sources_before) + 1, (
    f"SOURCES should gain one row: {len(sources_before)} -> {len(cats.get('SOURCES', []))}"
)

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "new SolidColor source row", log=log)
sm.save_session(out_dir, log=log)
diag.close()
