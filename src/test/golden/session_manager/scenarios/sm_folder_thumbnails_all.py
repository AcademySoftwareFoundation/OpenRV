"""Scenario: load *every* clip in the fixture folder and pin the fully-generated
thumbnail panel (COVERAGE I2, I5, I7, I8, I9, M7, primary #8 at full scale).

Excluded from the gated suite by SKIP_IDS because 83 clips means 166 rvio jobs at
MAX_WORKERS=2; run_folder_thumbnails_all.sh runs it as a mandatory check after all
six gates pass. Same flow as sm_folder_thumbnails, no clip limit.
"""
import os
import _sm_common as sm

out_dir = os.environ["GOLDEN_OUT"]
diag = open(os.path.join(out_dir, "diag.txt"), "w")


def log(*a):
    print(*a, file=diag, flush=True)


sm.folder_thumbnail_flow(out_dir, clip_limit=None, log=log)
diag.close()
