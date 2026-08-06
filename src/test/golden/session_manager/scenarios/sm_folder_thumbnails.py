"""Scenario: load a folder of mp4s and pin the fully-generated thumbnail panel
(COVERAGE I2, I5, I7, I8, I9, M7, primary #8).

Loads the first SM_FOLDER_CLIP_COUNT clips (12) rather than the whole fixture
folder: generation is two rvio jobs per clip at MAX_WORKERS=2 and this scenario
runs once per gate, so the full 83-clip folder is covered separately by
run_folder_thumbnails_all.sh once the gates pass. Both use the same flow.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


sm.folder_thumbnail_flow(out_dir, clip_limit=sm.SM_FOLDER_CLIP_COUNT, log=log)
diag.close()
