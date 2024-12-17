//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MuTwkApp__FunctionAction__h__
#define __MuTwkApp__FunctionAction__h__
#include <TwkApp/Action.h>
#include <Mu/FunctionObject.h>

namespace TwkApp
{

    //
    //  An action which holds a function object and executes on demand.
    //

    class MuFuncAction : public Action
    {
    public:
        MuFuncAction(Mu::FunctionObject*);
        MuFuncAction(Mu::FunctionObject*, const std::string& docstring);
        virtual ~MuFuncAction();
        virtual void execute(Document*, const Event&) const;
        virtual Action* copy() const;
        virtual bool error() const;

        Mu::FunctionObject* fobj() const { return m_func; }

    private:
        Mu::FunctionObject* m_func;
        mutable bool m_exception;
    };

} // namespace TwkApp

#endif // __MuTwkApp__FunctionAction__h__
