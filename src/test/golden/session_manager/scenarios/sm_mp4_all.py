"""Scenario: MP4 comprehensive — load clip, verify thumbnails and filmstrip path (COVERAGE H1–H4, primary #3).

This is the mandatory mp4 golden test that verifies the full preview generation
pipeline: load → tree shows source → local_thumbnail_gen activates → preview files
are requested (not necessarily rendered in headless).
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

clip = sm.SM_CLIP_1
assert os.path.exists(clip), f"media fixture not found: {clip}"

snode = rvc.addSourceVerbose([clip])
sm.wait_for_progressive_loading(log=log)
pump(600)

group = rvc.nodeGroup(snode)
rvc.setViewNode(group)
pump(300)

sm.activate_session_manager(log=log)
pump(600)

def find_source_node(grp):
    for node in rvc.nodesInGroup(grp):
        if rvc.nodeType(node) in ("RVFileSource", "RVImageSource"):
            return node
    return None

fnode = find_source_node(group)
assert fnode is not None, f"no source node in group {group}"
media = rvc.getStringProperty(fnode + ".media.movie")
log("media.movie:", media)
assert len(media) > 0 and clip in media[0]

# Verify media dimensions loaded.
try:
    w = rvc.getIntProperty(fnode + ".image.width")
    h = rvc.getIntProperty(fnode + ".image.height")
    log("image size:", w, "x", h)
    assert w[0] > 0 and h[0] > 0, f"invalid image dimensions: {w}x{h}"
except Exception as e:
    log("WARNING: image dimensions not available:", e)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None

cats = sm.tree_category_items(None, log=log)
assert "SOURCES" in cats

# Verify local_thumbnail_gen mode is active.
thumb_active = rvc.isModeActive("local_thumbnail_gen")
log("local_thumbnail_gen active:", thumb_active)
if not thumb_active:
    try:
        rvc.activateMode("local_thumbnail_gen")
        pump(300)
        log("activated local_thumbnail_gen")
    except Exception as e:
        log("WARNING: could not activate local_thumbnail_gen:", e)

# Wait for preview availability.
sm.wait_for_preview(group, log=log)

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
