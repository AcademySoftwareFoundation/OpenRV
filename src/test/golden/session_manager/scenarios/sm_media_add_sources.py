"""Scenario: add multiple different media sources and verify tree structure (COVERAGE A1–A4, C9–C11)."""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

# Add all movieproc sources.
black_grp = sm.add_black_source(log=log)
bars_grp = sm.add_bars_source(log=log)
white_grp = sm.add_white_source(log=log)

# Load one real MP4.
clip = sm.SM_CLIP_1
if os.path.exists(clip):
    snode = rvc.addSourceVerbose([clip])
    sm.wait_for_progressive_loading(log=log)
    mp4_grp = rvc.nodeGroup(snode)
    log("loaded mp4 group", mp4_grp)
else:
    mp4_grp = None
    log("NOTE: MP4 clip not found, skipping real media test")

rvc.setViewNode(black_grp)
pump(300)

sm.activate_session_manager(log=log)
pump(500)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None
cats = sm.tree_category_items(None, log=log)
log("tree categories:", list(cats.keys()))
assert "SOURCES" in cats, f"SOURCES not in tree: {list(cats.keys())}"

sources = cats.get("SOURCES", [])
log("SOURCES entries:", sources)
expected_count = 4 if mp4_grp else 3
assert len(sources) >= expected_count, (
    f"expected {expected_count}+ sources in tree, got {len(sources)}: {sources}"
)

sm.grab_panel_png(out_dir, log=log)
sm.save_session(out_dir, log=log)
diag.close()
