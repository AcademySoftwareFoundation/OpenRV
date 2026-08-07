#!/usr/bin/env python3
# Copyright (c) 2026 Autodesk Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
"""Analyze an OpenRV playback diagnostics log to attribute stuttering to frame
(image/EXR) decoding vs audio decoding/starvation vs both.

It ingests the CSV file produced when OpenRV is run with the RV_PLAYBACK_DIAG
environment variable set (default name: rv-playback-diag.log), and optionally an
.rvprof file written by `-debug profile`.

Standard library only -- no third party dependencies.

Usage:
    python3 analyze_playback_diag.py [rv-playback-diag.log] --fps 24 [--rvprof FILE]

Diagnostic log row schema:
    t_ms,event,thread,frame,dur_ms,extra
Events:
    decode      - one background image decode completed (dur_ms = decode time)
    cachemiss   - audio look-ahead cache had no data ready (silence emitted)
    audiolocked - audio fill was locked out and skipped (silence emitted)
    underrun    - ALSA hardware underrun / xrun
    buffering   - playback paused because the frame cache under-ran
    resume      - playback resumed after a buffering pause
    skip        - realtime playback dropped one or more frames
    display     - one on-screen frame was shown (dur_ms = evaluate + render);
                  extra: eval=<ms>;render=<ms>;threadedUpload=<0|1>;
                  interval=<ms since prev redraw>;dframe=<frame delta>;
                  skipped=<n>;refreshes=<redraws prev frame was held>;
                  shift=<m_shift>;elapsed=<play seconds>;
                  reqs=<heartbeat redraw requests coalesced into this paint>;
                  outsideGap=<ms spent outside render_v2: present + event loop>
    paint       - one GLView::paintGL pass; extra: paint=<whole paintGL ms>;
                  gap=<ms between prev paint-exit and this paint-entry:
                  Qt composite + swapBuffers/vsync + event loop>;
                  gpuFinish=<glFinish() ms after render, RV_DIAG_GLFINISH only>
    perrender   - per-render-event-processing handler duration (GUI-thread
                  Mu/Python work run between paints); dur_ms = handler time
    decsrc      - one media decode (extra reports file/type/bit-depth/size)
    displaycache- per presented frame: was it resident in cache? extra:
                  hit=<0|1>;full=<pct>;overflow=<0|1>;runway=<n>;used;cap;
                  otfDecodeMs=<ms> (on-the-fly decode time on a miss)
    cachestall  - caching gave up while cache overflowing; extra:
                  cacheFrame;cacheUtil;freeFrame;freeUtil;displayFrame
    upload      - one ImageRenderer::uploadPlane() call; dur_ms = CPU submit
                  time; extra: w;h;ch;type;pxsz;mb;pbo=<0|1>;update=<0|1>;
                  cpuGBps (effective CPU-submit throughput)
"""

import argparse
import csv
import os
import sys
from collections import defaultdict


def percentile(sorted_values, pct):
    """Linear-interpolated percentile of an already-sorted list."""
    if not sorted_values:
        return 0.0
    if len(sorted_values) == 1:
        return sorted_values[0]
    rank = (pct / 100.0) * (len(sorted_values) - 1)
    low = int(rank)
    high = min(low + 1, len(sorted_values) - 1)
    frac = rank - low
    return sorted_values[low] * (1.0 - frac) + sorted_values[high] * frac


def mean(values):
    return sum(values) / len(values) if values else 0.0


def median(sorted_values):
    return percentile(sorted_values, 50.0)


def merged_interval_length(intervals):
    """Total length of the union of [start, end] intervals (same unit in/out)."""
    if not intervals:
        return 0.0
    ordered = sorted(intervals)
    total = 0.0
    cur_start, cur_end = ordered[0]
    for start, end in ordered[1:]:
        if start > cur_end:
            total += cur_end - cur_start
            cur_start, cur_end = start, end
        else:
            cur_end = max(cur_end, end)
    total += cur_end - cur_start
    return total


def read_diag_log(path):
    """Return (rows, header_ok). Each row is a dict with typed fields."""
    rows = []
    with open(path, newline="") as fh:
        reader = csv.reader(fh)
        try:
            header = next(reader)
        except StopIteration:
            return rows
        # Tolerate a missing header (older logs) by detecting a numeric first col.
        if header and header[0].strip() != "t_ms":
            fh.seek(0)
            reader = csv.reader(fh)
        for raw in reader:
            if len(raw) < 5:
                continue
            try:
                row = {
                    "t_ms": float(raw[0]),
                    "event": raw[1].strip(),
                    "thread": int(raw[2]),
                    "frame": int(raw[3]),
                    "dur_ms": float(raw[4]),
                    "extra": raw[5].strip() if len(raw) > 5 else "",
                }
            except ValueError:
                continue
            rows.append(row)
    return rows


def analyze_decode(decode_rows, fps, span_s):
    """Per-thread and aggregate image-decode statistics."""
    per_thread = defaultdict(list)
    for r in decode_rows:
        per_thread[r["thread"]].append(r["dur_ms"])

    frame_budget_ms = 1000.0 / fps if fps > 0 else 0.0

    print("=" * 72)
    print("IMAGE (EXR) DECODE  --  measured on background caching threads")
    print("=" * 72)
    if not decode_rows:
        print("  No decode events recorded.")
        print("  (Either the sequence was already fully cached, or caching never ran.)")
        return {
            "count": 0,
            "active_fps": 0.0,
            "measured_fps": 0.0,
            "concurrency": 0.0,
            "mean_ms": 0.0,
            "n_threads": 0,
            "top_thread_share": 0.0,
            "over_budget_frac": 0.0,
        }

    all_durs = sorted(r["dur_ms"] for r in decode_rows)
    n_threads = len(per_thread)
    mean_ms = mean(all_durs)
    over_budget = [d for d in all_durs if frame_budget_ms and d > frame_budget_ms]
    over_budget_frac = len(over_budget) / len(all_durs)

    print("  target fps            : %.3f  (frame budget %.2f ms)" % (fps, frame_budget_ms))
    print("  decode events         : %d across %d caching thread(s)" % (len(decode_rows), n_threads))
    print(
        "  per-decode time (ms)  : mean %.2f  median %.2f  p95 %.2f  max %.2f"
        % (mean_ms, median(all_durs), percentile(all_durs, 95.0), all_durs[-1])
    )
    print("  decodes over budget   : %d / %d (%.1f%%)" % (len(over_budget), len(all_durs), 100.0 * over_budget_frac))

    top_thread_share = 0.0
    print("  per-thread share      :")
    for tid in sorted(per_thread):
        durs = sorted(per_thread[tid])
        share = len(durs) / len(decode_rows)
        top_thread_share = max(top_thread_share, share)
        print(
            "      thread %-3d : n=%-5d (%.1f%%)  mean %.2f  median %.2f  p95 %.2f  max %.2f"
            % (tid, len(durs), 100.0 * share, mean(durs), median(durs), percentile(durs, 95.0), durs[-1])
        )

    # Realized parallelism, measured from the actual busy intervals rather than
    # assuming all caching threads run in parallel (they do NOT in slow-media
    # mode, where RV serializes decode onto thread 1).
    #   busy_union  = wall time during which >=1 decode was in progress
    #   concurrency = sum(decode time) / busy_union  (avg threads working at once)
    #   active_fps  = decodes / busy_union           (real sustained decode rate)
    intervals = [((r["t_ms"] - r["dur_ms"]), r["t_ms"]) for r in decode_rows]
    busy_union_ms = merged_interval_length(intervals)
    busy_sum_ms = sum(r["dur_ms"] for r in decode_rows)
    concurrency = (busy_sum_ms / busy_union_ms) if busy_union_ms > 0 else 0.0
    active_fps = (len(decode_rows) / (busy_union_ms / 1000.0)) if busy_union_ms > 0 else 0.0
    measured_fps = len(decode_rows) / span_s if span_s > 0 else 0.0

    print(
        "  realized concurrency  : %.2f decode thread(s) working at once (of %d configured)" % (concurrency, n_threads)
    )
    print(
        "  active decode rate     : %.2f frames/s while decoding (busy %.1f s of %.1f s)"
        % (active_fps, busy_union_ms / 1000.0, span_s)
    )
    print("  overall throughput     : %.2f frames/s decoded across the whole run" % measured_fps)
    if top_thread_share >= 0.6:
        print("  NOTE: %.0f%% of decodes ran on a single thread -> decode is effectively" % (100.0 * top_thread_share))
        print("        serialized (slow-media mode). The extra threads are NOT helping.")
    if fps > 0:
        judgement = "CANNOT keep up" if active_fps < fps * 0.98 else "can keep up"
        print("  -> at %.2f fps the decode pipeline %s (based on realized parallelism)" % (fps, judgement))

    # Oversubscription test: group decode time by how many decodes were running
    # concurrently when each started. If median decode time climbs steeply with
    # concurrency, adding reader threads is counter-productive (they fight over
    # the shared OpenEXR global thread pool / CPU) and throughput will not scale.
    by_conc = defaultdict(list)
    for r in decode_rows:
        e = parse_extra(r.get("extra", ""))
        if "concurrency" in e:
            try:
                by_conc[int(e["concurrency"])].append(r["dur_ms"])
            except ValueError:
                pass
    oversubscribed = False
    if by_conc:
        print("  decode time vs concurrency (does adding threads scale?):")
        base_med = None
        for c in sorted(by_conc):
            durs = sorted(by_conc[c])
            med = median(durs)
            if c == 1:
                base_med = med
            ratio = (" (%.1fx slower than solo)" % (med / base_med)) if base_med and base_med > 0 and c > 1 else ""
            # Effective aggregate throughput at this level: c frames in flight,
            # each taking 'med' ms, so ~ c / med * 1000 frames/s of capacity.
            eff = (c / med * 1000.0) if med > 0 else 0.0
            print("      concurrency %-2d : n=%-5d median %7.2f ms  eff %5.1f fps%s" % (c, len(durs), med, eff, ratio))
        # Flag oversubscription if decodes at high concurrency are much slower
        # than solo decodes (the aggregate barely improves or regresses).
        if base_med and base_med > 0:
            hi = [c for c in by_conc if c >= 3]
            if hi:
                hi_med = median(sorted([d for c in hi for d in by_conc[c]]))
                if hi_med > base_med * 2.0:
                    oversubscribed = True
                    print(
                        "  NOTE: decodes are ~%.1fx slower when %d+ run at once than when solo."
                        % (hi_med / base_med, min(hi))
                    )
                    print("        This is EXR-pool / CPU oversubscription: the reader threads are")
                    print("        stealing cores from each other, so more threads != more throughput.")

    return {
        "count": len(decode_rows),
        "active_fps": active_fps,
        "measured_fps": measured_fps,
        "concurrency": concurrency,
        "oversubscribed": oversubscribed,
        "mean_ms": mean_ms,
        "n_threads": n_threads,
        "top_thread_share": top_thread_share,
        "over_budget_frac": over_budget_frac,
    }


