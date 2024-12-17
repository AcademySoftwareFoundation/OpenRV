//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TWKUTILINTERRUPT_H__
#define __TWKUTILINTERRUPT_H__
#include <TwkUtil/Timer.h>
#include <vector>
#include <string>
#include <pthread.h>
#include <TwkUtil/dll_defs.h>

//
//  Progress indication from the deep
//
//  This class is used by computationally intensive threads to see if
//  the user has requested that processing be stopped.  The flag is
//  initialized to false.  The UI thread can set the flag to true if
//  ESC is hit, etc...
//

namespace TwkUtil
{

    class TWKUTIL_EXPORT Interrupt
    {
    public:
        //
        //  Types
        //

        struct TWKUTIL_EXPORT Computation
        {
            Computation(const std::string& s)
                : message(s)
                , percentDone(0.0f)
                , lastCallTime(0.0f)
            {
            }

            Timer timer;
            std::string message;
            float percentDone;
            float lastCallTime;
        };

        typedef std::vector<Computation> Computations;
        typedef void (*ProgressFunc)(const Computation&);
        typedef std::vector<ProgressFunc> ProgressFuncs;
        typedef void (*MessageFunc)(const std::string&);
        typedef std::vector<MessageFunc> MessageFuncs;

        static bool flag;

        //
        //  Progress indication.
        //  Stacking is allowed, non-linear usage will not work.
        //
        //  e.g. this is ok:
        //
        //      beginComputation("heavy duty");
        //      continueCompuation(0.1);
        //      continueCompuation(0.2);
        //      ...
        //          beginComputation("sub heavy duty");
        //          continueCompuation(0.1);
        //          ...
        //          endComputation("done with sub heavy duty");
        //      continueCompuation(0.3); // referes to "heavy duty"
        //      endComputation();
        //

        static void addProgressFunc(ProgressFunc);
        static void removeProgressFunc(ProgressFunc);

        static bool isWorking() { return m_computations.size() != 0; }

        static const Computation& currentComputation()
        {
            return m_computations.front();
        }

        static void beginComputation(const std::string& message);
        static void continueComputation(float percent);
        static void endComputation(const std::string& message = "");

        //
        //  Messages (ERROR, WARNING, ETC)
        //

        static void addMessageFunc(MessageFuncs&, MessageFunc);
        static void removeMessageFunc(MessageFuncs&, MessageFunc);

        static void addWarningFunc(MessageFunc m)
        {
            addMessageFunc(m_warningFuncs, m);
        }

        static void addErrorFunc(MessageFunc m)
        {
            addMessageFunc(m_errorFuncs, m);
        }

        static void addInfoFunc(MessageFunc m)
        {
            addMessageFunc(m_infoFuncs, m);
        }

        //
        //  Messages: INFO, ERROR, WARNING
        //

        static void error(const std::string&);
        static void warning(const std::string&);
        static void info(const std::string&);

        //
        //  CAll this with the main thread
        //

        static void setMainThread();

    private:
        Interrupt() {}

        static void callProgressFuncs();
        static void callMessageFuncs(const MessageFuncs&, const std::string&);

    private:
        static Computations m_computations;
        static ProgressFuncs m_progressFuncs;
        static MessageFuncs m_warningFuncs;
        static MessageFuncs m_errorFuncs;
        static MessageFuncs m_infoFuncs;
        static bool m_checkThread;
        static pthread_t m_mainThread;
    };

} // End namespace TwkUtil

#endif // End #ifdef __TWKUTILINTERRUPT_H__
