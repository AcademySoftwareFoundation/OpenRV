//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuTwkApp__MuCrashObserver__h__
#define __MuTwkApp__MuCrashObserver__h__

#include <Mu/ExecutionObserver.h>
#include <pthread.h>
#include <vector>

namespace Mu
{
    class Function;
    class Node;
    class Thread;
} // namespace Mu

namespace TwkApp
{
    //
    //  MuCrashObserver
    //
    //  Keeps the crash handler's mu_function / mu_script_file annotations up to
    //  date as Mu executes, so a crash dump reflects which Mu function (and the
    //  file that defines it) was running. This is the Mu counterpart of the
    //  Python PyEval_SetTrace hook used for py_* annotations.
    //
    //  Fidelity is function-level, not line-level: Mu's Node hierarchy is
    //  non-polymorphic, so there is no safe way to recover the source line of an
    //  arbitrary activation node (several activation paths pass plain Nodes, not
    //  AnnotatedNodes). We therefore track only the Function and derive the file
    //  from its module, never casting execution nodes. See docs/crash-reporting.md.
    //
    //  It is installed as Mu's ExecutionObserver (see Mu/ExecutionObserver.h) and
    //  is invoked around every interpreted function activation. To stay cheap and
    //  race-free it only mutates the (process-global) crash annotations on the
    //  main thread; per-call work is a pointer compare plus, when the function
    //  changes, one name lookup.
    //
    class MuCrashObserver : public Mu::ExecutionObserver
    {
    public:
        static MuCrashObserver& instance();

        void onEnter(const Mu::Function* f, const Mu::Node* callNode, Mu::Thread& t) override;
        void onExit(Mu::Thread& t) override;

    private:
        MuCrashObserver();

        bool active() const; // main thread and crash handler initialized
        void writeTop();     // push top-of-stack function into the crash annotations

        std::vector<const Mu::Function*> m_stack;
        const Mu::Function* m_lastWrittenFn;
        pthread_t m_mainThread;
    };

    // Install MuCrashObserver as Mu's execution observer. Idempotent; call once
    // during Mu init, on the main thread.
    void installMuCrashObserver();
} // namespace TwkApp

#endif // __MuTwkApp__MuCrashObserver__h__