def parse_extra(extra):
    """Parse a 'k=v;k=v' extra field into a dict of strings."""
    out = {}
    for tok in extra.split(";"):
        if "=" in tok:
            k, _, v = tok.partition("=")
            out[k.strip()] = v.strip()
    return out


def analyze_display(rows, fps, span_s):
    """On-screen display pacing: evaluate (cache pull) vs GPU render/upload.

    In Play-All-Frames mode the achieved fps is bounded by 1/(eval+render) per
    displayed frame. If render dominates while frames are already cached, the
    stall is on the GPU/upload path rather than on decode.
    """
    disp = [r for r in rows if r["event"] == "display"]

    print("=" * 72)
    print("DISPLAY PACING  --  on-screen frame time (evaluate vs GPU render)")
    print("=" * 72)
    if not disp:
        print("  No display events recorded.")
        print("  (Rebuild with the display diagnostic, or the run predates it.)")
        return {"count": 0, "render_bound": False, "achieved_fps": 0.0}

    frame_budget_ms = 1000.0 / fps if fps > 0 else 0.0
    evals, renders, totals = [], [], []
    advances = 0  # dframe == +/-1 : clean 1:1 advance
    repeats = 0  # dframe == 0    : same frame shown again (redraw > content)
    multi_skips = 0  # |dframe| > 1   : frames skipped
    skipped_frames = 0  # sum of realtime m_skipped reported by the player
    # (t_ms, dframe) per displayed frame, in capture order. We derive the true
    # cadence from these absolute timestamps rather than the C++ "interval"
    # field so that idle/loading stretches and pause/resume gaps (which show up
    # as a handful of large outliers) do not distort the medians.
    disp_samples = []
    # Hold-count (number of redraws/vsync intervals) for each *new* frame. At
    # 60Hz redraw / 24fps this should alternate 2,3,2,3 (avg 2.5). A bias toward
    # 3 is the play-all-frames vsync-quantization signature that drags the
    # perceived fps below target even when frames are cached.
    refresh_counts = []
    threaded_upload = None
    bit_types = set()
    for r in disp:
        e = parse_extra(r["extra"])
        try:
            ev = float(e.get("eval", "nan"))
            rd = float(e.get("render", "nan"))
        except ValueError:
            continue
        if ev == ev:  # not NaN
            evals.append(ev)
        if rd == rd:
            renders.append(rd)
        totals.append(r["dur_ms"])
        if "threadedUpload" in e:
            threaded_upload = e["threadedUpload"] == "1"
        # dframe tells us whether the displayed frame number actually advanced.
        df = None
        if "dframe" in e:
            try:
                df = int(e["dframe"])
                if df == 0:
                    repeats += 1
                elif abs(df) == 1:
                    advances += 1
                else:
                    multi_skips += 1
            except ValueError:
                df = None
        disp_samples.append((r["t_ms"], df))
        if e.get("refreshes"):
            try:
                rc = int(e["refreshes"])
                if rc > 0:
                    refresh_counts.append(rc)
            except ValueError:
                pass
        try:
            skipped_frames += abs(int(e.get("skipped", "0")))
        except ValueError:
            pass

    comps = defaultdict(int)
    comp_times = defaultdict(list)
    extra_channel_frames = 0
    decsrc_count = 0
    ch_examples = {}
    for r in rows:
        if r["event"] == "decsrc":
            e = parse_extra(r["extra"])
            t = e.get("type")
            if t:
                bit_types.add(t)
            c = e.get("comp")
            if c:
                comps[c] += 1
                comp_times[c].append(r["dur_ms"])
            if e.get("extraCh") == "1":
                extra_channel_frames += 1
            if e.get("chNames") and e.get("chDecoded") not in (None, "3", "4"):
                ch_examples.setdefault(e.get("chDecoded"), e.get("chNames"))
            decsrc_count += 1

    evals.sort()
    renders.sort()
    totals.sort()
    n = len(totals)
    over = [t for t in totals if frame_budget_ms and t > frame_budget_ms]
    achieved_fps = (1000.0 / mean(totals)) if totals and mean(totals) > 0 else 0.0

    print("  target fps            : %.3f  (frame budget %.2f ms)" % (fps, frame_budget_ms))
    print("  displayed frames      : %d" % n)
    if threaded_upload is not None:
        print("  threaded upload       : %s" % ("ON" if threaded_upload else "OFF"))
    if bit_types:
        print("  decoded pixel types   : %s" % ", ".join(sorted(bit_types)))
    if comps:
        summary = ", ".join("%s x%d" % (k, v) for k, v in sorted(comps.items(), key=lambda kv: -kv[1]))
        print("  EXR compression       : %s" % summary)
        slow_codecs = {"PIZ_COMPRESSION", "DWAA_COMPRESSION", "DWAB_COMPRESSION", "B44_COMPRESSION", "B44A_COMPRESSION"}
        seen_slow = [k for k in comps if k in slow_codecs]
        if seen_slow:
            print("    NOTE: %s decode significantly slower than ZIP/ZIPS on CPU." % ", ".join(seen_slow))

        # Per-compression decode time, sorted slowest-median first. Uses the
        # decsrc decode duration (mov->imagesAtFrame) grouped by codec so we
        # can see which compression dominates the decode cost.
        print("  decode time by codec (ms):")

        def _label(codec):
            return {"?": "(movie/non-EXR)"}.get(codec, codec)

        rows_by_slow = sorted(comp_times.items(), key=lambda kv: -median(sorted(kv[1])))
        for codec, times in rows_by_slow:
            st = sorted(times)
            print(
                "      %-22s n=%-5d mean %7.2f  median %7.2f  p95 %7.2f  max %8.2f"
                % (_label(codec), len(st), mean(st), median(st), percentile(st, 95.0), st[-1])
            )
    if decsrc_count:
        print(
            "  frames decoding extra channels (> displayed RGBA): %d / %d (%.1f%%)"
            % (extra_channel_frames, decsrc_count, 100.0 * extra_channel_frames / decsrc_count)
        )
        for chd, names in list(ch_examples.items())[:3]:
            print("      e.g. %s channels decoded: %s" % (chd, names))
    if evals:
        print(
            "  evaluate/pull (ms)    : mean %.2f  median %.2f  p95 %.2f  max %.2f"
            % (mean(evals), median(evals), percentile(evals, 95.0), evals[-1])
        )
    if renders:
        print(
            "  GPU render+upload (ms): mean %.2f  median %.2f  p95 %.2f  max %.2f"
            % (mean(renders), median(renders), percentile(renders, 95.0), renders[-1])
        )
    print(
        "  total frame time (ms) : mean %.2f  median %.2f  p95 %.2f  max %.2f"
        % (mean(totals), median(totals), percentile(totals, 95.0), totals[-1])
    )
    print("  frames over budget    : %d / %d (%.1f%%)" % (len(over), n, 100.0 * len(over) / n if n else 0.0))
    print("  implied display rate  : %.2f fps (1 / mean frame time)" % achieved_fps)

    #
    #  CADENCE: the real answer to "why not 24 fps even though decode keeps up".
    #
    #  "implied display rate" above is 1/(eval+render): the fps we *could* hit if
    #  the display loop had zero overhead. What matters to the eye is how often a
    #  *new* frame actually appears. We derive that from the absolute timestamps
    #  of the display events:
    #    * redraw gap   = time between successive on-screen redraws (any frame)
    #    * advance gap  = time between successive *new* frames (dframe advanced)
    #  Using medians of these gaps ignores idle/loading time and pause/resume
    #  outliers, so the numbers reflect steady-state playback rather than the
    #  whole-file average.
    #
    redraw_gaps = []
    for i in range(1, len(disp_samples)):
        g = disp_samples[i][0] - disp_samples[i - 1][0]
        if g > 0:
            redraw_gaps.append(g)

    advance_ts = [t for (t, df) in disp_samples if df is not None and df != 0]
    advance_gaps = []
    for i in range(1, len(advance_ts)):
        g = advance_ts[i] - advance_ts[i - 1]
        if g > 0:
            advance_gaps.append(g)

    # A "stall" is an advance gap noticeably longer than the frame budget: the
    # frame froze on screen longer than it should have (the visible stutter).
    stall_gaps = [g for g in advance_gaps if frame_budget_ms and g > frame_budget_ms * 1.5]

    redraw_hz = 0.0
    playback_fps = 0.0
    med_advance_gap = 0.0
    if redraw_gaps or advance_gaps:
        print("  --")
        print("  frame cadence (from timestamps) :")

    if redraw_gaps:
        srg = sorted(redraw_gaps)
        redraw_hz = 1000.0 / median(srg) if median(srg) > 0 else 0.0
        print(
            "    redraw gap (ms)     : median %.2f  p95 %.2f  max %.2f  -> %.1f Hz redraw"
            % (median(srg), percentile(srg, 95.0), srg[-1], redraw_hz)
        )

    print(
        "    frame advances      : %d new, %d repeats (dframe=0), %d multi-skips (>1)"
        % (advances, repeats, multi_skips)
    )

    if advance_gaps:
        sag = sorted(advance_gaps)
        med_advance_gap = median(sag)
        playback_fps = 1000.0 / med_advance_gap if med_advance_gap > 0 else 0.0
        print(
            "    new-frame gap (ms)  : median %.2f  p95 %.2f  max %.2f"
            % (med_advance_gap, percentile(sag, 95.0), sag[-1])
        )
        print("    playback rate       : %.2f fps  (1 / median new-frame gap = perceived fps)" % playback_fps)
        if stall_gaps:
            worst = sorted(stall_gaps)[-1]
            print(
                "    stalls (>1.5x budget): %d / %d new-frame gaps froze the image; worst %.1f ms (%.1f frames)"
                % (len(stall_gaps), len(advance_gaps), worst, worst / frame_budget_ms if frame_budget_ms else 0.0)
            )

    # Refresh-count distribution: the direct proof of vsync quantization. For
    # 60Hz redraw / 24fps content the ideal is a 2:3 mix averaging 2.5 redraws
    # per new frame; a distribution skewed to 3 (avg > 2.5) explains a sub-24fps
    # perceived rate that is NOT decode- or render-bound.
    if refresh_counts:
        hist = defaultdict(int)
        for rc in refresh_counts:
            hist[rc] += 1
        avg_rc = sum(refresh_counts) / len(refresh_counts)
        redraw_ms = 1000.0 / redraw_hz if redraw_hz > 0 else 0.0
        print("    refreshes/new frame : mean %.2f  (redraws each new frame is held)" % avg_rc)
        for k in sorted(hist):
            share = 100.0 * hist[k] / len(refresh_counts)
            print("        held %d redraw(s) : %5d  (%4.1f%%)" % (k, hist[k], share))
        if redraw_ms > 0 and fps > 0:
            ideal_rc = (1000.0 / fps) / redraw_ms
            print("        ideal hold        : %.2f redraws/frame -> vsync bias %+.2f" % (ideal_rc, avg_rc - ideal_rc))
            if avg_rc > ideal_rc + 0.1:
                print("        NOTE: cadence is rounding UP to an extra vsync interval (play-all-frames")
                print("              m_shift re-anchor): perceived fps drops below target even though")
                print("              frames are cached and display work is cheap.")

    if repeats and (advances + repeats + multi_skips):
        total_dr = advances + repeats + multi_skips
        print(
            "    NOTE: %.0f%% of redraws re-show the same frame. If the playback rate above is"
            % (100.0 * repeats / total_dr)
        )
        print("          ~= target fps that is HEALTHY (redraw loop simply runs faster than the")
        print("          content); if it is well below target, new frames are arriving late.")
    if skipped_frames:
        print("    realtime m_skipped  : %d frame(s) reported skipped by the player" % skipped_frames)

    mean_render = mean(renders) if renders else 0.0
    mean_eval = mean(evals) if evals else 0.0
    render_bound = fps > 0 and achieved_fps < fps * 0.98 and mean_render >= mean_eval
    if render_bound:
        print("  -> render/upload dominates the frame budget: the display loop, not")
        print("     decode, is capping the fps (frames are cached but slow to show).")

    # Cadence-based classification (robust to idle time):
    #   healthy    : new frames arrive at ~target and redraw work is cheap
    #   stutter    : new-frame gaps are erratic / well below target while the
    #                measured display work per frame is small -> frames arriving
    #                late (decode/scheduling), not render-bound.
    cadence_ok = fps > 0 and playback_fps >= fps * 0.95
    cadence_stutter = fps > 0 and playback_fps > 0 and playback_fps < fps * 0.9 and mean(totals) < frame_budget_ms * 0.5
    if cadence_stutter:
        print("  -> playback rate %.1f fps < target %.1f fps although per-frame display work is" % (playback_fps, fps))
        print("     only %.2f ms: new frames are arriving late (not display/render bound)." % mean(totals))
        if stall_gaps:
            print("     %d visible stall(s) where the on-screen frame froze > 1.5x the budget." % len(stall_gaps))

    return {
        "count": n,
        "render_bound": render_bound,
        "cadence_ok": cadence_ok,
        "cadence_stutter": cadence_stutter,
        "achieved_fps": achieved_fps,
        "redraw_hz": redraw_hz,
        "playback_fps": playback_fps,
        "med_advance_gap_ms": med_advance_gap,
        "stalls": len(stall_gaps),
        "repeats": repeats,
        "advances": advances,
        "mean_render_ms": mean_render,
        "mean_eval_ms": mean_eval,
        "bit_types": sorted(bit_types),
    }


