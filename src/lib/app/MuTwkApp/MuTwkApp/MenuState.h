//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MuTwkApp__MenuState__h__
#define __MuTwkApp__MenuState__h__
#include <TwkApp/Menu.h>
#include <Mu/FunctionObject.h>

namespace TwkApp
{

    class MuStateFunc : public Menu::StateFunc
    {
    public:
        MuStateFunc(Mu::FunctionObject*);
        virtual ~MuStateFunc();
        virtual int state();
        virtual Menu::StateFunc* copy() const;
        virtual bool error() const;

        bool exceptionOccuredLastTime() const { return m_exception; }

    private:
        Mu::FunctionObject* m_func;
        bool m_exception;
    };

} // namespace TwkApp

#endif // __MuTwkApp__MenuState__h__
