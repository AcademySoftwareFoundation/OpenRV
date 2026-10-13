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
    paint       - one GLWindow::paintGL pass; extra: paint=<whole paintGL ms>;
                  gap=<ms between prev paint-exit and this paint-entry:
                  swapBuffers/vsync + platform update-request delivery + event
                  loop. The viewport is a QOpenGLWindow on its own native
                  surface, so this does NOT include a full-window widget
                  composite; gaps from the older widget-composite present path
                  are not comparable>;
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
    with open(path, newline="") as log_file:
        reader = csv.reader(log_file)
        try:
            header = next(reader)
        except StopIteration:
            return rows
        # Tolerate a missing header (older logs) by detecting a numeric first col.
        if header and header[0].strip() != "t_ms":
            log_file.seek(0)
            reader = csv.reader(log_file)
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
    for row in decode_rows:
        per_thread[row["thread"]].append(row["dur_ms"])

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

    all_durs = sorted(row["dur_ms"] for row in decode_rows)
    n_threads = len(per_thread)
    mean_ms = mean(all_durs)
    over_budget = [duration for duration in all_durs if frame_budget_ms and duration > frame_budget_ms]
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
    intervals = [((row["t_ms"] - row["dur_ms"]), row["t_ms"]) for row in decode_rows]
    busy_union_ms = merged_interval_length(intervals)
    busy_sum_ms = sum(row["dur_ms"] for row in decode_rows)
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
    for row in decode_rows:
        extra_fields = parse_extra(row.get("extra", ""))
        if "concurrency" in extra_fields:
            try:
                by_conc[int(extra_fields["concurrency"])].append(row["dur_ms"])
            except ValueError:
                pass
    oversubscribed = False
    if by_conc:
        print("  decode time vs concurrency (does adding threads scale?):")
        base_med = None
        for conc_level in sorted(by_conc):
            durs = sorted(by_conc[conc_level])
            med = median(durs)
            if conc_level == 1:
                base_med = med
            ratio = (
                (" (%.1fx slower than solo)" % (med / base_med)) if base_med and base_med > 0 and conc_level > 1 else ""
            )
            # Effective aggregate throughput at this level: conc_level frames in flight,
            # each taking 'med' ms, so ~ conc_level / med * 1000 frames/s of capacity.
            eff = (conc_level / med * 1000.0) if med > 0 else 0.0
            print(
                "      concurrency %-2d : n=%-5d median %7.2f ms  eff %5.1f fps%s"
                % (conc_level, len(durs), med, eff, ratio)
            )
        # Flag oversubscription if decodes at high concurrency are much slower
        # than solo decodes (the aggregate barely improves or regresses).
        if base_med and base_med > 0:
            high_levels = [conc_level for conc_level in by_conc if conc_level >= 3]
            if high_levels:
                hi_med = median(sorted([duration for conc_level in high_levels for duration in by_conc[conc_level]]))
                if hi_med > base_med * 2.0:
                    oversubscribed = True
                    print(
                        "  NOTE: decodes are ~%.1fx slower when %d+ run at once than when solo."
                        % (hi_med / base_med, min(high_levels))
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
            key, _, value = tok.partition("=")
            out[key.strip()] = value.strip()
    return out


def analyze_display(rows, fps, span_s):
    """On-screen display pacing: evaluate (cache pull) vs GPU render/upload.

    In Play-All-Frames mode the achieved fps is bounded by 1/(eval+render) per
    displayed frame. If render dominates while frames are already cached, the
    stall is on the GPU/upload path rather than on decode.
    """
    disp = [row for row in rows if row["event"] == "display"]

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
    for row in disp:
        extra_fields = parse_extra(row["extra"])
        try:
            eval_ms = float(extra_fields.get("eval", "nan"))
            render_ms = float(extra_fields.get("render", "nan"))
        except ValueError:
            continue
        if eval_ms == eval_ms:  # not NaN
            evals.append(eval_ms)
        if render_ms == render_ms:
            renders.append(render_ms)
        totals.append(row["dur_ms"])
        if "threadedUpload" in extra_fields:
            threaded_upload = extra_fields["threadedUpload"] == "1"
        # dframe tells us whether the displayed frame number actually advanced.
        dframe = None
        if "dframe" in extra_fields:
            try:
                dframe = int(extra_fields["dframe"])
                if dframe == 0:
                    repeats += 1
                elif abs(dframe) == 1:
                    advances += 1
                else:
                    multi_skips += 1
            except ValueError:
                dframe = None
        disp_samples.append((row["t_ms"], dframe))
        if extra_fields.get("refreshes"):
            try:
                refresh_count = int(extra_fields["refreshes"])
                if refresh_count > 0:
                    refresh_counts.append(refresh_count)
            except ValueError:
                pass
        try:
            skipped_frames += abs(int(extra_fields.get("skipped", "0")))
        except ValueError:
            pass

    comps = defaultdict(int)
    comp_times = defaultdict(list)
    extra_channel_frames = 0
    decsrc_count = 0
    ch_examples = {}
    for row in rows:
        if row["event"] == "decsrc":
            extra_fields = parse_extra(row["extra"])
            pixel_type = extra_fields.get("type")
            if pixel_type:
                bit_types.add(pixel_type)
            compression = extra_fields.get("comp")
            if compression:
                comps[compression] += 1
                comp_times[compression].append(row["dur_ms"])
            if extra_fields.get("extraCh") == "1":
                extra_channel_frames += 1
            if extra_fields.get("chNames") and extra_fields.get("chDecoded") not in (None, "3", "4"):
                ch_examples.setdefault(extra_fields.get("chDecoded"), extra_fields.get("chNames"))
            decsrc_count += 1

    evals.sort()
    renders.sort()
    totals.sort()
    frame_count = len(totals)
    over = [frame_time for frame_time in totals if frame_budget_ms and frame_time > frame_budget_ms]
    achieved_fps = (1000.0 / mean(totals)) if totals and mean(totals) > 0 else 0.0

    print("  target fps            : %.3f  (frame budget %.2f ms)" % (fps, frame_budget_ms))
    print("  displayed frames      : %d" % frame_count)
    if threaded_upload is not None:
        print("  threaded upload       : %s" % ("ON" if threaded_upload else "OFF"))
    if bit_types:
        print("  decoded pixel types   : %s" % ", ".join(sorted(bit_types)))
    if comps:
        summary = ", ".join(
            "%s x%d" % (codec, count) for codec, count in sorted(comps.items(), key=lambda item: -item[1])
        )
        print("  EXR compression       : %s" % summary)
        slow_codecs = {"PIZ_COMPRESSION", "DWAA_COMPRESSION", "DWAB_COMPRESSION", "B44_COMPRESSION", "B44A_COMPRESSION"}
        seen_slow = [codec for codec in comps if codec in slow_codecs]
        if seen_slow:
            print("    NOTE: %s decode significantly slower than ZIP/ZIPS on CPU." % ", ".join(seen_slow))

        # Per-compression decode time, sorted slowest-median first. Uses the
        # decsrc decode duration (mov->imagesAtFrame) grouped by codec so we
        # can see which compression dominates the decode cost.
        print("  decode time by codec (ms):")

        def _label(codec):
            return {"?": "(movie/non-EXR)"}.get(codec, codec)

        rows_by_slow = sorted(comp_times.items(), key=lambda item: -median(sorted(item[1])))
        for codec, times in rows_by_slow:
            sorted_times = sorted(times)
            print(
                "      %-22s n=%-5d mean %7.2f  median %7.2f  p95 %7.2f  max %8.2f"
                % (
                    _label(codec),
                    len(sorted_times),
                    mean(sorted_times),
                    median(sorted_times),
                    percentile(sorted_times, 95.0),
                    sorted_times[-1],
                )
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
    print(
        "  frames over budget    : %d / %d (%.1f%%)"
        % (len(over), frame_count, 100.0 * len(over) / frame_count if frame_count else 0.0)
    )
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
        gap = disp_samples[i][0] - disp_samples[i - 1][0]
        if gap > 0:
            redraw_gaps.append(gap)

    advance_ts = [timestamp for (timestamp, dframe) in disp_samples if dframe is not None and dframe != 0]
    advance_gaps = []
    for i in range(1, len(advance_ts)):
        gap = advance_ts[i] - advance_ts[i - 1]
        if gap > 0:
            advance_gaps.append(gap)

    # A "stall" is an advance gap noticeably longer than the frame budget: the
    # frame froze on screen longer than it should have (the visible stutter).
    stall_gaps = [gap for gap in advance_gaps if frame_budget_ms and gap > frame_budget_ms * 1.5]

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
        for refresh_count in refresh_counts:
            hist[refresh_count] += 1
        avg_rc = sum(refresh_counts) / len(refresh_counts)
        redraw_ms = 1000.0 / redraw_hz if redraw_hz > 0 else 0.0
        print("    refreshes/new frame : mean %.2f  (redraws each new frame is held)" % avg_rc)
        for hold_count in sorted(hist):
            share = 100.0 * hist[hold_count] / len(refresh_counts)
            print("        held %d redraw(s) : %5d  (%4.1f%%)" % (hold_count, hist[hold_count], share))
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
        "count": frame_count,
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
    misses = [row for row in rows if row["event"] == "cachemiss"]
    locked = [row for row in rows if row["event"] == "audiolocked"]
    underruns = [row for row in rows if row["event"] == "underrun"]

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
    steady = [row for row in (misses + locked + underruns) if row["t_ms"] > 2000.0]
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
    events = [row for row in rows if row["event"] in ("buffering", "resume")]
    buffering = [row for row in rows if row["event"] == "buffering"]
    skips = [row for row in rows if row["event"] == "skip"]

    print("=" * 72)
    print("PLAYBACK CONTROL  --  buffering pauses and realtime frame skips")
    print("=" * 72)
    print("  buffering pauses      : %d" % len(buffering))
    print(
        "  realtime frame skips  : %d event(s), %d frame(s) dropped"
        % (len(skips), sum(abs(int(row["dur_ms"])) for row in skips))
    )

    # Pair buffering -> resume in time order to estimate pause durations.
    pause_durations = []
    open_pause_t = None
    for row in sorted(events, key=lambda entry: entry["t_ms"]):
        if row["event"] == "buffering" and open_pause_t is None:
            open_pause_t = row["t_ms"]
        elif row["event"] == "resume" and open_pause_t is not None:
            pause_durations.append(row["t_ms"] - open_pause_t)
            open_pause_t = None
    if pause_durations:
        sorted_pauses = sorted(pause_durations)
        print(
            "  pause duration (ms)   : n=%d  mean %.1f  median %.1f  max %.1f  total %.1f"
            % (len(sorted_pauses), mean(sorted_pauses), median(sorted_pauses), sorted_pauses[-1], sum(sorted_pauses))
        )
        if span_s > 0:
            print("  time spent buffering  : %.1f%% of the run" % (100.0 * sum(sorted_pauses) / (span_s * 1000.0)))
    return {
        "buffering": len(buffering),
        "skips": len(skips),
        "skipped_frames": sum(abs(int(row["dur_ms"])) for row in skips),
        "pause_total_ms": sum(pause_durations),
    }


def analyze_slow_media(rows, span_s):
    """Report per-source slowRandomAccess flags and slow-media mode transitions."""
    sources = [row for row in rows if row["event"] == "sourceinfo"]
    transitions = sorted((row for row in rows if row["event"] == "slowmedia"), key=lambda entry: entry["t_ms"])

    if not sources and not transitions:
        return {"slow_sources": 0, "slow_fraction": 0.0, "any": False}

    print("=" * 72)
    print("SLOW-MEDIA MODE  --  why decode may be restricted to one thread")
    print("=" * 72)

    slow_sources = 0
    if sources:
        print("  sources opened:")
        for row in sources:
            flag = "SLOW random access" if row["dur_ms"] >= 0.5 else "fast"
            if row["dur_ms"] >= 0.5:
                slow_sources += 1
            print("      [%s] %s" % (flag, row["extra"]))
    else:
        print("  (no per-source info recorded)")

    # Estimate the fraction of the run spent in slow-media mode by integrating
    # the on/off transitions over time. Assume mode starts "off".
    slow_ms = 0.0
    state_on = False
    last_t = transitions[0]["t_ms"] if transitions else 0.0
    end_t = max((row["t_ms"] for row in rows), default=0.0)
    for transition in transitions:
        if state_on:
            slow_ms += transition["t_ms"] - last_t
        state_on = transition["dur_ms"] >= 0.5  # 1.0=on, 0.0=off
        last_t = transition["t_ms"]
    if state_on:
        slow_ms += end_t - last_t
    slow_fraction = (slow_ms / (span_s * 1000.0)) if span_s > 0 else 0.0

    print("  slow-media transitions: %d" % len(transitions))
    if transitions:
        onc = sum(1 for transition in transitions if transition["dur_ms"] >= 0.5)
        print("      turned ON %d time(s), OFF %d time(s)" % (onc, len(transitions) - onc))
        print(
            "      first ON at frame %d (t=%.1f s)"
            % (
                next((transition["frame"] for transition in transitions if transition["dur_ms"] >= 0.5), -1),
                next((transition["t_ms"] / 1000.0 for transition in transitions if transition["dur_ms"] >= 0.5), 0.0),
            )
        )
    print("  time in slow-media    : ~%.0f%% of the run" % (100.0 * slow_fraction))

    return {"slow_sources": slow_sources, "slow_fraction": slow_fraction, "any": True}


def parse_rvprof(path):
    """Light parse of an .rvprof file; report display-thread frame budget overruns."""
    records = []
    try:
        with open(path) as rvprof_file:
            for line in rvprof_file:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                fields = {}
                for tok in line.split(","):
                    if "=" in tok:
                        key, _, value = tok.partition("=")
                        try:
                            fields[key] = float(value)
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

    render_ms = sorted(1000.0 * (record["R1"] - record["R0"]) for record in records if record["R1"] >= record["R0"])
    io_ms = sorted(
        1000.0 * (record["IO1"] - record["IO0"])
        for record in records
        if "IO1" in record and "IO0" in record and record["IO1"] >= record["IO0"]
    )
    print("  frames profiled       : %d" % len(records))
    if render_ms:
        print(
            "  display render (ms)   : mean %.2f  median %.2f  p95 %.2f  max %.2f"
            % (mean(render_ms), median(render_ms), percentile(render_ms, 95.0), render_ms[-1])
        )
    if io_ms:
        nonzero = [io_time for io_time in io_ms if io_time > 0.01]
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
    display_cache_rows = [row for row in rows if row["event"] == "displaycache"]
    stalls = [row for row in rows if row["event"] == "cachestall"]

    print("=" * 72)
    print("CACHE CONTENT  --  was the needed frame actually in cache?")
    print("=" * 72)
    if not display_cache_rows:
        print("  No displaycache events recorded.")
        print("  (Rebuild with the cache-content diagnostic, or the run predates it.)")
        return {"count": 0, "miss_rate": 0.0, "misses": 0, "cachestalls": len(stalls)}

    frame_count = len(display_cache_rows)
    hits, misses = 0, 0
    miss_full = []  # cache percent-full at each miss
    miss_runway = []  # contiguous cached frames ahead at each miss
    miss_overflow = 0  # misses where the cache was overflowing (full)
    otf_ms = []  # on-the-fly decode time (display thread) at misses
    runway_all = []  # runway on every presented frame (hit or miss)
    zero_runway = 0  # presented frames whose NEXT frame was not cached
    full_all = []
    for row in display_cache_rows:
        extra_fields = parse_extra(row["extra"])
        is_hit = extra_fields.get("hit") == "1"
        try:
            runway = int(extra_fields.get("runway", "0"))
            runway_all.append(runway)
            if runway == 0:
                zero_runway += 1
        except ValueError:
            runway = None
        try:
            full_all.append(float(extra_fields.get("full", "0")))
        except ValueError:
            pass
        if is_hit:
            hits += 1
        else:
            misses += 1
            try:
                miss_full.append(float(extra_fields.get("full", "0")))
            except ValueError:
                pass
            if runway is not None:
                miss_runway.append(runway)
            if extra_fields.get("overflow") == "1":
                miss_overflow += 1
            try:
                otf = float(extra_fields.get("otfDecodeMs", "0"))
                if otf > 0:
                    otf_ms.append(otf)
            except ValueError:
                pass

    miss_rate = misses / frame_count if frame_count else 0.0
    print("  presented frames      : %d" % frame_count)
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
        print(
            "  next frame NOT cached : %d / %d presented frames (%.1f%%)"
            % (zero_runway, frame_count, 100.0 * zero_runway / frame_count)
        )

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
        for row in sample:
            extra_fields = parse_extra(row["extra"])
            print(
                "      want frame %s (util %s) but protected cached frame %s (util %s); displayFrame %s"
                % (
                    extra_fields.get("cacheFrame", "?"),
                    extra_fields.get("cacheUtil", "?"),
                    extra_fields.get("freeFrame", "?"),
                    extra_fields.get("freeUtil", "?"),
                    extra_fields.get("displayFrame", "?"),
                )
            )

    return {
        "count": frame_count,
        "misses": misses,
        "miss_rate": miss_rate,
        "miss_overflow": miss_overflow,
        "med_full_at_miss": med_full_miss,
        "med_runway_at_miss": med_runway_miss,
        "zero_runway_frac": (zero_runway / frame_count) if frame_count else 0.0,
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
    for row in rows:
        if row["event"] != "display":
            continue
        extra_fields = parse_extra(row["extra"])
        try:
            interval = float(extra_fields.get("interval", "0"))
            eval_ms = float(extra_fields.get("eval", "0"))
            render_ms = float(extra_fields.get("render", "0"))
        except ValueError:
            continue
        try:
            reqs = int(extra_fields.get("reqs", "-1"))
        except ValueError:
            reqs = -1
        try:
            outside_gap = float(extra_fields.get("outsideGap", "-1"))
        except ValueError:
            outside_gap = -1.0
        try:
            dframe = int(extra_fields.get("dframe", "0"))
        except ValueError:
            dframe = 0
        disp.append((row["t_ms"], interval, eval_ms, render_ms, reqs, outside_gap, dframe))

    print("=" * 72)
    print("STALL ANATOMY  --  where does a slow redraw spend its time?")
    print("=" * 72)
    if not disp or budget <= 0:
        print("  No display events with interval timing recorded.")
        return {"stalls": 0, "decode_overlap_frac": 0.0}

    # Decode windows [start,end] with the concurrency reported at decode start.
    decodes = []
    for row in rows:
        if row["event"] != "decode":
            continue
        extra_fields = parse_extra(row.get("extra", ""))
        try:
            conc = int(extra_fields.get("concurrency", "1"))
        except ValueError:
            conc = 1
        decodes.append((row["t_ms"] - row["dur_ms"], row["t_ms"], conc))
    decodes.sort()
    dec_starts = [decode[0] for decode in decodes]

    # Per-render event-processing handler samples (GUI-thread Mu/Python work
    # that runs between paints). Used to split the "outside render_v2" bucket
    # into event-loop-handler time vs present (swapBuffers/vsync) time.
    perrender = sorted((row["t_ms"], row["dur_ms"]) for row in rows if row["event"] == "perrender")
    pr_ts = [sample[0] for sample in perrender]

    # Optional GPU-completion probe (RV_DIAG_GLFINISH): paint events carry
    # gpuFinish = glFinish() time right after render(), i.e. how long the GPU
    # took to actually execute the frame's upload+shaders. If this is large on
    # slow paints, the stall is GPU upload/render bound; if it is small while the
    # present (gap) still stalls, the block is in the present path itself --
    # swapBuffers/vsync or the platform's update-request delivery. (It is no
    # longer attributable to a full-window composite: the viewport presents on
    # its own QOpenGLWindow surface and does not serialize with the window.)
    gpu_all = []
    gpu_gap = []  # (gap, gpuFinish) pairs for paints where glFinish was active
    for row in rows:
        if row["event"] != "paint":
            continue
        extra_fields = parse_extra(row["extra"])
        try:
            gpu_finish_ms = float(extra_fields.get("gpuFinish", "-1"))
            paint_gap_ms = float(extra_fields.get("gap", "-1"))
        except ValueError:
            continue
        if gpu_finish_ms >= 0:
            gpu_all.append(gpu_finish_ms)
            if paint_gap_ms >= 0:
                gpu_gap.append((paint_gap_ms, gpu_finish_ms))

    def perrender_in(start_ms, end_ms):
        """Sum of per-render handler time whose events fall within [start_ms, end_ms]."""
        import bisect

        low = bisect.bisect_left(pr_ts, start_ms)
        high = bisect.bisect_right(pr_ts, end_ms)
        return sum(perrender[i][1] for i in range(low, high))

    def max_conc_in(start_ms, end_ms):
        """Max decode concurrency for any decode window overlapping [start_ms, end_ms]."""
        import bisect

        # any decode with start <= end_ms and end >= start_ms
        high = bisect.bisect_right(dec_starts, end_ms)
        best = 0
        # scan backwards from high while decode start could still overlap; decode
        # durations are bounded (<~0.3s) but we simply scan the candidates whose
        # start < end_ms and check end >= start_ms.
        for i in range(high - 1, -1, -1):
            decode_start, decode_end, decode_conc = decodes[i]
            if decode_end >= start_ms:
                if decode_conc > best:
                    best = decode_conc
            # stop once starts are far enough before start_ms that no window reaches
            if decode_start < start_ms - 400.0:
                break
        return best

    slow = [display_event for display_event in disp if display_event[1] > budget * 1.5]
    outside_all = [
        max(0.0, display_event[1] - display_event[2] - display_event[3])
        for display_event in disp
        if display_event[1] > 0
    ]

    # New-frame vs repeat present time. In Play-All-Frames a repeat re-shows the
    # already-resident GPU texture (cheap present), while a NEW frame must upload
    # a fresh texture before the swap can present it. If the present cost is paid
    # almost entirely on NEW frames, the stall is the synchronous GPU texture
    # upload (threaded upload OFF), not a uniform desktop-compositor/vsync
    # problem -- which points the fix at async/threaded upload or prefetch.
    new_gap = sorted(display_event[5] for display_event in disp if display_event[6] != 0 and display_event[5] >= 0)
    rep_gap = sorted(display_event[5] for display_event in disp if display_event[6] == 0 and display_event[5] >= 0)
    if new_gap and rep_gap:
        stall_line = budget * 1.5
        new_stalls = sum(1 for gap in new_gap if gap > stall_line)
        rep_stalls = sum(1 for gap in rep_gap if gap > stall_line)
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
        # the new frame and/or the desktop compositor presenting changed
        # content), not a uniform per-present vsync cost (which would hit
        # repeats too). "Desktop compositor" here means the window manager's
        # compositor, which is still in the present path -- distinct from Qt's
        # widget composite, which the viewport no longer goes through.
        if total_stalls > 0 and new_stalls >= total_stalls * 0.8:
            print("      -> stalls occur almost only on NEW frames (repeats present fine).")
            print("         The cost is specific to presenting a NEW frame: either the")
            print("         synchronous GPU texture upload (threaded upload OFF) or the")
            print("         desktop compositor presenting changed content. Test both cheaply:")
            print("           * TWK_ALLOW_THREADED_UPLOAD=1  (overlaps upload with present)")
            print("           * disable desktop compositor / fullscreen-unredirect")
        else:
            print("      -> stalls hit repeats too: uniform desktop-compositor/vsync cost,")
            print("         not new-frame texture upload.")

    if outside_all:
        sorted_outside = sorted(outside_all)
        print("  'outside' time per redraw (interval - eval - render):")
        print(
            "      median %.2f ms  p95 %.2f ms  max %.2f ms"
            % (median(sorted_outside), percentile(sorted_outside, 95.0), sorted_outside[-1])
        )
        print("      (at 60Hz vsync ~16 ms of this is the normal swap wait)")

    if not slow:
        print("  No slow redraws (> 1.5x budget) -- pacing is clean.")
        return {"stalls": 0, "decode_overlap_frac": 0.0}

    # Heartbeat = 120Hz (RvApplication timer), so an on-time redraw loop posts
    # ~ interval/8.33ms redraw requests per actual paint. If during a stall the
    # number of coalesced requests (reqs) tracks the gap, RV DID ask for the
    # paint and Qt deferred it (present path). If reqs stays ~1, the
    # heartbeat/event loop itself was starved (RV never asked) -- or, on a
    # platform that gates requestUpdate() on the display refresh, the window's
    # update request was itself throttled.
    HEARTBEAT_MS = 1000.0 / 120.0
    slow_reqs = [display_event[4] for display_event in slow if display_event[4] >= 0]
    if slow_reqs:
        exp = [max(1.0, display_event[1] / HEARTBEAT_MS) for display_event in slow if display_event[4] >= 0]
        got = sorted(slow_reqs)
        print("  heartbeat requests coalesced per stalled paint:")
        print("      observed reqs/paint : median %.1f  p95 %.1f" % (median(got), percentile(got, 95.0)))
        print("      expected (if 120Hz) : median %.1f  (gap / 8.33 ms)" % median(sorted(exp)))
        if median(got) >= median(sorted(exp)) * 0.6:
            print("      -> RV posted redraws on time but Qt did NOT paint them: the")
            print("         stall is in the present path (QPaintDeviceWindow::update ->")
            print("         requestUpdate delivery, or the vsync swap), NOT RV's")
            print("         heartbeat or decode.")
        else:
            print("      -> few requests during stalls: the 120Hz heartbeat/event loop")
            print("         itself is being starved (main thread blocked) -- investigate")
            print("         what blocks Session::update between paints.")

    # Three-way split of a stalled interval (needs the outsideGap field):
    #   eval+render        : the measured display work (tiny)
    #   rest of render_v2  : interval - outsideGap - eval - render
    #                        (frameChangeEvent + graph work + locks)
    #   outside render_v2  : outsideGap
    #                        (paintGL rest + swapBuffers/vsync + update-request
    #                         delivery + event loop)
    slow_gap = [display_event for display_event in slow if display_event[5] >= 0]
    if slow_gap:
        outside_gaps = sorted(display_event[5] for display_event in slow_gap)
        inside = sorted(max(0.0, display_event[1] - display_event[5]) for display_event in slow_gap)
        rest_rv2 = sorted(
            max(0.0, display_event[1] - display_event[5] - display_event[2] - display_event[3])
            for display_event in slow_gap
        )
        print(
            "  stalled interval split (where the ~%.0f ms goes):"
            % median(sorted(display_event[1] for display_event in slow_gap))
        )
        print("      outside render_v2  : median %.2f ms  (present/swap + event loop)" % median(outside_gaps))
        print("      inside render_v2   : median %.2f ms  (eval + render + frameChangeEvent)" % median(inside))
        print(
            "      rest of render_v2  : median %.2f ms  (frameChangeEvent/graph, excl eval+render)" % median(rest_rv2)
        )

        # Split the "outside render_v2" bucket into the per-render event-loop
        # handler vs the swapBuffers/vsync present path, using perrender
        # events that land within each stalled between-paint window.
        if perrender:
            handler = []
            swap = []
            for display_event in slow_gap:
                timestamp, gap = display_event[0], display_event[5]
                handler_ms = perrender_in(timestamp - gap, timestamp)
                handler.append(handler_ms)
                swap.append(max(0.0, gap - handler_ms))
            median_handler_ms = median(sorted(handler))
            median_swap_ms = median(sorted(swap))
            print("      -- outside split (via perrender events) --")
            print("      event-loop handler : median %.2f ms  (per-render Mu/Python on GUI thread)" % median_handler_ms)
            print("      present + swap     : median %.2f ms  (swapBuffers / vsync / req delivery)" % median_swap_ms)
            if median_handler_ms >= median_swap_ms:
                print("      -> the stall is the PER-RENDER EVENT HANDLER blocking the GUI thread")
                print("         (userGenericEvent 'per-render-event-processing' -> Mu/Python).")
                print("         Fix target: make that handler cheap/async, not the present path.")
            else:
                print("      -> the stall is the PRESENT path blocking the GUI thread")
                print("         (swapBuffers/vsync on the viewport's QOpenGLWindow, or the")
                print("         platform deferring its update request). The viewport already")
                print("         presents directly on its own native surface, so look at vsync")
                print("         /swap interval and the desktop compositor, not Qt's widget")
                print("         composite.")

        # GPU-completion probe (only present when run with RV_DIAG_GLFINISH=1).
        if gpu_all:
            sorted_gpu = sorted(gpu_all)
            print("      -- GPU probe (RV_DIAG_GLFINISH: glFinish after render) --")
            print(
                "      gpuFinish (GPU exec) : median %.2f ms  p95 %.2f  max %.2f  (n=%d)"
                % (median(sorted_gpu), percentile(sorted_gpu, 95.0), sorted_gpu[-1], len(sorted_gpu))
            )
            slow_gpu = sorted(
                gpu_finish_ms
                for (paint_gap_ms, gpu_finish_ms) in gpu_gap
                if (paint_gap_ms + gpu_finish_ms) > budget * 1.5
            )
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
                print("         the next frame, or reduce upload cost -- NOT the present path.")
            elif percentile(sorted_gpu, 95.0) > budget * 1.5:
                print("      -> GPU execution stalls on the tail: upload/render bound.")
            else:
                print("      -> GPU finishes fast even on stalls: the block is AFTER the GPU is")
                print("         done -> the swapBuffers/vsync present path (or the desktop")
                print("         compositor), not upload.")
        elif median(outside_gaps) >= median(inside):
            print("      -> the stall is OUTSIDE render_v2 (present + event loop). Rebuild with the")
            print("         paint/perrender instrumentation to split swap vs event-handler.")

    s_iv = sorted(display_event[1] for display_event in slow)
    s_ev = sorted(display_event[2] for display_event in slow)
    s_rd = sorted(display_event[3] for display_event in slow)
    s_out = sorted(max(0.0, display_event[1] - display_event[2] - display_event[3]) for display_event in slow)
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
    for display_event in slow:
        timestamp, slow_interval = display_event[0], display_event[1]
        max_conc = max_conc_in(timestamp - slow_interval, timestamp)
        if max_conc >= 1:
            overlap_any += 1
        if max_conc >= 3:
            overlap_hi += 1
            iv_overlap_hi.append(slow_interval)
        else:
            iv_no_decode.append(slow_interval)
    slow_count = len(slow)
    frac_hi = overlap_hi / slow_count if slow_count else 0.0
    print("  decode overlap of slow redraws:")
    print(
        "      overlapped ANY decode      : %d / %d (%.0f%%)"
        % (overlap_any, slow_count, 100.0 * overlap_any / slow_count)
    )
    print("      overlapped 3+ concurrent   : %d / %d (%.0f%%)" % (overlap_hi, slow_count, 100.0 * frac_hi))
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
        "stalls": slow_count,
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
    ups = [row for row in rows if row["event"] == "upload"]
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
    for row in ups:
        extra_fields = parse_extra(row["extra"])
        cpu_ms.append(row["dur_ms"])
        try:
            cpu_gbps = float(extra_fields.get("cpuGBps", "0"))
        except ValueError:
            cpu_gbps = 0.0
        gbps.append(cpu_gbps)
        is_pbo = extra_fields.get("pbo", "0") == "1"
        if is_pbo:
            pbo_yes += 1
        if extra_fields.get("update", "0") == "1":
            update_yes += 1
        key = "%sx%s ch%s ty%s" % (
            extra_fields.get("w", "?"),
            extra_fields.get("h", "?"),
            extra_fields.get("ch", "?"),
            extra_fields.get("type", "?"),
        )
        bucket = by_res[key]
        bucket["n"] += 1
        bucket["cpu"].append(row["dur_ms"])
        bucket["gbps"].append(cpu_gbps)
        if is_pbo:
            bucket["pbo"] += 1
            bucket["pbo_gbps"].append(cpu_gbps)
        else:
            bucket["nopbo_gbps"].append(cpu_gbps)

    upload_count = len(ups)
    cpu_s = sorted(cpu_ms)
    gbps_s = sorted(gbps)
    pbo_frac = pbo_yes / upload_count

    print("  upload events         : %d plane upload(s)" % upload_count)
    print(
        "  PBO fast path used    : %d / %d (%.1f%%)   sub-image updates: %d"
        % (pbo_yes, upload_count, 100.0 * pbo_frac, update_yes)
    )
    print(
        "  CPU submit time (ms)  : mean %.2f  median %.2f  p95 %.2f  max %.2f"
        % (mean(cpu_ms), median(cpu_s), percentile(cpu_s, 95.0), cpu_s[-1])
    )
    print("  CPU submit throughput : median %.2f GB/s  min %.2f GB/s" % (median(gbps_s), gbps_s[0]))
    print("")
    print("  by resolution/format  (frame size drives upload cost):")
    for key, bucket in sorted(by_res.items(), key=lambda item: -item[1]["n"]):
        sorted_cpu = sorted(bucket["cpu"])
        sorted_gbps = sorted(bucket["gbps"])
        print(
            "    %-24s n=%-5d cpu med %.2f ms / p95 %.2f ms  cpuGBps med %.2f  pbo %d%%"
            % (
                key,
                bucket["n"],
                median(sorted_cpu),
                percentile(sorted_cpu, 95.0),
                median(sorted_gbps),
                int(round(100.0 * bucket["pbo"] / bucket["n"])),
            )
        )
        #  Split throughput PBO vs non-PBO: a plain memcpy into the PBO does
        #  not depend on RGB-vs-RGBA, so if PBO throughput is uniform across
        #  formats the CPU-submit cost is purely the non-PBO fallback; the
        #  RGB16F penalty then lives on the GPU side (gpuFinish).
        sorted_pbo_gbps = sorted(bucket["pbo_gbps"])
        sorted_nopbo_gbps = sorted(bucket["nopbo_gbps"])
        if sorted_pbo_gbps and sorted_nopbo_gbps:
            print(
                "        PBO med %.2f GB/s (n=%d)   non-PBO med %.2f GB/s (n=%d)"
                % (median(sorted_pbo_gbps), len(sorted_pbo_gbps), median(sorted_nopbo_gbps), len(sorted_nopbo_gbps))
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

    return {"count": upload_count, "pbo_frac": pbo_frac, "cpu_gbps_med": median(gbps_s)}


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
            #  Do NOT point at render_v2 targetFrame/shift here. That pacing
            #  theory was investigated and disproven (the measured vsync bias
            #  was -0.10, and the held-1 share explained the cadence); the cost
            #  was later traced to the GPU texture-upload path instead. Point at
            #  the upload/present metrics, which is where it actually lived.
            cache_reasons.append(
                "cache MISS rate ~0% (needed frames were resident) -> drops are NOT a cache-"
                "content problem; look at the upload path (pbo %, cpuGBps) and gpuFinish first"
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
        for reason in frame_reasons:
            print("    - " + reason)
    if display_reasons:
        print("  Display-side evidence:")
        for reason in display_reasons:
            print("    - " + reason)
    if audio_reasons:
        print("  Audio-side evidence:")
        for reason in audio_reasons:
            print("    - " + reason)
    if cache_reasons:
        print("  Cache-side evidence:")
        for reason in cache_reasons:
            print("    - " + reason)


def main(argv):
    parser = argparse.ArgumentParser(description="Attribute OpenRV playback stutter to frame vs audio decode.")
    parser.add_argument(
        "log",
        nargs="?",
        default="rv-playback-diag.log",
        help="path to the RV_PLAYBACK_DIAG log (default: rv-playback-diag.log)",
    )
    parser.add_argument("--fps", type=float, default=24.0, help="target playback fps of the session (default: 24)")
    parser.add_argument("--rvprof", default=None, help="optional .rvprof file from -debug profile")
    args = parser.parse_args(argv)

    if not os.path.isfile(args.log):
        # Do not interpolate untrusted paths into shells or file APIs beyond this
        # read; argparse value is used only as a direct path to open() for reading.
        sys.stderr.write("ERROR: log file not found: %s\n" % args.log)
        return 2

    rows = read_diag_log(args.log)
    if not rows:
        sys.stderr.write("ERROR: no diagnostic rows parsed from %s\n" % args.log)
        return 2

    t_min = min(row["t_ms"] for row in rows)
    t_max = max(row["t_ms"] for row in rows)
    span_s = max(0.0, (t_max - t_min) / 1000.0)

    print("OpenRV playback diagnostics: %s" % args.log)
    print("  rows=%d  span=%.1f s  target_fps=%.3f" % (len(rows), span_s, args.fps))
    print("")

    decode_rows = [row for row in rows if row["event"] == "decode"]
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
