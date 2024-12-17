//
// Copyright (c) 2011 Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __PyTwkApp__MenuState__h__
#define __PyTwkApp__MenuState__h__
#include <TwkApp/Menu.h>
#include <Python.h>

namespace TwkApp
{
    class PyStateFunc : public Menu::StateFunc
    {
    public:
        PyStateFunc(PyObject*);
        virtual ~PyStateFunc();
        virtual int state();
        virtual Menu::StateFunc* copy() const;
        virtual bool error() const;

        bool exceptionOccuredLastTime() const { return m_exception; }

    private:
        PyObject* m_func;
        bool m_exception;
    };

} // namespace TwkApp

#endif // __PyTwkApp__MenuState__h__
