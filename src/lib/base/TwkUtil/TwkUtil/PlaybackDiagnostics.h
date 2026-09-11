//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilPlaybackDiagnostics_h_
#define _TwkUtilPlaybackDiagnostics_h_

#include <string>
#include <fstream>
#include <mutex>
#include <TwkUtil/Timer.h>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    //
    //  Lightweight, thread-safe diagnostic logger used to attribute playback
    //  stuttering to frame (image) decoding versus audio decoding/starvation.
    //
    //  Enabled by setting the RV_PLAYBACK_DIAG environment variable to a
    //  non-empty value other than "0". When enabled it writes CSV rows to a
    //  fixed file named "rv-playback-diag.log" in the process's current working
    //  directory.
    //
    //  Security note: the output path is a hard-coded, constant filename. No
    //  environment or other external input is ever used to build the path, so
    //  there is no file-path injection risk (RV_PLAYBACK_DIAG is only ever
    //  interpreted as an on/off boolean, never as a path).
    //
    //  Row schema (header written on open):
    //    t_ms,event,thread,frame,dur_ms,extra
    //      t_ms   - milliseconds since diagnostics were initialized
    //      event  - decode | cachemiss | audiolocked | buffering | resume |
    //               skip | underrun
    //      thread - caching/worker thread id (-1 when not applicable)
    //      frame  - frame number (-1 when not applicable)
    //      dur_ms - duration in milliseconds (decode time), or a context value
    //               such as look-ahead seconds depending on the event
    //      extra  - free-form key=value context
    //

    class TWKUTIL_EXPORT PlaybackDiagnostics
    {
    public:
        //  Cached check of the RV_PLAYBACK_DIAG environment variable. Cheap
        //  enough to call on hot paths: callers should build the (more
        //  expensive) log arguments only when this returns true.
        static bool enabled();

        static PlaybackDiagnostics& instance();

        void record(const char* event, int threadId, int frame, double durMs, const std::string& extra = std::string());

    private:
        PlaybackDiagnostics();
        ~PlaybackDiagnostics();

        PlaybackDiagnostics(const PlaybackDiagnostics&) = delete;
        PlaybackDiagnostics& operator=(const PlaybackDiagnostics&) = delete;

        std::mutex m_mutex;
        std::ofstream m_file;
        Timer m_timer;
        bool m_ok;
    };

} // namespace TwkUtil

#endif
