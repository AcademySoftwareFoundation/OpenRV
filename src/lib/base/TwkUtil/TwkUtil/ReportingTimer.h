//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilReportingTimer_h_
#define _TwkUtilReportingTimer_h_

#include <TwkUtil/Timer.h>
#include <iostream>

namespace TwkUtil
{

    //******************************************************************************
    class TWKUTIL_EXPORT ReportingTimer
    {
    public:
        ReportingTimer(float interval);

        void start()
        {
            m_last = 0.0f;
            m_timer.start();
        }

        void report(int i, int of, std::ostream& ostr);
        void reportFinished(std::ostream& ostr);

        void stop() { m_timer.stop(); }

    protected:
        float m_last;
        float m_interval;
        TwkUtil::Timer m_timer;
    };

    //******************************************************************************
    // INLINE FUNCTIONS
    //******************************************************************************
    inline ReportingTimer::ReportingTimer(float ivl)
        : m_last(0.0f)
        , m_interval(ivl)
    {
        // Nothing
    }

    //******************************************************************************
    inline void ReportingTimer::report(int i, int of, std::ostream& ostr)
    {
        float elps = m_timer.elapsed();

        if (elps > 1.0f)
        {
            if (elps - m_last > m_interval)
            {
                ostr << "\r" << m_timer.estimateReport(i, of) << std::flush;
                m_last = elps;
            }
        }
    }

    //******************************************************************************
    inline void ReportingTimer::reportFinished(std::ostream& ostr)
    {
        if (m_timer.elapsed() > 1.0f)
        {
            ostr << "\r" << m_timer.estimateReport(9, 10) << std::endl;
        }
    }

} // End namespace TwkUtil

#endif
