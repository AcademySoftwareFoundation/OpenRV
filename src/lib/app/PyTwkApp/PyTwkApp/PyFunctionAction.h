//******************************************************************************
// Copyright (c) 2011 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __PyTwkApp__PyFunctionAction__h__
#define __PyTwkApp__PyFunctionAction__h__
#include <PyTwkApp/PyInterface.h>
#include <TwkApp/Action.h>
#include <Python.h>

namespace TwkApp
{
    //
    //  An action which holds a function object and executes on demand.
    //

    class PyFunctionAction : public Action
    {
    public:
        PyFunctionAction(PyObject*);
        PyFunctionAction(PyObject*, const std::string& docstring);
        virtual ~PyFunctionAction();
        virtual void execute(Document*, const Event&) const;
        virtual Action* copy() const;
        virtual bool error() const;

        PyObject* fobj() const { return m_func; }

    private:
        PyObject* m_func;
        mutable bool m_exception;
    };

} // namespace TwkApp

#endif // __PyTwkApp__PyFunctionAction__h__