def analyze_audio(rows, span_s):
    """Audio starvation statistics."""
    misses = [r for r in rows if r["event"] == "cachemiss"]
    locked = [r for r in rows if r["event"] == "audiolocked"]
    underruns = [r for r in rows if r["event"] == "underrun"]

    print("=" * 72)
    print("AUDIO  --  cache misses, lock-outs and hardware underruns")
    print("=" * 72)
    total = len(misses) + len(locked) + len(underruns)
    print("  audio cache misses    : %d" % len(misses))
    print("  audio lock-outs       : %d" % len(locked))
    print("  ALSA underruns/xruns  : %d" % len(underruns))
    if span_s > 0:
        print("  starvation rate       : %.3f events/s over %.1f s" % (total / span_s, span_s))

    # First event is often a harmless startup miss; count steady-state ones.
    steady = [r for r in (misses + locked + underruns) if r["t_ms"] > 2000.0]
    print("  steady-state events   : %d (excluding first 2 s of playback)" % len(steady))
    return {
        "misses": len(misses),
        "locked": len(locked),
        "underruns": len(underruns),
        "total": total,
        "steady": len(steady),
    }


def analyze_buffering(rows, span_s):
    """Buffering pauses (frame cache under-runs) and realtime skips."""
    events = [r for r in rows if r["event"] in ("buffering", "resume")]
    buffering = [r for r in rows if r["event"] == "buffering"]
    skips = [r for r in rows if r["event"] == "skip"]

    print("=" * 72)
    print("PLAYBACK CONTROL  --  buffering pauses and realtime frame skips")
    print("=" * 72)
    print("  buffering pauses      : %d" % len(buffering))
    print(
        "  realtime frame skips  : %d event(s), %d frame(s) dropped"
        % (len(skips), sum(abs(int(r["dur_ms"])) for r in skips))
    )

    # Pair buffering -> resume in time order to estimate pause durations.
    pause_durations = []
    open_pause_t = None
    for r in sorted(events, key=lambda x: x["t_ms"]):
        if r["event"] == "buffering" and open_pause_t is None:
            open_pause_t = r["t_ms"]
        elif r["event"] == "resume" and open_pause_t is not None:
            pause_durations.append(r["t_ms"] - open_pause_t)
            open_pause_t = None
    if pause_durations:
        sp = sorted(pause_durations)
        print(
            "  pause duration (ms)   : n=%d  mean %.1f  median %.1f  max %.1f  total %.1f"
            % (len(sp), mean(sp), median(sp), sp[-1], sum(sp))
        )
        if span_s > 0:
            print("  time spent buffering  : %.1f%% of the run" % (100.0 * sum(sp) / (span_s * 1000.0)))
    return {
        "buffering": len(buffering),
        "skips": len(skips),
        "skipped_frames": sum(abs(int(r["dur_ms"])) for r in skips),
        "pause_total_ms": sum(pause_durations),
    }


