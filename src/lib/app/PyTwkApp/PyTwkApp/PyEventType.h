//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __PyTwkApp__PyEventType__h__
#define __PyTwkApp__PyEventType__h__
#include <Python.h>
#include <iostream>

namespace TwkApp
{
    class Event;
    class Document;

    typedef struct
    {
        PyObject_HEAD const Event* event;
        const Document* document;
    } PyEventObject;

    PyTypeObject* pyEventType();

    void initPyEventType();

    //
    // Create a new event object. The new object has 1 ref on it
    //
    PyObject* PyEventFromEvent(const Event*, const Document*);

} // namespace TwkApp

#endif // __PyTwkApp__PyEventType__h__
