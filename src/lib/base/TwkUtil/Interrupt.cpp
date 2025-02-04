//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/Interrupt.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace TwkUtil
{
    using namespace std;

    bool Interrupt::flag = false;
    Interrupt::Computations Interrupt::m_computations;
    Interrupt::ProgressFuncs Interrupt::m_progressFuncs;
    Interrupt::MessageFuncs Interrupt::m_errorFuncs;
    Interrupt::MessageFuncs Interrupt::m_warningFuncs;
    Interrupt::MessageFuncs Interrupt::m_infoFuncs;
    pthread_t Interrupt::m_mainThread;
    bool Interrupt::m_checkThread = false;

    void Interrupt::setMainThread()
    {
        m_mainThread = pthread_self();
        m_checkThread = true;
    }

    void Interrupt::addProgressFunc(ProgressFunc pf)
    {
        m_progressFuncs.push_back(pf);
    }

    void Interrupt::removeProgressFunc(ProgressFunc pf)
    {
        stl_ext::remove(m_progressFuncs, pf);
    }

    void Interrupt::addMessageFunc(MessageFuncs& funcs, MessageFunc f)
    {
        funcs.push_back(f);
    }

    void Interrupt::removeMessageFunc(MessageFuncs& funcs, MessageFunc f)
    {
        stl_ext::remove(funcs, f);
    }

    void Interrupt::callMessageFuncs(const MessageFuncs& funcs,
                                     const std::string& message)
    {
        for (int i = 0; i < funcs.size(); i++)
        {
            MessageFunc f = funcs[i];
            f(message);
        }
    }

    void Interrupt::callProgressFuncs()
    {
        for (int i = 0; i < m_progressFuncs.size(); i++)
        {
            ProgressFunc f = m_progressFuncs[i];
            f(m_computations.back());
        }
    }

    void Interrupt::beginComputation(const std::string& message)
    {
#if defined _MSC_VER
        if (m_checkThread)
            if (pthread_self().p != m_mainThread.p)
                return;
#else
        if (m_checkThread)
            if (pthread_self() != m_mainThread)
                return;
#endif

        m_computations.push_back(Computation(message));
        m_computations.back().timer.start();
    }

    void Interrupt::continueComputation(float percent)
    {
        /* AJG - this might not work that well */
#if defined _MSC_VER
        if (m_checkThread)
            if (pthread_self().p != m_mainThread.p)
                return;
#else
        if (m_checkThread)
            if (pthread_self() != m_mainThread)
                return;
#endif

        Computation& c = m_computations.back();

        c.percentDone = percent;
        float elapsed = c.timer.elapsed();

        if ((elapsed - c.lastCallTime >= 0.1) && (elapsed > 0.5))
        {
            callProgressFuncs();
            c.lastCallTime = elapsed;
        }
    }

    void Interrupt::endComputation(const std::string& message)
    {
        /* AJG - this might not work that well */
#if defined _MSC_VER
        if (m_checkThread)
            if (pthread_self().p != m_mainThread.p)
                return;
#else
        if (m_checkThread)
            if (pthread_self() != m_mainThread)
                return;
#endif

        Computation& c = m_computations.back();
        c.message = message;
        c.percentDone = 1.0;
        c.timer.stop();
        if (c.timer.elapsed() > 0.5)
            callProgressFuncs();
        m_computations.pop_back();
    }

    void Interrupt::error(const std::string& message)
    {
        if (m_errorFuncs.empty())
        {
            cerr << "ERROR: " << message << endl;
        }
        else
        {
            callMessageFuncs(m_errorFuncs, message);
        }
    }

    void Interrupt::warning(const std::string& message)
    {
        if (m_warningFuncs.empty())
        {
            cerr << "WARNING: " << message << endl;
        }
        else
        {
            callMessageFuncs(m_warningFuncs, message);
        }
    }

    void Interrupt::info(const std::string& message)
    {
        if (m_infoFuncs.empty())
        {
            cerr << "INFO: " << message << endl;
        }
        else
        {
            callMessageFuncs(m_infoFuncs, message);
        }
    }

} // End namespace TwkUtil