def analyze_slow_media(rows, span_s):
    """Report per-source slowRandomAccess flags and slow-media mode transitions."""
    sources = [r for r in rows if r["event"] == "sourceinfo"]
    transitions = sorted((r for r in rows if r["event"] == "slowmedia"), key=lambda x: x["t_ms"])

    if not sources and not transitions:
        return {"slow_sources": 0, "slow_fraction": 0.0, "any": False}

    print("=" * 72)
    print("SLOW-MEDIA MODE  --  why decode may be restricted to one thread")
    print("=" * 72)

    slow_sources = 0
    if sources:
        print("  sources opened:")
        for r in sources:
            flag = "SLOW random access" if r["dur_ms"] >= 0.5 else "fast"
            if r["dur_ms"] >= 0.5:
                slow_sources += 1
            print("      [%s] %s" % (flag, r["extra"]))
    else:
        print("  (no per-source info recorded)")

    # Estimate the fraction of the run spent in slow-media mode by integrating
    # the on/off transitions over time. Assume mode starts "off".
    slow_ms = 0.0
    state_on = False
    last_t = transitions[0]["t_ms"] if transitions else 0.0
    end_t = max((r["t_ms"] for r in rows), default=0.0)
    for tr in transitions:
        if state_on:
            slow_ms += tr["t_ms"] - last_t
        state_on = tr["dur_ms"] >= 0.5  # 1.0=on, 0.0=off
        last_t = tr["t_ms"]
    if state_on:
        slow_ms += end_t - last_t
    slow_fraction = (slow_ms / (span_s * 1000.0)) if span_s > 0 else 0.0

    print("  slow-media transitions: %d" % len(transitions))
    if transitions:
        onc = sum(1 for t in transitions if t["dur_ms"] >= 0.5)
        print("      turned ON %d time(s), OFF %d time(s)" % (onc, len(transitions) - onc))
        print(
            "      first ON at frame %d (t=%.1f s)"
            % (
                next((t["frame"] for t in transitions if t["dur_ms"] >= 0.5), -1),
                next((t["t_ms"] / 1000.0 for t in transitions if t["dur_ms"] >= 0.5), 0.0),
            )
        )
    print("  time in slow-media    : ~%.0f%% of the run" % (100.0 * slow_fraction))

    return {"slow_sources": slow_sources, "slow_fraction": slow_fraction, "any": True}


def parse_rvprof(path):
    """Light parse of an .rvprof file; report display-thread frame budget overruns."""
    records = []
    try:
        with open(path) as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                fields = {}
                for tok in line.split(","):
                    if "=" in tok:
                        k, _, v = tok.partition("=")
                        try:
                            fields[k] = float(v)
                        except ValueError:
                            pass
                if "R0" in fields and "R1" in fields:
                    records.append(fields)
    except OSError as exc:
        print("  could not read rvprof: %s" % exc)
        return

    print("=" * 72)
    print("RVPROF (display thread)  --  %s" % os.path.basename(path))
    print("=" * 72)
    if not records:
        print("  No usable per-frame records found.")
        return

    render_ms = sorted(1000.0 * (r["R1"] - r["R0"]) for r in records if r["R1"] >= r["R0"])
    io_ms = sorted(
        1000.0 * (r["IO1"] - r["IO0"]) for r in records if "IO1" in r and "IO0" in r and r["IO1"] >= r["IO0"]
    )
    print("  frames profiled       : %d" % len(records))
    if render_ms:
        print(
            "  display render (ms)   : mean %.2f  median %.2f  p95 %.2f  max %.2f"
            % (mean(render_ms), median(render_ms), percentile(render_ms, 95.0), render_ms[-1])
        )
    if io_ms:
        nonzero = [v for v in io_ms if v > 0.01]
        print(
            "  display-thread IO (ms): %d/%d frames did synchronous IO; mean(nonzero) %.2f  max %.2f"
            % (len(nonzero), len(io_ms), mean(nonzero) if nonzero else 0.0, io_ms[-1])
        )
        print("  (display-thread IO > 0 means a cache miss forced a decode on the display")
        print("   thread -- a direct on-screen stutter caused by slow image decode.)")


