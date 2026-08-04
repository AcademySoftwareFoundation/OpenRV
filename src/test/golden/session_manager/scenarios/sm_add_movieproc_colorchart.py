"""Scenario: Add > SRGB + ACES Color Chart creates colorchart movieproc sources (COVERAGE C12, C13, primary #4).

Before/after pair around adding both chart sources: SOURCES must gain two rows.
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

srgb_group = sm.add_colorchart_source(log=log)
pump(300)

aces_group = sm.add_movieproc_source(
    "acescolorchart",
    "width=1280,height=720,fps=24,start=1,end=24",
    "ACESColorChart",
    log=log,
)
rvc.setViewNode(srgb_group)
pump(600)

def find_source_node(grp):
    for node in rvc.nodesInGroup(grp):
        if rvc.nodeType(node) in ("RVFileSource", "RVImageSource"):
            return node
    return None

snode_srgb = find_source_node(srgb_group)
snode_aces = find_source_node(aces_group)
assert snode_srgb is not None
assert snode_aces is not None

media_srgb = rvc.getStringProperty(snode_srgb + ".media.movie")
media_aces = rvc.getStringProperty(snode_aces + ".media.movie")
log("srgb media", media_srgb)
log("aces media", media_aces)
assert "srgbcolorchart" in media_srgb[0].lower(), f"expected srgbcolorchart, got: {media_srgb}"
assert "acescolorchart" in media_aces[0].lower(), f"expected acescolorchart, got: {media_aces}"

tree_view = sm.find_tree_view(log=log)
cats = sm.tree_category_items(None, log=log)
assert "SOURCES" in cats
assert len(cats["SOURCES"]) == len(sources_before) + 2, (
    f"SOURCES should gain two rows: {len(sources_before)} -> {len(cats['SOURCES'])}"
)

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "two new colour-chart rows", log=log)
sm.save_session(out_dir, log=log)
diag.close()
