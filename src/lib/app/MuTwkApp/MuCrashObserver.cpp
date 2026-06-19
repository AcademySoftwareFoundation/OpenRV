//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuTwkApp/MuCrashObserver.h>
#include <Mu/Function.h>
#include <Mu/Module.h>
#include <TwkUtil/CrashHandler.h>

namespace TwkApp
{

    MuCrashObserver& MuCrashObserver::instance()
    {
        static MuCrashObserver theObserver;
        return theObserver;
    }

    MuCrashObserver::MuCrashObserver()
        : m_lastWrittenFn(nullptr)
        , m_mainThread(pthread_self()) // constructed during install on the main thread
    {
        m_stack.reserve(64);
    }

    bool MuCrashObserver::active() const
    {
        return pthread_equal(pthread_self(), m_mainThread) && TwkUtil::CrashHandler::instance().isInitialized();
    }

    void MuCrashObserver::onEnter(const Mu::Function* f, const Mu::Node* /*callNode*/, Mu::Thread& /*t*/)
    {
        if (!active())
            return;

        // Function-level only: we deliberately never inspect the call node.
        // Mu's Node hierarchy is non-polymorphic and several activation paths
        // pass plain Nodes (not AnnotatedNodes), so there is no safe way to read
        // a source line here. The function and its file are all we record.
        m_stack.push_back(f);
        writeTop();
    }

    void MuCrashObserver::onExit(Mu::Thread& /*t*/)
    {
        if (!active())
            return;

        if (!m_stack.empty())
            m_stack.pop_back();
        writeTop();
    }

    void MuCrashObserver::writeTop()
    {
        TwkUtil::CrashHandler& crashHandler = TwkUtil::CrashHandler::instance();

        const Mu::Function* top = m_stack.empty() ? nullptr : m_stack.back();

        // The (relatively) expensive name/file lookup only happens when the
        // active function actually changes; the common case (loops, recursion
        // into the same function) is a single pointer compare.
        if (top == m_lastWrittenFn)
            return;

        if (top)
        {
            crashHandler.addAnnotation("mu_function", top->fullyQualifiedName().c_str());

            const char* file = "";
            if (const Mu::Module* m = top->globalModule())
                file = m->location().c_str();
            crashHandler.addAnnotation("mu_script_file", file);
        }
        else
        {
            crashHandler.addAnnotation("mu_function", "");
            crashHandler.addAnnotation("mu_script_file", "");
        }

        m_lastWrittenFn = top;
    }

    void installMuCrashObserver() { Mu::executionObserver = &MuCrashObserver::instance(); }

} // namespace TwkApp