def analyze_cache(rows, fps, span_s):
    """Cache-content verification: was the frame the display asked for actually
    resident in the look-ahead cache, and if not, how full was the cache?

    Emitted by IPGraph::evaluateAtFrame (displaycache) and
    FBCache::initiateCachingOfBestFrameGroup (cachestall).
    """
    dc = [r for r in rows if r["event"] == "displaycache"]
    stalls = [r for r in rows if r["event"] == "cachestall"]

    print("=" * 72)
    print("CACHE CONTENT  --  was the needed frame actually in cache?")
    print("=" * 72)
    if not dc:
        print("  No displaycache events recorded.")
        print("  (Rebuild with the cache-content diagnostic, or the run predates it.)")
        return {"count": 0, "miss_rate": 0.0, "misses": 0, "cachestalls": len(stalls)}

    n = len(dc)
    hits, misses = 0, 0
    miss_full = []  # cache percent-full at each miss
    miss_runway = []  # contiguous cached frames ahead at each miss
    miss_overflow = 0  # misses where the cache was overflowing (full)
    otf_ms = []  # on-the-fly decode time (display thread) at misses
    runway_all = []  # runway on every presented frame (hit or miss)
    zero_runway = 0  # presented frames whose NEXT frame was not cached
    full_all = []
    for r in dc:
        e = parse_extra(r["extra"])
        is_hit = e.get("hit") == "1"
        try:
            rw = int(e.get("runway", "0"))
            runway_all.append(rw)
            if rw == 0:
                zero_runway += 1
        except ValueError:
            rw = None
        try:
            full_all.append(float(e.get("full", "0")))
        except ValueError:
            pass
        if is_hit:
            hits += 1
        else:
            misses += 1
            try:
                miss_full.append(float(e.get("full", "0")))
            except ValueError:
                pass
            if rw is not None:
                miss_runway.append(rw)
            if e.get("overflow") == "1":
                miss_overflow += 1
            try:
                otf = float(e.get("otfDecodeMs", "0"))
                if otf > 0:
                    otf_ms.append(otf)
            except ValueError:
                pass

    miss_rate = misses / n if n else 0.0
    print("  presented frames      : %d" % n)
    print("  cache HITS / MISSES   : %d / %d  (%.1f%% miss)" % (hits, misses, 100.0 * miss_rate))
    if full_all:
        sfa = sorted(full_all)
        print("  cache fullness (all)  : median %.1f%%  min %.1f%%  max %.1f%%" % (median(sfa), sfa[0], sfa[-1]))
    if runway_all:
        sra = sorted(runway_all)
        print(
            "  ahead runway (all)    : median %d  p05 %d frame(s) cached ahead of playhead"
            % (median(sra), percentile(sra, 5.0))
        )
        print("  next frame NOT cached : %d / %d presented frames (%.1f%%)" % (zero_runway, n, 100.0 * zero_runway / n))

    med_full_miss = 0.0
    med_runway_miss = 0.0
    if misses:
        if miss_full:
            smf = sorted(miss_full)
            med_full_miss = median(smf)
            print("  -- at cache MISSES --")
            print(
                "    cache was full      : median %.1f%%  (%d/%d misses while overflowing)"
                % (med_full_miss, miss_overflow, misses)
            )
        if miss_runway:
            smr = sorted(miss_runway)
            med_runway_miss = median(smr)
            print("    ahead runway        : median %d  max %d frame(s) cached ahead" % (med_runway_miss, smr[-1]))
        if otf_ms:
            som = sorted(otf_ms)
            print(
                "    on-the-fly decode   : n=%d  median %.1f ms  p95 %.1f ms  max %.1f ms (blocks display)"
                % (len(som), median(som), percentile(som, 95.0), som[-1])
            )

        # The signature the user suspects: misses happening while the cache is
        # essentially full and there is little/no runway ahead of the playhead.
        if med_full_miss >= 90.0 and med_runway_miss <= 2:
            print("  => CONFIRMED pattern: frames are MISSING from a nearly-full cache with an")
            print("     empty ahead-runway -> the cache is full of frames that are not the ones")
            print("     the player needs next. See cachestall below for why.")

    print("  cache stalls (caching gave up while overflowing): %d" % len(stalls))
    if stalls:
        # Show how far the refused cache target sat from the protected frame.
        sample = stalls[: min(3, len(stalls))]
        for r in sample:
            e = parse_extra(r["extra"])
            print(
                "      want frame %s (util %s) but protected cached frame %s (util %s); displayFrame %s"
                % (
                    e.get("cacheFrame", "?"),
                    e.get("cacheUtil", "?"),
                    e.get("freeFrame", "?"),
                    e.get("freeUtil", "?"),
                    e.get("displayFrame", "?"),
                )
            )

    return {
        "count": n,
        "misses": misses,
        "miss_rate": miss_rate,
        "miss_overflow": miss_overflow,
        "med_full_at_miss": med_full_miss,
        "med_runway_at_miss": med_runway_miss,
        "zero_runway_frac": (zero_runway / n) if n else 0.0,
        "cachestalls": len(stalls),
        "otf_count": len(otf_ms),
    }


