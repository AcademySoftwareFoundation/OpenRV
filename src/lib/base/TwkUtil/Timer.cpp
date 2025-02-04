//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/Timer.h>

/* AJG - windows time */
#if defined PLATFORM_WINDOWS
#include <time.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <string.h>
#include <stdio.h>

#ifndef RUSAGE_SELF
#define RUSAGE_SELF 0
#endif

namespace TwkUtil
{

#if !defined(PLATFORM_WINDOWS)

    int Timer::gettimeofdayWrapper(struct timeval* tp) const
    {
        return gettimeofday(tp, 0);
    }

#else

#if 1
    //
    //  XP Has no GetCurrentProcessorNumber(), so make our own.  This
    //  only works on x86 processors.
    //

#if 0
DWORD GetCurrentProcessorNumberXP()
{
    _asm {mov eax, 1}
    _asm {cpuid}
    _asm {shr ebx, 24}
    _asm {mov eax, ebx}
}
#endif

    //
    //  Note that the actual time provided by the below has no relation
    //  to clock time, but we don't care since we're only looking at
    //  intervals.
    //

    static bool firstTimerConstructed = true;
    static bool debugTimers = false;

    int Timer::gettimeofdayWrapper(struct timeval* tp) const
    {
        //
        //  Get current tick count and frequency (ticks/second)
        //
        unsigned __int64 tickFrequency, ticks;

        QueryPerformanceFrequency((LARGE_INTEGER*)&tickFrequency);
        QueryPerformanceCounter((LARGE_INTEGER*)&ticks);

        if (firstTimerConstructed)
        {
            if (getenv("RV_DEBUG_TIMERS"))
                debugTimers = true;
            firstTimerConstructed = false;
            if (debugTimers)
                fprintf(stderr, "WIN PERFORMANCE TIMER FREQUENCY %I64u\n",
                        tickFrequency);
        }

        if (m_winFirst)
        {
            //  cpu = GetCurrentProcessorNumberXP();
            m_winBaseTicks = ticks;
            m_winFirst = false;
        }

        //
        //  NOTE: testing shows that performace counter numbers are
        //      cpu-independent, so hold this code in reserve if we ever
        //      discover situations where they're not synced.
        //
        //      if we had to we could put the baseTicks in a map, keyed
        //      on processor number.  but it's nice to not have to do
        //      that lookup.
        //

        /*
        int newCpu = GetCurrentProcessorNumberXP();
        if (newCpu != cpu)
        {
            fprintf (stderr, "*************** current processor changed (%d)
        !\n", newCpu); cpu = newCpu;
        }
        */
        if (ticks < m_winBaseTicks)
        {
            if (debugTimers)
            {
                fprintf(stderr, "*************** tick counter decreased !\n");
                fprintf(stderr, "                baseTicks %I64u ticks %I64u\n",
                        m_winBaseTicks, ticks);
            }
            m_winBaseTicks = ticks;
        }

        //
        //  Get ticks since last call
        //
        ticks -= m_winBaseTicks;
        m_winBaseTicks += ticks;

        //
        //  Calc secs and usecs since last call, using current freq.
        //  Note that this'll be off if the freq changed in the mean
        //  time, but hopefully that'll be rare.
        //
        //  Incremental secs and usecs are added to stored totals.
        //

        m_winSec += ticks / tickFrequency;

        long newUsec =
            (long)(((ticks % tickFrequency) * 1000000i64) / tickFrequency);
        if (m_winUsec + newUsec > 1000000i64)
            ++m_winSec;

        m_winUsec = (m_winUsec + newUsec) % 1000000i64;

        tp->tv_sec = m_winSec;
        tp->tv_usec = m_winUsec;

        //  fprintf (stderr, "sec %u usec %u ticks %I64u leftover %I64u base
        //  %I64u\n", tp->tv_sec, tp->tv_usec, ticks, ticks % tickFrequency,
        //  m_winBaseTicks); fflush(stderr);

        return 0;
    }

#if 0
int
gettimeofday (struct timeval *tp, void *not_used)
{
    static unsigned long run_count = 0;
    static unsigned __int64 base_ticks, tick_frequency;
    unsigned __int64 ticks, nano, nanoOrig;
    bool first = true;

    if (first)
    {
        union {
            __int32 ints[2]; 
            FILETIME ft;
        } n;
        QueryPerformanceFrequency((LARGE_INTEGER*)&tick_frequency);
        QueryPerformanceCounter((LARGE_INTEGER*)&base_ticks);
        GetSystemTimeAsFileTime(&n.ft);
        nanoOrig = __int64(n.ints[0]) + 0x100000000i64*__int64(n.ints[1]);
        first = false;
    }

    QueryPerformanceCounter((LARGE_INTEGER*) &ticks);
    //  ticks -= base_ticks;
    nano = nanoOrig
            + 10000000i64 * (ticks / tick_frequency)
            +(10000000i64 * (ticks % tick_frequency)) / tick_frequency;

    /* seconds since epoch */
    tp->tv_sec = (long)((nano /*- EPOCH_BIAS*/) / 10000000i64);

    /* microseconds remaining */
    tp->tv_usec = (long)((nano / 10i64) % 1000000i64);

    //  fprintf (stderr, "sec %u usec %u ticks %I64u freq %I64u \n", tp->tv_sec, tp->tv_usec, ticks, tick_frequency);


    return 0;
}

int
gettimeofday (struct timeval *tp, void *not_used)
{
    static unsigned long run_count = 0;
    static unsigned __int64 base_ticks, tick_frequency;
    static FILETIME base_systime_as_filetime;
    unsigned __int64 ticks;
    FILETIME ft;

    if (first)
    {
        QueryPerformanceFrequency((LARGE_INTEGER*)&tick_frequency);
        QueryPerformanceCounter((LARGE_INTEGER*)&base_ticks);
        GetSystemTimeAsFileTime(&base_systime_as_filetime. ft_val);
        ft = base_systime_as_filetime;
        first = false;
    }
    else
    {
        QueryPerformanceCounter((LARGE_INTEGER*) &ticks);
        ticks -= base_ticks;
        ft = base_systime_as_filetime
                + 10000000i64 * (ticks / tick_frequency)
                +(10000000i64 * (ticks % tick_frequency)) / tick_frequency;
    }

    /* seconds since epoch */
    tp->tv_sec = (long)((ft - EPOCH_BIAS) / Const64(10000000));

    /* microseconds remaining */
    tp->tv_usec = (long)((ft / Const64(10)) % Const64(1000000));

    return 0;
}
#endif
#else
    /* AJG - heisted off a messageboard somewhere */
    int Timer::gettimeofdayWrapper(struct timeval* tv) const
    {
        union
        {
            // AJG - this used to be a LONG_LONG
            __int64 ns100; // time since 1 Jan 1601 in 100ns units
            FILETIME ft;
        } now;

        GetSystemTimeAsFileTime(&now.ft);
        tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL);
        tv->tv_sec = (long)((now.ns100 - 116444736000000000LL) / 10000000LL);

