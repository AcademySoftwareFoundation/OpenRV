//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <MuTwkApp/MenuState.h>
#include <MuTwkApp/MenuItem.h>
#include <Mu/FunctionObject.h>
#include <Mu/Function.h>
#include <MuLang/ExceptionType.h>
#include <Mu/Thread.h>
#include <MuTwkApp/MuInterface.h>
#include <iostream>

namespace TwkApp
{
    using namespace std;

    MuStateFunc::MuStateFunc(Mu::FunctionObject* obj)
        : m_func(obj)
        , m_exception(false)
    {
        m_func->retainExternal();
    }

    MuStateFunc::~MuStateFunc()
    {
        m_func->releaseExternal();
        m_func = 0;
    }

    int MuStateFunc::state()
    {
        assert(m_func);
        Mu::Process* p = muProcess();
        Mu::Function::ArgumentVector args;
        const Mu::Function* F = m_func->function();
        Value v = muAppThread()->call(F, args, false);

        if (muAppThread()->uncaughtException() && !m_exception)
        {
            cerr << "ERROR: while evaluating function: ";
            Value v(m_func);
            m_func->type()->outputValue(cerr, v);
            cerr << endl;

            if (const Mu::Object* e = muAppThread()->exception())
            {
                cerr << "ERROR: Exception Value: ";
                e->type()->outputValue(cerr, (ValuePointer)&e);
                cerr << endl;

                if (e->type() == muContext()->exceptionType()
                    && muContext()->debugging())
                {
                    const Mu::ExceptionType::Exception* exc =
                        static_cast<const Mu::ExceptionType::Exception*>(e);

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

        return v._int;
    }

    bool MuStateFunc::error() const { return m_exception; }

    Menu::StateFunc* MuStateFunc::copy() const
    {
        return new MuStateFunc(m_func);
    }

} // namespace TwkApp