def analyze_stall_causes(rows, fps):
    """Decompose slow redraws into eval / GPU-render / 'outside' time and test
    whether they coincide with concurrent EXR decode activity.

    The refresh-count histogram tells us WHETHER new frames arrive late; this
    tells us WHERE the time goes on a slow redraw. For each display event we
    have interval (wall time since the previous redraw), eval and render. The
    residual (interval - eval - render) is 'outside' time: heartbeat/event-loop
    scheduling, vsync swap wait, or the main thread being blocked/starved. If
    the slow redraws overlap windows where >=3 decodes were running, the main
    thread is losing the CPU (or a lock) to the reader pool even though the
    frame is already cached -- i.e. oversubscription is stalling *display*, not
    just decode.
    """
    budget = 1000.0 / fps if fps > 0 else 0.0

    disp = []
    for r in rows:
        if r["event"] != "display":
            continue
        e = parse_extra(r["extra"])
        try:
            interval = float(e.get("interval", "0"))
            ev = float(e.get("eval", "0"))
            rd = float(e.get("render", "0"))
        except ValueError:
            continue
        try:
            reqs = int(e.get("reqs", "-1"))
        except ValueError:
            reqs = -1
        try:
            outside_gap = float(e.get("outsideGap", "-1"))
        except ValueError:
            outside_gap = -1.0
        try:
            df = int(e.get("dframe", "0"))
        except ValueError:
            df = 0
        disp.append((r["t_ms"], interval, ev, rd, reqs, outside_gap, df))

    print("=" * 72)
    print("STALL ANATOMY  --  where does a slow redraw spend its time?")
    print("=" * 72)
    if not disp or budget <= 0:
        print("  No display events with interval timing recorded.")
        return {"stalls": 0, "decode_overlap_frac": 0.0}

    # Decode windows [start,end] with the concurrency reported at decode start.
    decodes = []
    for r in rows:
        if r["event"] != "decode":
            continue
        e = parse_extra(r.get("extra", ""))
        try:
            conc = int(e.get("concurrency", "1"))
        except ValueError:
            conc = 1
        decodes.append((r["t_ms"] - r["dur_ms"], r["t_ms"], conc))
    decodes.sort()
    dec_starts = [d[0] for d in decodes]

    # Per-render event-processing handler samples (GUI-thread Mu/Python work
    # that runs between paints). Used to split the "outside render_v2" bucket
    # into event-loop-handler time vs composite/swapBuffers present time.
    perrender = sorted((r["t_ms"], r["dur_ms"]) for r in rows if r["event"] == "perrender")
    pr_ts = [p[0] for p in perrender]

    # Optional GPU-completion probe (RV_DIAG_GLFINISH): paint events carry
    # gpuFinish = glFinish() time right after render(), i.e. how long the GPU
    # took to actually execute the frame's upload+shaders. If this is large on
    # slow paints, the stall is GPU upload/render bound; if it is small while the
    # present (gap) still stalls, the block is the compositor/present path.
    gpu_all = []
    gpu_gap = []  # (gap, gpuFinish) pairs for paints where glFinish was active
    for r in rows:
        if r["event"] != "paint":
            continue
        e = parse_extra(r["extra"])
        try:
            g = float(e.get("gpuFinish", "-1"))
            gp = float(e.get("gap", "-1"))
        except ValueError:
            continue
        if g >= 0:
            gpu_all.append(g)
            if gp >= 0:
                gpu_gap.append((gp, g))

    def perrender_in(a, b):
        """Sum of per-render handler time whose events fall within [a,b]."""
        import bisect

        lo = bisect.bisect_left(pr_ts, a)
        hi = bisect.bisect_right(pr_ts, b)
        return sum(perrender[i][1] for i in range(lo, hi))

    def max_conc_in(a, b):
        """Max decode concurrency for any decode window overlapping [a,b]."""
        import bisect

        # any decode with start <= b and end >= a
        hi = bisect.bisect_right(dec_starts, b)
        best = 0
        # scan backwards from hi while decode start could still overlap; decode
        # durations are bounded (<~0.3s) but we simply scan the candidates whose
        # start < b and check end >= a.
        for i in range(hi - 1, -1, -1):
            s, e2, c = decodes[i]
            if e2 >= a:
                if c > best:
                    best = c
            # stop once starts are far enough before 'a' that no window reaches
            if s < a - 400.0:
                break
        return best

    slow = [d for d in disp if d[1] > budget * 1.5]
    outside_all = [max(0.0, d[1] - d[2] - d[3]) for d in disp if d[1] > 0]

    # New-frame vs repeat present time. In Play-All-Frames a repeat re-shows the
    # already-resident GPU texture (cheap present), while a NEW frame must upload
    # a fresh texture before the swap can present it. If the present cost is paid
    # almost entirely on NEW frames, the stall is the synchronous GPU texture
    # upload (threaded upload OFF), not a uniform compositor/vsync problem --
    # which points the fix at async/threaded upload or prefetch.
    new_gap = sorted(d[5] for d in disp if d[6] != 0 and d[5] >= 0)
    rep_gap = sorted(d[5] for d in disp if d[6] == 0 and d[5] >= 0)
    if new_gap and rep_gap:
        stall_line = budget * 1.5
        new_stalls = sum(1 for g in new_gap if g > stall_line)
        rep_stalls = sum(1 for g in rep_gap if g > stall_line)
        print("  present (outsideGap) by frame kind:")
        print(
            "      NEW frames (dframe!=0) : median %.2f ms  p95 %.2f  (n=%d)  stalls>%.0fms: %d (%.0f%%)"
            % (
                median(new_gap),
                percentile(new_gap, 95.0),
                len(new_gap),
                stall_line,
                new_stalls,
                100.0 * new_stalls / len(new_gap),
            )
        )
        print(
            "      repeats  (dframe==0)   : median %.2f ms  p95 %.2f  (n=%d)  stalls>%.0fms: %d (%.0f%%)"
            % (
                median(rep_gap),
                percentile(rep_gap, 95.0),
                len(rep_gap),
                stall_line,
                rep_stalls,
                100.0 * rep_stalls / len(rep_gap),
            )
        )
        total_stalls = new_stalls + rep_stalls
        # The stalls live in the tail, not the median: compare where the >1.5x
        # budget presents actually occur. If they are overwhelmingly NEW frames,
        # the cost is specific to presenting new content (GPU texture upload of
        # the new frame and/or the compositor presenting changed content), not a
        # uniform per-present vsync cost (which would hit repeats too).
        if total_stalls > 0 and new_stalls >= total_stalls * 0.8:
            print("      -> stalls occur almost only on NEW frames (repeats present fine).")
            print("         The cost is specific to presenting a NEW frame: either the")
            print("         synchronous GPU texture upload (threaded upload OFF) or the")
            print("         compositor presenting changed content. Test both cheaply:")
            print("           * TWK_ALLOW_THREADED_UPLOAD=1  (overlaps upload with present)")
            print("           * disable desktop compositor / fullscreen-unredirect")
        else:
            print("      -> stalls hit repeats too: uniform compositor/vsync present cost,")
            print("         not new-frame texture upload.")

    if outside_all:
        so = sorted(outside_all)
        print("  'outside' time per redraw (interval - eval - render):")
        print("      median %.2f ms  p95 %.2f ms  max %.2f ms" % (median(so), percentile(so, 95.0), so[-1]))
        print("      (at 60Hz vsync ~16 ms of this is the normal swap wait)")

    if not slow:
        print("  No slow redraws (> 1.5x budget) -- pacing is clean.")
        return {"stalls": 0, "decode_overlap_frac": 0.0}

    # Heartbeat = 120Hz (RvApplication timer), so an on-time redraw loop posts
    # ~ interval/8.33ms redraw requests per actual paint. If during a stall the
    # number of coalesced requests (reqs) tracks the gap, RV DID ask for the
    # paint and Qt deferred it (compositor present). If reqs stays ~1, the
    # heartbeat/event loop itself was starved (RV never asked).
    HEARTBEAT_MS = 1000.0 / 120.0
    slow_reqs = [d[4] for d in slow if d[4] >= 0]
    if slow_reqs:
        exp = [max(1.0, d[1] / HEARTBEAT_MS) for d in slow if d[4] >= 0]
        got = sorted(slow_reqs)
        print("  heartbeat requests coalesced per stalled paint:")
        print("      observed reqs/paint : median %.1f  p95 %.1f" % (median(got), percentile(got, 95.0)))
        print("      expected (if 120Hz) : median %.1f  (gap / 8.33 ms)" % median(sorted(exp)))
        if median(got) >= median(sorted(exp)) * 0.6:
            print("      -> RV posted redraws on time but Qt did NOT paint them: the")
            print("         stall is in the Qt6 compositor/present path (async QWidget::")
            print("         update decoupled from vsync), NOT RV's heartbeat or decode.")
        else:
            print("      -> few requests during stalls: the 120Hz heartbeat/event loop")
            print("         itself is being starved (main thread blocked) -- investigate")
            print("         what blocks Session::update between paints.")

    # Three-way split of a stalled interval (needs the outsideGap field):
    #   eval+render        : the measured display work (tiny)
    #   rest of render_v2  : interval - outsideGap - eval - render
    #                        (frameChangeEvent + graph work + locks)
    #   outside render_v2  : outsideGap
    #                        (paintGL rest + Qt composite + swapBuffers/vsync
    #                         + event loop)
    slow_gap = [d for d in slow if d[5] >= 0]
    if slow_gap:
        og = sorted(d[5] for d in slow_gap)
        inside = sorted(max(0.0, d[1] - d[5]) for d in slow_gap)
        rest_rv2 = sorted(max(0.0, d[1] - d[5] - d[2] - d[3]) for d in slow_gap)
        print("  stalled interval split (where the ~%.0f ms goes):" % median(sorted(d[1] for d in slow_gap)))
        print("      outside render_v2  : median %.2f ms  (present/composite/swap + event loop)" % median(og))
        print("      inside render_v2   : median %.2f ms  (eval + render + frameChangeEvent)" % median(inside))
        print(
            "      rest of render_v2  : median %.2f ms  (frameChangeEvent/graph, excl eval+render)" % median(rest_rv2)
        )

        # Split the "outside render_v2" bucket into the per-render event-loop
        # handler vs the composite/swapBuffers present path, using perrender
        # events that land within each stalled between-paint window.
        if perrender:
            handler = []
            swap = []
            for d in slow_gap:
                t, gap = d[0], d[5]
                h = perrender_in(t - gap, t)
                handler.append(h)
                swap.append(max(0.0, gap - h))
            mh = median(sorted(handler))
            ms = median(sorted(swap))
            print("      -- outside split (via perrender events) --")
            print("      event-loop handler : median %.2f ms  (per-render Mu/Python on GUI thread)" % mh)
            print("      composite + swap   : median %.2f ms  (Qt present / swapBuffers / vsync)" % ms)
            if mh >= ms:
                print("      -> the stall is the PER-RENDER EVENT HANDLER blocking the GUI thread")
                print("         (userGenericEvent 'per-render-event-processing' -> Mu/Python).")
                print("         Fix target: make that handler cheap/async, not the present path.")
            else:
                print("      -> the stall is the Qt COMPOSITE/SWAPBUFFERS present path blocking the")
                print("         GUI thread (QOpenGLWidget composite + vsync swap). Fix target:")
                print("         restore a direct synchronous present for the playback view.")

        # GPU-completion probe (only present when run with RV_DIAG_GLFINISH=1).
        if gpu_all:
            ga = sorted(gpu_all)
            print("      -- GPU probe (RV_DIAG_GLFINISH: glFinish after render) --")
            print(
                "      gpuFinish (GPU exec) : median %.2f ms  p95 %.2f  max %.2f  (n=%d)"
                % (median(ga), percentile(ga, 95.0), ga[-1], len(ga))
            )
            slow_gpu = sorted(g for (gp, g) in gpu_gap if (gp + g) > budget * 1.5)
            if slow_gpu:
                print(
                    "      on stalled paints    : gpuFinish median %.2f ms  p95 %.2f"
                    % (median(slow_gpu), percentile(slow_gpu, 95.0))
                )
            # The decision must use the GPU time ON the stalled paints, not the
            # overall distribution (most paints are fast, which hides the tail).
            stall_gpu_med = median(slow_gpu) if slow_gpu else 0.0
            if stall_gpu_med > budget * 1.5:
                print("      -> GPU execution ITSELF stalls (median %.1f ms on stalled paints):" % stall_gpu_med)
                print("         the synchronous texture upload/render of new frames is the")
                print("         bottleneck. Fix: async/threaded upload, PBO streaming, prefetch")
                print("         the next frame, or reduce upload cost -- NOT the compositor.")
            elif percentile(ga, 95.0) > budget * 1.5:
                print("      -> GPU execution stalls on the tail: upload/render bound.")
            else:
                print("      -> GPU finishes fast even on stalls: the block is AFTER the GPU is")
                print("         done -> the compositor/swapBuffers present path, not upload.")
        elif median(og) >= median(inside):
            print("      -> the stall is OUTSIDE render_v2 (present + event loop). Rebuild with the")
            print("         paint/perrender instrumentation to split swap vs event-handler.")

    s_iv = sorted(d[1] for d in slow)
    s_ev = sorted(d[2] for d in slow)
    s_rd = sorted(d[3] for d in slow)
    s_out = sorted(max(0.0, d[1] - d[2] - d[3]) for d in slow)
    print("  slow redraws (> %.1f ms) : %d" % (budget * 1.5, len(slow)))
    print("      interval  : median %.2f  p95 %.2f  max %.2f ms" % (median(s_iv), percentile(s_iv, 95.0), s_iv[-1]))
    print("      eval      : median %.2f  p95 %.2f ms" % (median(s_ev), percentile(s_ev, 95.0)))
    print("      render    : median %.2f  p95 %.2f ms" % (median(s_rd), percentile(s_rd, 95.0)))
    print(
        "      outside   : median %.2f  p95 %.2f ms  <- unexplained by display work"
        % (median(s_out), percentile(s_out, 95.0))
    )

    overlap_any = 0
    overlap_hi = 0
    iv_overlap_hi = []
    iv_no_decode = []
    for d in slow:
        t, iv = d[0], d[1]
        mc = max_conc_in(t - iv, t)
        if mc >= 1:
            overlap_any += 1
        if mc >= 3:
            overlap_hi += 1
            iv_overlap_hi.append(iv)
        else:
            iv_no_decode.append(iv)
    n = len(slow)
    frac_hi = overlap_hi / n if n else 0.0
    print("  decode overlap of slow redraws:")
    print("      overlapped ANY decode      : %d / %d (%.0f%%)" % (overlap_any, n, 100.0 * overlap_any / n))
    print("      overlapped 3+ concurrent   : %d / %d (%.0f%%)" % (overlap_hi, n, 100.0 * frac_hi))
    if iv_overlap_hi and iv_no_decode:
        print("      median slow-redraw interval when 3+ decodes active : %.2f ms" % median(sorted(iv_overlap_hi)))
        print("      median slow-redraw interval otherwise              : %.2f ms" % median(sorted(iv_no_decode)))
    if frac_hi >= 0.4:
        print("  -> most stalls coincide with 3+ concurrent decodes: the main display")
        print("     thread is losing CPU/locks to the reader pool even though the frame")
        print("     is cached. This is decode oversubscription stalling DISPLAY, not a")
        print("     pacing-math (m_shift) problem.")
    elif median(s_out) > budget:
        print("  -> stalls are 'outside' time not tied to decode: investigate the")
        print("     heartbeat/update timer and vsync swap on the main thread.")

    return {
        "stalls": n,
        "decode_overlap_frac": frac_hi,
        "median_outside_ms": median(s_out),
    }


