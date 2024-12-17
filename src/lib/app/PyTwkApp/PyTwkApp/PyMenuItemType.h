//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __PyTwkApp__PyMenuItem__h__
#define __PyTwkApp__PyMenuItem__h__
#include <iostream>

namespace TwkApp
{
    class Event;
    class Document;

    typedef struct
    {
        PyObject_HEAD PyObject* label;
        PyObject* actionHook;
        PyObject* key;
        PyObject* stateHook;
        PyObject* subMenu;
    } PyMenuItemObject;

    PyTypeObject* pyMenuItemType();

    void initPyMenuItemType();

} // namespace TwkApp

#endif // __PyTwkApp__PyMenuItem__h__
