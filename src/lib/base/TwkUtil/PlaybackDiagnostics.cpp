//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/PlaybackDiagnostics.h>

#include <cstdlib>
#include <ios>
#include <string>

namespace TwkUtil
{
    using namespace std;

    bool PlaybackDiagnostics::enabled()
    {
        //
        //  Cached once: RV_PLAYBACK_DIAG is interpreted purely as an on/off
        //  boolean (non-empty and not "0"). It is never used to build a file
        //  path or otherwise passed to the filesystem, so there is no
        //  path-injection risk from this environment variable.
        //
        static const bool e = []() -> bool
        {
            const char* v = getenv("RV_PLAYBACK_DIAG");
            if (v == nullptr || v[0] == '\0')
                return false;
            if (v[0] == '0' && v[1] == '\0')
                return false;
            return true;
        }();
        return e;
    }

    PlaybackDiagnostics& PlaybackDiagnostics::instance()
    {
        static PlaybackDiagnostics s;
        return s;
    }

    PlaybackDiagnostics::PlaybackDiagnostics()
        : m_ok(false)
    {
        //
        //  The output path is a hard-coded constant filename in the current
        //  working directory. No environment variable or other external input
        //  is used to construct it, so there is no file-path injection risk.
        //
        m_file.open("rv-playback-diag.log", ios::out | ios::trunc);
        if (m_file.is_open())
        {
            m_file << "t_ms,event,thread,frame,dur_ms,extra\n";
            m_file.flush();
            m_ok = true;
        }

        m_timer.start();
    }

    PlaybackDiagnostics::~PlaybackDiagnostics()
    {
        if (m_file.is_open())
            m_file.close();
    }

    void PlaybackDiagnostics::record(const char* event, int threadId, int frame, double durMs, const std::string& extra)
    {
        if (!enabled())
            return;

        //
        //  Sanitize the free-form "extra" field so it can never corrupt the CSV
        //  structure: a comma would be read as a new field and a newline as a
        //  new row. Replace commas with semicolons and any newline/carriage
        //  return with a space.
        //
        string safeExtra;
        safeExtra.reserve(extra.size());
        for (char c : extra)
        {
            if (c == ',')
                safeExtra.push_back(';');
            else if (c == '\n' || c == '\r')
                safeExtra.push_back(' ');
            else
                safeExtra.push_back(c);
        }

        const double tMs = m_timer.elapsed() * 1000.0;

        lock_guard<mutex> lock(m_mutex);

        if (!m_ok)
            return;

        m_file << tMs << ',' << (event ? event : "") << ',' << threadId << ',' << frame << ',' << durMs << ',' << safeExtra << '\n';
        m_file.flush();
    }

} // namespace TwkUtil
