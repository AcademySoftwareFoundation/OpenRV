//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __Mu__ExecutionObserver__h__
#define __Mu__ExecutionObserver__h__

namespace Mu
{
    class Function;
    class Node;
    class Thread;

    //
    //  ExecutionObserver
    //
    //  A lightweight, optional hook into interpreted Mu function execution.
    //  The interpreter calls onEnter()/onExit() around every interpreted
    //  function activation (and at the external Thread::call() boundary).
    //
    //  This exists so higher layers can observe "where Mu is executing now"
    //  without the Mu core itself depending on those layers. The crash-reporting
    //  implementation lives in MuTwkApp (see MuCrashObserver), which registers
    //  itself here. The Mu core deliberately has no dependency on TwkUtil/Qt.
    //
    //  By default no observer is installed and the per-call cost is a single
    //  null-pointer check.
    //

    struct ExecutionObserver
    {
        virtual ~ExecutionObserver() {}

        // Called when an interpreted function begins executing. callNode is the
        // call-site node (an AnnotatedNode carrying file/line when debugging is
        // enabled) or null when entering from the external Thread::call() path.
        virtual void onEnter(const Function* f, const Node* callNode, Thread& t) = 0;

        // Called when that function finishes (normal return, Mu/C++ exception,
        // or tail-call unwind). Observers maintain their own state to know what
        // to restore.
        virtual void onExit(Thread& t) = 0;
    };

    // The single installed observer, or null. Set by the app layer at init.
    extern ExecutionObserver* executionObserver;

    //
    //  ActivationScope
    //
    //  RAII guard placed in each function-activation site. Invokes the observer
    //  on construction/destruction. Safe with respect to Mu's setjmp/longjmp
    //  return mechanism (the return longjmp lands within the same frame, after
    //  this guard is constructed, so the destructor still runs on fall-through)
    //  and with respect to C++ exceptions (Mu exceptions are C++ throws, which
    //  run destructors).
    //

    struct ActivationScope
    {
        Thread& thread;
        bool active;

        ActivationScope(const Function* f, const Node* callNode, Thread& t)
            : thread(t)
            , active(executionObserver != nullptr)
        {
            if (active)
                executionObserver->onEnter(f, callNode, t);
        }

        ~ActivationScope()
        {
            if (active)
                executionObserver->onExit(thread);
        }
    };

} // namespace Mu

#endif // __Mu__ExecutionObserver__h__
