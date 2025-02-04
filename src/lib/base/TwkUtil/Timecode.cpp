//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkUtil/Timecode.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace TwkUtil
{
    using namespace std;

    namespace
    {

        typedef TwkMath::Time Time;

        void tcResidual(int frame, TCTime fps, Timecode& tc)
        {
            bool neg = frame < 0;
            if (neg)
                frame *= -1;

            const Time dsec = TCTime(frame) / TCTime(fps);
            const int sec = std::floor(dsec);
            const int min = sec / 60;
            const int hrs = min / 60;
            const int fsec = std::ceil(sec * TCTime(fps));
            const int fres = frame - fsec;
            const int frm = fres; // WAS: std::floor((dsec - sec) * Time(fps));
            const int fpad = fps >= 1000.0 ? 4 : (fps >= 100.0 ? 3 : 2);

#if 0
    if (frm != fres)
    {
        cout << "frame = " << frame << ", fps = " << fps << ", frm = " << frm << ", fres = " << fres << endl;
    }
#endif

            tc.rawFrame = frame;
            tc.rawFPS = fps;
            tc.frames = frm;
            tc.seconds = sec % 60;
            tc.minutes = min % 60;
            tc.hours = hrs;
            tc.padding = fpad;
            tc.type = TCResidual;
        }

        void tcDropFrame(int frame, TCTime fps, Timecode& tc)
        {
            const Time dsec = TCTime(frame) / TCTime(fps);
            const int sec = std::floor(dsec);
            const int min = std::floor(dsec / 60.0);
            const int hrs = std::floor(dsec / 60.0 / 60.0);
        }

        void tcMonotonic(int frame, Time fps, Timecode& tc)
        {
            return tcResidual(frame, std::ceil(fps), tc);
        }

    } // namespace

    Timecode timecodeFromFrameNumber(int frame, TCTime fps, TCType type)
    {
        Timecode tc;
        Time ffps = std::floor(fps);

        if (fps == 0.0)
        {
            tc.rawFrame = frame;
            return tc;
        }

        if (ffps == fps || type == TCResidual)
            tcResidual(frame, fps, tc);
        else if (type == TCDropFrame)
            tcDropFrame(frame, fps, tc);
        else
            tcMonotonic(frame, fps, tc);

        return tc;
    }

    namespace
    {
        unsigned int digits(int f)
        {
            //
            //  floor(log10(f)) + 1, but just in case somebody uses int64 here
            //  in the future this will continue to work.
            //

            unsigned int count = 0;
            for (; f; f /= 10)
                count++;
            return count;
        }
    } // namespace

    std::string timecodeToString(const Timecode& tc, int maxFrame, char tsep,
                                 char fsep, bool elideHours)
    {
        ostringstream out;
        out.fill('0');

        if (tc.rawFPS == TCTime(0.0))
        {
            out << "--" << tsep << "--" << tsep << "--" << fsep << "--";
            return out.str();
        }

        if (tc.rawFrame < 0)
            out << "-";

        int h = maxFrame ? std::floor(TCTime(maxFrame) / tc.rawFPS
                                      / TCTime(60.0) / TCTime(60.0))
                         : tc.hours;

        if (!elideHours && h != 0)
            out << setw(std::max((unsigned int)2, digits(h))) << tc.hours
                << tsep;

        out << setw(2) << tc.minutes << tsep << setw(2) << tc.seconds << fsep
            << setw(digits(std::floor(tc.rawFPS + 1.0))) << tc.frames;

        return out.str();
    }

    TCSeparatedString timecodeToSeparatedString(const Timecode& tc,
                                                int maxFrame, bool elideHours)
    {
        TCSeparatedString s;
        ostringstream out;

        if (tc.rawFPS != TCTime(0.0))
        {
            int h = maxFrame ? std::floor(TCTime(maxFrame) / tc.rawFPS
                                          / TCTime(60.0) / TCTime(60.0))
                             : tc.hours;

            if (!elideHours && h != 0)
            {
                if (tc.rawFrame < 0)
                    out << "-";
                out << setfill('0')
                    << setw(std::max((unsigned int)2, digits(h))) << tc.hours;
                s.hours = out.str();
                out.str("");
                out.clear();
            }
            else
            {
                if (tc.rawFrame < 0)
                    out << "-";
            }

            out << setfill('0') << setw(2) << tc.minutes;
            s.minutes = out.str();
            out.str("");
            out.clear();

            out << setfill('0') << setw(2) << tc.seconds;
            s.seconds = out.str();
            out.str("");
            out.clear();

            out << setfill('0') << setw(digits(std::floor(tc.rawFPS + 1.0)))
                << tc.frames;
            s.frames = out.str();
        }

        return s;
    }

} // namespace TwkUtil
