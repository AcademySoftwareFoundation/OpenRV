"""Scenario: sort-asc/sort-desc buttons sort inputs alphabetically (COVERAGE G6, G7, G8, primary #7).

Three captures pin the visible order: unsorted, A-Z, then Z-A. A-Z and Z-A must
differ from each other, otherwise the sort direction is not actually pinned.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump, click_button

src_c = sm.add_black_source(log=log)
sm.set_ui_name(src_c, "Charlie")
src_a = sm.add_bars_source(log=log)
sm.set_ui_name(src_a, "Alpha")
src_b = sm.add_white_source(log=log)
sm.set_ui_name(src_b, "Bravo")

seq = rvc.newNode("RVSequenceGroup", "")
rvc.setNodeInputs(seq, [src_c, src_a, src_b])
sm.set_ui_name(seq, "SortSeq")
rvc.setViewNode(seq)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

inputs_view = sm.find_inputs_view(log=log)
assert inputs_view is not None

sm.select_inputs_tab(log=log)
unsorted_names = [sm.get_ui_name(n) for n in rvc.nodeConnections(seq, False)[0]]
log("unsorted order", unsorted_names)
assert unsorted_names == ["Charlie", "Alpha", "Bravo"], f"unexpected start: {unsorted_names}"
panel_before = sm.grab_panel_png(out_dir, "panel_before.png", log=log)

asc_btn = sm.find_sort_asc_button(log=log)
assert asc_btn is not None, "sortAscButton not found"
click_button(asc_btn, settle_ms=400)
pump(400)

asc_order = rvc.nodeConnections(seq, False)[0]
log("asc order", [sm.get_ui_name(n) for n in asc_order])
names_asc = [sm.get_ui_name(n) for n in asc_order]
assert names_asc == sorted(names_asc), f"expected ascending sort, got: {names_asc}"
panel_asc = sm.grab_panel_png(out_dir, "panel_asc.png", log=log)
sm.assert_images_differ(panel_before, panel_asc, "inputs sorted A-Z", log=log)

desc_btn = sm.find_sort_desc_button(log=log)
assert desc_btn is not None
click_button(desc_btn, settle_ms=400)
pump(400)

desc_order = rvc.nodeConnections(seq, False)[0]
names_desc = [sm.get_ui_name(n) for n in desc_order]
log("desc order", names_desc)
assert names_desc == sorted(names_desc, reverse=True), f"expected descending, got: {names_desc}"

panel_desc = sm.grab_panel_png(out_dir, "panel_desc.png", log=log)
sm.assert_images_differ(panel_asc, panel_desc, "inputs sorted Z-A", log=log)
sm.save_session(out_dir, log=log)
diag.close()