def analyze_upload(rows, fps):
    """Per-plane GPU texture-upload analysis (Option A).

    Each 'upload' row is one ImageRenderer::uploadPlane() call. Fields:
      w,h,ch,type,pxsz,mb,pbo(0/1),update(0/1),cpuGBps
    dur_ms is the CPU-side submission time (map+memcpy for PBO, or the
    glTexImage2D client copy for the non-PBO fallback). The per-paint GPU
    total is measured separately by RV_DIAG_GLFINISH in the 'paint' event.

    This isolates the cause of slow new-frame uploads:
      * pbo usage low            -> we are on the slow fallback path (a bug/gate)
      * pbo=1 but cpuGBps low     -> map+memcpy (CPU/mem bandwidth) is the cost
      * pbo=1 and cpuGBps high    -> CPU submit is fine; the cost is GPU-side DMA
                                     (see gpuFinish); suspect format conversion
    """
    ups = [r for r in rows if r["event"] == "upload"]
    print("=" * 72)
    print("GPU TEXTURE UPLOAD  --  ImageRenderer::uploadPlane() (Option A)")
    print("=" * 72)
    if not ups:
        print("  No upload events recorded.")
        print("  (Run with RV_PLAYBACK_DIAG=1 on a build that has the uploadPlane probe.)")
        return {"count": 0}

    cpu_ms = []
    gbps = []
    pbo_yes = 0
    update_yes = 0
    by_res = defaultdict(lambda: {"n": 0, "cpu": [], "gbps": [], "pbo": 0, "pbo_gbps": [], "nopbo_gbps": []})
    for r in ups:
        e = parse_extra(r["extra"])
        cpu_ms.append(r["dur_ms"])
        try:
            g = float(e.get("cpuGBps", "0"))
        except ValueError:
            g = 0.0
        gbps.append(g)
        is_pbo = e.get("pbo", "0") == "1"
        if is_pbo:
            pbo_yes += 1
        if e.get("update", "0") == "1":
            update_yes += 1
        key = "%sx%s ch%s ty%s" % (e.get("w", "?"), e.get("h", "?"), e.get("ch", "?"), e.get("type", "?"))
        b = by_res[key]
        b["n"] += 1
        b["cpu"].append(r["dur_ms"])
        b["gbps"].append(g)
        if is_pbo:
            b["pbo"] += 1
            b["pbo_gbps"].append(g)
        else:
            b["nopbo_gbps"].append(g)

    n = len(ups)
    cpu_s = sorted(cpu_ms)
    gbps_s = sorted(gbps)
    pbo_frac = pbo_yes / n

    print("  upload events         : %d plane upload(s)" % n)
    print(
        "  PBO fast path used    : %d / %d (%.1f%%)   sub-image updates: %d"
        % (pbo_yes, n, 100.0 * pbo_frac, update_yes)
    )
    print(
        "  CPU submit time (ms)  : mean %.2f  median %.2f  p95 %.2f  max %.2f"
        % (mean(cpu_ms), median(cpu_s), percentile(cpu_s, 95.0), cpu_s[-1])
    )
    print("  CPU submit throughput : median %.2f GB/s  min %.2f GB/s" % (median(gbps_s), gbps_s[0]))
    print("")
    print("  by resolution/format  (frame size drives upload cost):")
    for key, b in sorted(by_res.items(), key=lambda kv: -kv[1]["n"]):
        cs = sorted(b["cpu"])
        gs = sorted(b["gbps"])
        print(
            "    %-24s n=%-5d cpu med %.2f ms / p95 %.2f ms  cpuGBps med %.2f  pbo %d%%"
            % (key, b["n"], median(cs), percentile(cs, 95.0), median(gs), int(round(100.0 * b["pbo"] / b["n"])))
        )
        #  Split throughput PBO vs non-PBO: a plain memcpy into the PBO does
        #  not depend on RGB-vs-RGBA, so if PBO throughput is uniform across
        #  formats the CPU-submit cost is purely the non-PBO fallback; the
        #  RGB16F penalty then lives on the GPU side (gpuFinish).
        pg = sorted(b["pbo_gbps"])
        ng = sorted(b["nopbo_gbps"])
        if pg and ng:
            print(
                "        PBO med %.2f GB/s (n=%d)   non-PBO med %.2f GB/s (n=%d)"
                % (median(pg), len(pg), median(ng), len(ng))
            )

    print("")
    if pbo_frac < 0.9:
        print("  -> VERDICT: %.0f%% of uploads fall back to the NON-PBO path" % (100.0 * (1.0 - pbo_frac)))
        print("     (glTexImage2D from client memory). This is the slow, synchronous")
        print("     upload path. Check the usePBO gate in uploadPlane():")
        print("       - fb->scanlinePixelPadding()==0 and contiguous scanlines")
        print(
            "       - d->pPBOToGPU created (m_uploadedTextures.size() < RV_RENDERING_MAX_CONCURRENT_PBOS, default 10)"
        )
        print("       - PBO size >= totalBytes")
        print("     Fixing the gate should restore DMA-speed uploads (Option B).")
    elif median(gbps_s) < 3.0:
        print("  -> VERDICT: PBO path IS used but CPU submit throughput is low")
        print("     (%.2f GB/s median). The map+FastMemcpy_MP into the PBO is the" % median(gbps_s))
        print("     cost -> CPU/memory-bandwidth bound, not GPU. Look at memcpy")
        print("     thread count / NUMA, or upload directly from the decoded FB.")
    else:
        print("  -> VERDICT: CPU submit is fast (%.2f GB/s median) and PBO is used." % median(gbps_s))
        print("     The remaining cost is GPU-side (see gpuFinish in the 'paint'")
        print("     stall anatomy). Suspect a driver pixel-format conversion on")
        print("     glTexSubImage2D: verify internalFormat vs (format,type) is a")
        print("     natively supported combo for GL_TEXTURE_RECTANGLE half-float.")

    return {"count": n, "pbo_frac": pbo_frac, "cpu_gbps_med": median(gbps_s)}


