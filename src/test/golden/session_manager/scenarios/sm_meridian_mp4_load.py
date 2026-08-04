"""Scenario: load an MP4 clip and verify the source appears in the tree (COVERAGE H2, primary #3)."""
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
pump(500)

group = rvc.nodeGroup(snode)
rvc.setViewNode(group)
pump(200)

sm.activate_session_manager(log=log)
pump(500)

def find_source_node(grp):
    for node in rvc.nodesInGroup(grp):
        if rvc.nodeType(node) in ("RVFileSource", "RVImageSource"):
            return node
    return None

fnode = find_source_node(group)
assert fnode is not None
media = rvc.getStringProperty(fnode + ".media.movie")
log("loaded media:", media)
assert len(media) > 0
assert clip in media[0] or os.path.basename(clip).split(".")[0] in media[0], (
    f"unexpected media URL: {media}"
)

# loadTotal may already be 0 if the file loaded quickly; that's fine.
total = rvc.loadTotal()
log("loadTotal (may be 0 if fast-load):", total)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None
cats = sm.tree_category_items(None, log=log)
assert "SOURCES" in cats, f"SOURCES not in tree: {list(cats.keys())}"

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
