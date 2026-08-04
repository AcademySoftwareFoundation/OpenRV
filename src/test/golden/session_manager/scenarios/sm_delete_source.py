"""Scenario: delete source via delete button (COVERAGE E1, M2, primary #3).

panel_before.png has both sources listed under SOURCES; panel_after.png is taken
after the real delete button removes "Goner", so the pair pins the row actually
disappearing rather than just the node leaving the graph.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump, click_button

src_keep = sm.add_black_source(log=log)
sm.set_ui_name(src_keep, "Keeper")
src_del = sm.add_bars_source(log=log)
sm.set_ui_name(src_del, "Goner")
rvc.setViewNode(src_keep)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

tree_view = sm.find_tree_view(log=log)
assert tree_view is not None

ok = sm.select_tree_item_for_node(None, src_del, log=log)
assert ok, f"could not select {src_del}"
pump(300)

del_btn = sm.find_delete_button(log=log)
assert del_btn is not None, "deleteButton not found"
assert del_btn.isEnabled(), "delete button should be enabled when node is selected"

sources_before = sm.tree_category_items(None, log=log).get("SOURCES", [])
log("SOURCES rows before delete:", len(sources_before))
panel_before = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

click_button(del_btn, settle_ms=600)
pump(600)

assert not rvc.nodeExists(src_del), f"{src_del} should be deleted after clicking delete"
assert rvc.nodeExists(src_keep), f"{src_keep} should still exist"
log("deleted", src_del, "Keeper still exists:", rvc.nodeExists(src_keep))

sources_after = sm.tree_category_items(None, log=log).get("SOURCES", [])
log("SOURCES rows after delete:", len(sources_after))
assert len(sources_after) == len(sources_before) - 1, (
    f"tree should lose exactly one SOURCES row: {len(sources_before)} -> {len(sources_after)}"
)

panel_after = sm.grab_panel_png(out_dir, "panel_after.png", log=log)
sm.assert_images_differ(panel_before, panel_after, "deleted row disappears", log=log)
sm.save_session(out_dir, log=log)
diag.close()