def verdict(dec, aud, buf, slow, disp, cache, fps):
    print("=" * 72)
    print("VERDICT")
    print("=" * 72)

    frame_reasons = []
    if buf["buffering"] > 0:
        frame_reasons.append("%d buffering pause(s) (%.0f ms total)" % (buf["buffering"], buf["pause_total_ms"]))
    if buf["skips"] > 0:
        frame_reasons.append("%d realtime skip event(s), %d frame(s) dropped" % (buf["skips"], buf["skipped_frames"]))
    if fps > 0 and dec["count"] > 0 and dec["active_fps"] < fps * 0.98:
        frame_reasons.append(
            "realized decode rate %.1f fps < target %.1f fps (%.1f thread(s) working at once)"
            % (dec["active_fps"], fps, dec["concurrency"])
        )
    if dec["count"] > 0 and dec["over_budget_frac"] > 0.5:
        frame_reasons.append("%.0f%% of decodes exceed the frame budget" % (100.0 * dec["over_budget_frac"]))
    if dec["count"] > 0 and dec["top_thread_share"] >= 0.6:
        frame_reasons.append(
            "decode serialized onto one thread (%.0f%% of decodes) -- slow-media mode"
            % (100.0 * dec["top_thread_share"])
        )
    if slow["any"] and slow["slow_fraction"] >= 0.5:
        frame_reasons.append(
            "session in slow-media mode ~%.0f%% of the run (%d slow source(s))"
            % (100.0 * slow["slow_fraction"], slow["slow_sources"])
        )

    display_reasons = []
    if disp["count"] > 0 and disp["render_bound"]:
        display_reasons.append(
            "display loop capped at %.1f fps < target %.1f fps; GPU render+upload "
            "mean %.1f ms vs evaluate %.1f ms (frames cached but slow to show)"
            % (disp["achieved_fps"], fps, disp["mean_render_ms"], disp["mean_eval_ms"])
        )
    if disp["count"] > 0 and disp.get("cadence_stutter"):
        msg = (
            "playback rate %.1f fps < target %.1f fps while per-frame display work is only "
            "%.2f ms -- new frames are arriving late (decode/scheduling), not display/render bound"
            % (disp.get("playback_fps", 0.0), fps, disp.get("mean_render_ms", 0.0) + disp.get("mean_eval_ms", 0.0))
        )
        if disp.get("stalls"):
            msg += " (%d visible stall(s) froze the frame > 1.5x budget)" % disp["stalls"]
        display_reasons.append(msg)

    audio_reasons = []
    if aud["steady"] > 0:
        audio_reasons.append("%d steady-state audio starvation event(s)" % aud["steady"])
    if aud["underruns"] > 0:
        audio_reasons.append("%d ALSA underrun(s)" % aud["underruns"])

    # Cache-content evidence: distinguishes "frame not in cache" (the on-the-fly
    # decode / cache-retention problem) from a pure pacing problem.
    cache_reasons = []
    if cache.get("count", 0) > 0:
        if cache["miss_rate"] > 0.01:
            msg = "%.1f%% of presented frames were cache MISSES (needed frame not resident)" % (
                100.0 * cache["miss_rate"]
            )
            if cache.get("med_full_at_miss", 0.0) >= 80.0:
                msg += "; at those misses the cache was ~%.0f%% full with median runway %d frame(s)" % (
                    cache["med_full_at_miss"],
                    cache.get("med_runway_at_miss", 0),
                )
                msg += " -> cache is full of the wrong frames (retention/eviction), not a pacing issue"
            if cache.get("otf_count", 0) > 0:
                msg += "; %d on-the-fly decode(s) blocked the display thread" % cache["otf_count"]
            cache_reasons.append(msg)
            if cache.get("cachestalls", 0) > 0:
                cache_reasons.append(
                    "%d cachestall event(s): caching refused to evict a protected frame to "
                    "cache the frame the player needed next" % cache["cachestalls"]
                )
        else:
            cache_reasons.append(
                "cache MISS rate ~0% (needed frames were resident) -> drops are NOT a cache-"
                "content problem; investigate pacing (render_v2 targetFrame/shift)"
            )

    frame_bound = bool(frame_reasons)
    audio_bound = bool(audio_reasons)
    display_bound = bool(display_reasons)

    # A confirmed cache-content problem: real misses while the cache is full.
    cache_miss_confirmed = (
        cache.get("count", 0) > 0 and cache["miss_rate"] > 0.01 and cache.get("med_full_at_miss", 0.0) >= 80.0
    )

    if cache_miss_confirmed:
        print("  Cause: CACHE CONTENT -- the frame the player needs is often NOT in the cache")
        print("         even though the cache is nearly full (on-the-fly decode stalls the")
        print("         display). This is a cache retention/look-ahead problem, not raw decode.")
    elif display_bound and not frame_bound and not audio_bound:
        print("  Cause: DISPLAY/RENDER path is the bottleneck -- frames are cached but the")
        print("         on-screen loop (GPU upload/render) can't hit the target fps.")
    elif frame_bound and audio_bound:
        print("  Cause: BOTH frame decode AND audio are contributing to the stutter.")
    elif frame_bound:
        print("  Cause: FRAME (image/EXR) DECODING is too slow -- this is the bottleneck.")
    elif audio_bound:
        print("  Cause: AUDIO decoding/starvation is the bottleneck.")
    elif display_bound:
        print("  Cause: DISPLAY/RENDER path is a bottleneck (in addition to the above).")
    else:
        print("  Cause: no clear stutter signature in this capture.")
        print("  (Try a longer capture during the stutter, confirm RV_PLAYBACK_DIAG=1,")
        print("   and pass the real playback --fps.)")

    if frame_reasons:
        print("  Frame-side evidence:")
        for r in frame_reasons:
            print("    - " + r)
    if display_reasons:
        print("  Display-side evidence:")
        for r in display_reasons:
            print("    - " + r)
    if audio_reasons:
        print("  Audio-side evidence:")
        for r in audio_reasons:
            print("    - " + r)
    if cache_reasons:
        print("  Cache-side evidence:")
        for r in cache_reasons:
            print("    - " + r)


def main(argv):
    ap = argparse.ArgumentParser(description="Attribute OpenRV playback stutter to frame vs audio decode.")
    ap.add_argument(
        "log",
        nargs="?",
        default="rv-playback-diag.log",
        help="path to the RV_PLAYBACK_DIAG log (default: rv-playback-diag.log)",
    )
    ap.add_argument("--fps", type=float, default=24.0, help="target playback fps of the session (default: 24)")
    ap.add_argument("--rvprof", default=None, help="optional .rvprof file from -debug profile")
    args = ap.parse_args(argv)

    if not os.path.isfile(args.log):
        # Do not interpolate untrusted paths into shells or file APIs beyond this
        # read; argparse value is used only as a direct path to open() for reading.
        sys.stderr.write("ERROR: log file not found: %s\n" % args.log)
        return 2

    rows = read_diag_log(args.log)
    if not rows:
        sys.stderr.write("ERROR: no diagnostic rows parsed from %s\n" % args.log)
        return 2

    t_min = min(r["t_ms"] for r in rows)
    t_max = max(r["t_ms"] for r in rows)
    span_s = max(0.0, (t_max - t_min) / 1000.0)

    print("OpenRV playback diagnostics: %s" % args.log)
    print("  rows=%d  span=%.1f s  target_fps=%.3f" % (len(rows), span_s, args.fps))
    print("")

    decode_rows = [r for r in rows if r["event"] == "decode"]
    dec = analyze_decode(decode_rows, args.fps, span_s)
    print("")
    aud = analyze_audio(rows, span_s)
    print("")
    buf = analyze_buffering(rows, span_s)
    print("")
    disp = analyze_display(rows, args.fps, span_s)
    print("")
    cache = analyze_cache(rows, args.fps, span_s)
    print("")
    analyze_stall_causes(rows, args.fps)
    print("")
    analyze_upload(rows, args.fps)
    print("")
    slow = analyze_slow_media(rows, span_s)
    if slow["any"]:
        print("")
    if args.rvprof:
        parse_rvprof(args.rvprof)
        print("")
    verdict(dec, aud, buf, slow, disp, cache, args.fps)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
