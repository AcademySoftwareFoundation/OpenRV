//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MuTwkApp/EventType.h>
#include <MuTwkApp/FunctionAction.h>
#include <MuTwkApp/MuInterface.h>
#include <Mu/FunctionType.h>
#include <MuLang/ExceptionType.h>
#include <Mu/Thread.h>
#include <TwkApp/Event.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <iostream>
#include <sstream>

namespace TwkApp
{
    using namespace std;

    MuFuncAction::MuFuncAction(Mu::FunctionObject* obj)
        : Action()
    {
        m_exception = false;
        m_func = obj;
        m_func->retainExternal();
    }

    MuFuncAction::MuFuncAction(Mu::FunctionObject* obj, const string& doc)
        : Action(doc)
    {
        m_func = obj;
        m_exception = false;
        m_func->retainExternal();
    }

    MuFuncAction::~MuFuncAction() { m_func->releaseExternal(); }

    void MuFuncAction::execute(Document* d, const Event& event) const
    {
        HOP_PROF_FUNC();

        const EventType* etype =
            static_cast<const EventType*>(m_func->function()->argType(0));
        Mu::Process* p = muProcess();

        EventType::EventInstance* e = new EventType::EventInstance(etype);
        e->event = &event;
        event.handled = true; // the user can call reject()
        e->document = d;

        Mu::Function::ArgumentVector args(1);
        args.back() = Mu::Value(e);

        Mu::FunctionObject* preservedFunc = m_func;
        ostringstream preservedFuncName;
        m_func->function()->output(preservedFuncName);

        muAppThread()->call(m_func->function(), args, false);

        if (muAppThread()->uncaughtException() && !m_exception)
        {
            cerr << "ERROR: event = " << event.name() << endl;

            if (m_func != preservedFunc || !m_func || !(m_func->function()))
            {
                cerr << "ERROR: event handler function object corrupted!"
                     << endl;
            }

            cerr << "ERROR: function = " << preservedFuncName.str() << endl;

            if (const Mu::Object* o = muAppThread()->exception())
            {
                cerr << "ERROR: Exception Value: ";
                o->type()->outputValue(cerr, (Mu::ValuePointer)&o);
                cerr << endl;

                if (o->type() == muContext()->exceptionType()
                    && muContext()->debugging())
                {
                    const Mu::ExceptionType::Exception* exc =
                        static_cast<const Mu::ExceptionType::Exception*>(o);

                    cerr << "Backtrace:" << endl
                         << exc->backtraceAsString() << endl;
                }
            }

            m_exception = true;
        }
        else
        {
            // m_exception = false;
        }
    }

    bool MuFuncAction::error() const { return m_exception; }

    Action* MuFuncAction::copy() const
    {
        return new MuFuncAction(m_func, docString());
    }

} // namespace TwkApp
