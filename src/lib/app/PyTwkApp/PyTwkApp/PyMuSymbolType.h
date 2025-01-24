//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __PyTwkApp__PyMuSymbol__h__
#define __PyTwkApp__PyMuSymbol__h__

#include <Python.h>

#include <iostream>

namespace Mu
{
    class Symbol;
    class Function;
} // namespace Mu

namespace TwkApp
{
    class Event;
    class Document;

    typedef struct
    {
        PyObject_HEAD const Mu::Symbol* symbol;
        const Mu::Function* function;
    } PyMuSymbolObject;

    PyTypeObject* pyMuSymbolType();

    void initPyMuSymbolType();

} // namespace TwkApp

#endif // __PyTwkApp__PyMuSymbol__h__