        return (0);
    }
#endif
#endif

    //******************************************************************************

    Timer::Timer(bool startNow)
        : m_stopped(-1)
        , m_running(false)
#ifdef PLATFORM_WINDOWS
        , m_winBaseTicks(0)
        , m_winSec(0)
        , m_winUsec(0)
        , m_winFirst(true)
#endif

    {
        m_tp0.tv_sec = 0;
        m_tp0.tv_usec = 0;
        m_tp1.tv_sec = 0;
        m_tp1.tv_usec = 0;
        if (startNow)
            start();
    }

    //******************************************************************************

    void Timer::start()
    {
        gettimeofdayWrapper(&m_tp0);
        m_usecElapsed = 0;
        m_stopped = -1.0;
        m_running = true;
    }

    //******************************************************************************

    double Timer::stop()
    {
        gettimeofdayWrapper(&m_tp1);
        m_running = false;

        m_usecElapsed =
            (size_t(m_tp1.tv_sec) * size_t(1000000) + size_t(m_tp1.tv_usec)
             - size_t(m_tp0.tv_sec) * size_t(1000000) - size_t(m_tp0.tv_usec));

        return m_stopped = m_usecElapsed * 1e-6;
    }

    //******************************************************************************

    double Timer::elapsed() const
    {
        if (m_stopped > 0)
        {
            return m_stopped;
        }

        struct timeval tp;
        gettimeofdayWrapper(&tp);

        size_t usecElapsed =
            (size_t(tp.tv_sec) * size_t(1000000) + size_t(tp.tv_usec)
             - size_t(m_tp0.tv_sec) * size_t(1000000) - size_t(m_tp0.tv_usec));

        return usecElapsed * 1e-6;
    }

    //******************************************************************************

    size_t Timer::usecElapsed() const
    {
        if (m_stopped > 0)
        {
            return m_usecElapsed;
        }

        struct timeval tp;
        gettimeofdayWrapper(&tp);

        size_t usecElapsed =
            (size_t(tp.tv_sec) * size_t(1000000) + size_t(tp.tv_usec)
             - size_t(m_tp0.tv_sec) * size_t(1000000) - size_t(m_tp0.tv_usec));

        return usecElapsed;
    }

    //******************************************************************************

    double Timer::estimate(int n, int ofn) const
    {
        //	watch out for precision loss
        if (n >= ofn)
        {
            return 0.0f;
        }
        int remaining = ofn - n;
        double e = elapsed();
        double one = e / double(n);
        return one * remaining;
    }

    //******************************************************************************
    std::string Timer::estimateReport(int n, int ofn, bool remaining) const
    {
        char temp[256];
        char temp2[256];
        char temp3[256];

        int hours, minutes;
        double secondsLeft;

        hms(elapsed(), hours, minutes, secondsLeft);
        sprintf(temp, "%02d:%02d:%02d", hours, minutes, int(secondsLeft));

        if (remaining)
        {
            hms(estimate(n, ofn), hours, minutes, secondsLeft);
            sprintf(temp2, "%02d:%02d:%02d", hours, minutes, int(secondsLeft));

            sprintf(temp3, "%d%% done, %s elapsed... %s remaining... ",
                    int(double(n) / double(ofn - 1) * 100.0), temp, temp2);
        }
        else
        {
            sprintf(temp3, "%d%% done, %s elapsed... ",
                    int(double(n) / double(ofn - 1) * 100.0), temp);
        }

        return std::string(temp3);
    }

    //******************************************************************************
    void Timer::hms(double sec, int& hours, int& minutes, double& seconds)
    {
        minutes = int(sec / 60.f);
        hours = int(double(minutes) / 60.f);
        minutes -= hours * 60;
        seconds = sec - double(minutes * 60 + hours * 3600);
    }

} // End namespace TwkUtil
