//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#include <TwkPython/PyLockObject.h>
#include <Python.h>

// Ensure that the current thread is ready to call the Python C API
// regardless of the current state of Python, or of the global interpreter lock.
PyLockObject::PyLockObject()
    : _gilState(PyGILState_Ensure())
{
}

// Release any resources previously acquired. After this call,
// Python’s state will be the same as it was prior to the corresponding
// PyGILState_Ensure() call (Done while initializing _gilState).
PyLockObject::~PyLockObject() { PyGILState_Release(_gilState); }
