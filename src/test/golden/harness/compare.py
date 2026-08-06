#!/usr/bin/env python3
"""Comparators for golden tests: behavioral (GTO node graph) and pixel (PNG).

Behavioral gate:
    Normalize a captured text-GTO session and compare it byte-for-byte against
    the stored golden. Normalization removes environment-specific noise (absolute
    paths, a few volatile session-header fields) so the same graph compares equal
    across machines/checkouts. The default (empty) session is already
    byte-identical run-to-run, so normalization is a no-op there; the hooks exist
    for later media-bearing scenarios.

Pixel gate:
    Thin wrapper over the built `rmsImageDiff -cmp -dmax <v>` tool (whole-image,
    no ROI). Gate at 0 (exact) on the pinned Xvfb + software-Mesa path.

CLI:
    compare.py --golden-dir DIR --actual-dir DIR [--dmax 0]
Expects `session.rv` in each dir; compares `panel.png` too if present in both.
"""

import argparse
import os
import re
import subprocess
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(_HERE, "..", "..", "..", ".."))


def _find_rms_image_diff() -> str:
    """Locate the built rmsImageDiff binary across staging layouts.

    Linux stages flat under app/bin/; the macOS build only stages inside the
    .app bundle (verified 2026-07-24: no app/bin/ exists on a Mac build at
    all). RMS_IMAGE_DIFF overrides both for a nonstandard layout.
    """
    override = os.environ.get("RMS_IMAGE_DIFF")
    if override:
        return override
    candidates = [
        os.path.join(REPO_ROOT, "_build", "stage", "app", "bin", "rmsImageDiff"),
        os.path.join(REPO_ROOT, "_build", "stage", "app", "RV.app", "Contents", "MacOS", "rmsImageDiff"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    return candidates[0]  # preserve prior default for the "not found" error message


RMS_IMAGE_DIFF = _find_rms_image_diff()

# Session-header properties that reflect UI/playback state rather than graph
# structure; drop whole GTO property lines whose name matches, so they can't
# cause spurious behavioral diffs.
_VOLATILE_PROP_RE = re.compile(r"^\s*(string sessionName|int currentFrame|int\[\] marks)\b")
# sRGB2linear is set by source_setup / rendering path (Xvfb vs real GPU), not
# by session_manager graph logic. Only relaxed for the GUI sanity gate, which
# compares real-display output against Linux Xvfb-captured golden/ baselines
# that predate GOLDEN_SOURCE_SETUP=1 -- see run_gui_sanity_gate.sh.
_RENDER_PATH_PROP_RE = re.compile(r"^\s*int (sRGB2linear|Rec709ToLinear)\b")
# Absolute media paths vary by machine; canonicalize to a stable token.
_MOVIE_LINE_RE = re.compile(r'^(\s*string movie = ")([^"]+)("\s*)$')
_FIXTURE_SUFFIX = "/src/test/golden/session_manager/fixtures/"
_SESSION_BLOCK_START = re.compile(r"^\s{4}session\s*$")
_SESSION_BLOCK_END = re.compile(r"^\s{4}\}\s*$")


def _canonicalize_movie_path(path: str) -> str:
    """Map any checkout's absolute fixture path to a stable <REPO>/... token."""
    if path in ("<MP4_FIXTURE>", "<REPO_FIXTURE>"):
        return path
    if path.endswith(".mp4") or path.endswith(".mov"):
        return "<MP4_FIXTURE>"
    if path.endswith(".exr"):
        return "<LAYER_EXR_FIXTURE>"
    idx = path.find("/src/test/golden/layer_select/fixtures/")
    if idx >= 0:
        return "<REPO>" + path[idx:]
    idx = path.find(_FIXTURE_SUFFIX)
    if idx >= 0:
        return "<REPO>" + path[idx:]
    home = os.path.expanduser("~")
    return path.replace(REPO_ROOT, "<REPO>").replace(home, "<HOME>")


def normalize_gto(
    text: str,
    *,
    relax_render_path: bool = False,
    relax_session_playback: bool = False,
) -> str:
    """Return a canonical form of a text-GTO session for comparison."""
    out_lines = []
    in_session_block = False
    session_depth = 0
    for line in text.splitlines():
        if _VOLATILE_PROP_RE.match(line):
            continue
        if relax_render_path and _RENDER_PATH_PROP_RE.match(line):
            continue
        if _SESSION_BLOCK_START.match(line):
            in_session_block = True
            session_depth = 0
            out_lines.append(line)
            continue
        if in_session_block and relax_session_playback:
            if line.strip() == "{":
                session_depth += 1
                out_lines.append(line)
                continue
            if line.strip() == "}":
                session_depth -= 1
                if session_depth <= 0:
                    in_session_block = False
                    session_depth = 0
                out_lines.append(line)
                continue
            if session_depth == 1 and re.match(
                r"^\s{8}(string viewNode|int\[2\] range|int\[2\] region|float fps)\b", line
            ):
                continue
        elif in_session_block and line.strip() == "}":
            in_session_block = False
            session_depth = 0
        m = _MOVIE_LINE_RE.match(line)
        if m:
            canon = _canonicalize_movie_path(m.group(2))
            line = "%s%s%s" % (m.group(1), canon, m.group(3))
        else:
            home = os.path.expanduser("~")
            line = line.replace(REPO_ROOT, "<REPO>").replace(home, "<HOME>")
        out_lines.append(line)
    return "\n".join(out_lines) + "\n"


def compare_gto(
    golden_path: str,
    actual_path: str,
    *,
    relax_render_path: bool = False,
    relax_session_playback: bool = False,
) -> tuple[bool, str]:
    norm_kw = {
        "relax_render_path": relax_render_path,
        "relax_session_playback": relax_session_playback,
    }
    with open(golden_path, "r") as f:
        g = normalize_gto(f.read(), **norm_kw)
    with open(actual_path, "r") as f:
        a = normalize_gto(f.read(), **norm_kw)
    if g == a:
        return True, "behavioral: MATCH"
    # Produce a short unified diff for the report.
    import difflib

    diff = "\n".join(
        difflib.unified_diff(
            g.splitlines(),
            a.splitlines(),
            fromfile="golden",
            tofile="actual",
            lineterm="",
            n=2,
        )
    )
    return False, "behavioral: MISMATCH\n" + diff


def compare_png(golden_png: str, actual_png: str, dmax: float) -> tuple[bool, str]:
    if not os.path.isfile(RMS_IMAGE_DIFF):
        return False, f"pixel: rmsImageDiff not found at {RMS_IMAGE_DIFF}"
    # Parse stdout's verdict line. The exit code is NOT usable: rmsImageDiff -cmp
    # exits 0 for a mismatch as well as a match -- it only returns non-zero when it
    # cannot read or compare the files at all (e.g. 255 for "channel size does not
    # match"). An ad-hoc `rmsImageDiff -cmp ... && echo same` therefore reports every
    # mismatch as a match, which is exactly how a real pixel-gate failure got
    # misread as flakiness once. Check for "Images are matched." and nothing else.
    #
    # Do not pass -m alongside -cmp: in the per-pixel loop, -m's branch shadows
    # -cmp's comparison entirely, so the dmax check would never run.
    proc = subprocess.run(
        [RMS_IMAGE_DIFF, "-cmp", "-dmax", str(dmax), golden_png, actual_png],
        capture_output=True,
        text=True,
    )
    stdout = proc.stdout.strip()
    ok = "Images are matched." in stdout
    return ok, f"pixel: {'MATCH' if ok else 'MISMATCH'} (dmax={dmax})\n{stdout}"


def report_png(golden_png: str, actual_png: str) -> str:
    """Non-gating pixel report: RMS + max-diff location/values, no verdict.

    Used by the GUI sanity gate, which has no scripted pixel pass/fail --
    real GPU/font/compositor rendering is never byte-identical to the pinned
    Xvfb+software-Mesa goldens, so a threshold here would either mask real
    regressions (too loose) or flag rendering noise as failures forever (too
    tight). Instead this prints quantitative info plus both PNG paths for a
    human or an AI reviewer to look at and judge -- see
    ../VERIFICATION.md#gui-sanity-gate-real-display.
    """
    if not os.path.isfile(RMS_IMAGE_DIFF):
        return f"pixel: rmsImageDiff not found at {RMS_IMAGE_DIFF}"
    proc = subprocess.run(
        [RMS_IMAGE_DIFF, "-m", golden_png, actual_png],
        capture_output=True,
        text=True,
    )
    stdout = proc.stdout.strip()
    return f"pixel: INFO (no threshold -- review required)\n{stdout}\ngolden={golden_png}\nactual={actual_png}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--golden-dir", required=True, help="Pixel baseline dir (and behavioral if --behavioral-golden-dir omitted)")
    ap.add_argument(
        "--behavioral-golden-dir",
        default=None,
        help="Behavioral session.rv baseline (defaults to --golden-dir). GUI sanity "
        "gate on macOS passes golden-mac/ here while pixel report still uses golden/.",
    )
    ap.add_argument("--actual-dir", required=True)
    ap.add_argument("--dmax", type=float, default=0.0)
    ap.add_argument(
        "--pixel-mode",
        choices=("gate", "report"),
        default="gate",
        help="gate (default): -cmp at --dmax, a mismatch fails the run (used by "
        "run_all_goldens.sh). report: no threshold, no verdict -- print RMS/"
        "max-diff + both PNG paths for a human/AI to judge (used by "
        "run_gui_sanity_gate.sh); never contributes to the exit code.",
    )
    ap.add_argument(
        "--relax-render-path",
        action="store_true",
        help="Drop int sRGB2linear lines (GUI sanity gate only -- real GPU vs Xvfb "
        "baseline drift, not session_manager graph logic).",
    )
    ap.add_argument(
        "--relax-session-playback",
        action="store_true",
        help="Drop session-block viewNode/range/region (GUI sanity gate only -- "
        "Linux-vs-Mac capture timing drift, not graph structure).",
    )
    args = ap.parse_args()
    relax_render_path = args.relax_render_path or os.environ.get("COMPARE_RELAX_RENDER_PATH") == "1"
    relax_session_playback = (
        args.relax_session_playback or os.environ.get("COMPARE_RELAX_SESSION_PLAYBACK") == "1"
    )

    results = []
    ok_all = True

    behavioral_golden = args.behavioral_golden_dir or args.golden_dir
    g_sess = os.path.join(behavioral_golden, "session.rv")
    a_sess = os.path.join(args.actual_dir, "session.rv")
    if os.path.isfile(g_sess) and os.path.isfile(a_sess):
        ok, msg = compare_gto(
            g_sess,
            a_sess,
            relax_render_path=relax_render_path,
            relax_session_playback=relax_session_playback,
        )
        ok_all &= ok
        results.append(msg)
    else:
        ok_all = False
        results.append(
            f"behavioral: missing session.rv (golden={os.path.isfile(g_sess)}, actual={os.path.isfile(a_sess)})"
        )

    # Compare every PNG artifact present in the golden dir (not just
    # panel.png -- popup-menu scenarios grab their own top-level window,
    # e.g. configmenu.png). A golden PNG with no matching actual PNG is a
    # hard FAIL, not a silently-skipped gate, in BOTH modes: a broken port
    # that can't find the widget (and so never writes the artifact) must not
    # pass, even under the report-only GUI sanity gate -- whether the artifact
    # exists at all is objective, only its pixel content is left to review.
    golden_pngs = (
        sorted(f for f in os.listdir(args.golden_dir) if f.endswith(".png")) if os.path.isdir(args.golden_dir) else []
    )
    for name in golden_pngs:
        g_png = os.path.join(args.golden_dir, name)
        a_png = os.path.join(args.actual_dir, name)
        if not os.path.isfile(a_png):
            ok_all = False
            results.append(f"pixel: missing actual {name} (golden exists)")
            continue
        if args.pixel_mode == "report":
            results.append(f"[{name}] {report_png(g_png, a_png)}")
        else:
            ok, msg = compare_png(g_png, a_png, args.dmax)
            ok_all &= ok
            results.append(f"[{name}] {msg}")
    # (If the golden dir has no PNGs at all, the pixel gate simply isn't
    # exercised for this scenario.)

    print("\n".join(results))
    print("RESULT:", "PASS" if ok_all else "FAIL")
    return 0 if ok_all else 1


if __name__ == "__main__":
    sys.exit(main())
