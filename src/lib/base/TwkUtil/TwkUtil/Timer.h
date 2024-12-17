//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilTimer_h_
#define _TwkUtilTimer_h_
#include <string>

#if defined _MSC_VER
#include <time.h>
// mR - 10/28/07 - winsock2 is causing a multi-defined error - do we need it?
#include <winsock2.h>
// #include <winsock.h>
#elif defined PLATFORM_LINUX
#include <sys/time.h>
#endif

#ifdef PLATFORM_APPLE_MACH_BSD
#include <sys/time.h>
#endif

#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    //  Basic real-time stopwatch. Returns elapsed time in seconds.
    class TWKUTIL_EXPORT Timer
    {
    public:
        Timer(bool start = false);

        void start();
        double stop();
        double elapsed() const;
        size_t usecElapsed() const;

        // first arg n second arg number of n's
        double estimate(int n, int ofN) const;

        static void hms(double secondsTotal, int& hours, int& minutes,
                        double& seconds);

        std::string estimateReport(int n, int ofN, bool remaining = true) const;

        bool isRunning() const { return m_running; }

        //
        //  Need to wrap this on windows.  Wrapper is no-op on linux.
        //
        int gettimeofdayWrapper(struct timeval*) const;

    private:
        timeval m_tp0;
        timeval m_tp1;
        size_t m_usecElapsed;
        bool m_running;
        double m_stopped;
#ifdef PLATFORM_WINDOWS
        mutable bool m_winFirst;
        mutable long m_winSec;
        mutable long m_winUsec;
        mutable unsigned __int64 m_winBaseTicks;
#endif
    };

    //
    //  Timer instance you can put on the stack, but avoid even construction
    //  cost if "debug" flag to constructor is not true.
    //

    class TWKUTIL_EXPORT DebugTimer
    {
    public:
        DebugTimer(bool debug, bool start = false)
            : m_timer(0)
        {
            if (debug)
                m_timer = new Timer(start);
        }

        ~DebugTimer()
        {
            if (m_timer)
                delete m_timer;
        }

        void start()
        {
            if (m_timer)
                m_timer->start();
        }

        double stop() { return (m_timer) ? m_timer->stop() : 0.0; }

        double elapsed() { return (m_timer) ? m_timer->elapsed() : 0.0; }

        size_t usecElapsed() { return (m_timer) ? m_timer->usecElapsed() : 0; }

        bool isRunning() { return (m_timer) ? m_timer->isRunning() : false; }

    private:
        Timer* m_timer;
    };

} // End namespace TwkUtil

#endif
