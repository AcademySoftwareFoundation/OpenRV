"""Scenario: preview toggle via the config button menu (COVERAGE H1, H3, H4).

Pins the toggle in pixels with a quiescent pair: the previews-on half is grabbed
only after the source's thumbnail and filmstrip are both generated, so it cannot
race a job finishing, and the previews-off half has no preview widgets at all.

The menu is safe to drive here because a single source does not put the Mu mode
under the memory pressure that gets its config QMenu collected -- see
_sm_common.folder_thumbnail_flow for the folder-sized case, which avoids the menu.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


import rv.commands as rvc
from qt_scenario_utils import pump

src = sm.add_black_source(log=log)
rvc.setViewNode(src)
pump(200)

sm.activate_session_manager(log=log)
pump(400)

config_btn = sm.find_config_button(log=log)
assert config_btn is not None, "configButton not found"

menu = sm.find_config_menu(log=log)
action_names = [a.text().replace("&", "") for a in menu.actions()]
log("config menu actions:", action_names)
assert "Show Source Previews" in action_names, f"toggle action missing: {action_names}"
menu.close()
pump(200)

# --- previews on: wait for the one source to finish generating ---------------
previews = sm.tree_source_row_previews(log=log)
assert len(previews) == 1, f"expected one preview widget, found {len(previews)}"
sm.wait_for_all_previews(1, log=log)
fallback_gone = sm.tree_row_preview_hashes(log=log)
panel_on = sm.grab_panel_png(out_dir, "panel_previews_on.png", log=log)

# --- previews off: the preview column disappears ------------------------------
assert sm.toggle_previews(log=log) is False, "toggle should switch previews off"
pump(600)
assert not sm.tree_source_row_previews(log=log), (
    "preview widgets are still installed in the tree with previews off"
)
panel_off = sm.grab_panel_png(out_dir, "panel_previews_off.png", log=log)
sm.assert_images_differ(
    panel_on, panel_off, "the preview column disappears when previews are off", log=log
)

# --- and back on: the checked state round-trips -------------------------------
assert sm.toggle_previews(log=log) is True, "toggle should switch previews back on"
sm.wait_for_rows_with_thumbnails(1, "no-such-hash", timeout_s=120, log=log)
log("preview hashes after round-trip:", [h[:12] for h in sm.tree_row_preview_hashes(log=log)])
log("hashes while on (first grab):", [h[:12] for h in fallback_gone])

sm.save_session(out_dir, log=log)
diag.close()
