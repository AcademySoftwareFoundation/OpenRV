//******************************************************************************
// Copyright (c) 2011 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <PyTwkApp/PyFunctionAction.h>
#include <PyTwkApp/PyEventType.h>
#include <PyTwkApp/PyInterface.h>

#include <MuPy/PyModule.h>
#include <TwkApp/Event.h>
#include <TwkPython/PyLockObject.h>
#include <Python.h>
#include <iostream>
#include <sstream>

namespace TwkApp
{
    using namespace std;

    PyFunctionAction::PyFunctionAction(PyObject* obj)
        : Action()
    {
        PyLockObject locker;
        m_exception = false;
        m_func = obj;
        Py_XINCREF(m_func);
    }

    PyFunctionAction::PyFunctionAction(PyObject* obj, const string& doc)
        : Action(doc)
    {
        PyLockObject locker;
        m_func = obj;
        m_exception = false;
        Py_XINCREF(m_func);
    }

    PyFunctionAction::~PyFunctionAction()
    {
        PyLockObject locker;
        Py_XDECREF(m_func);
    }

    void PyFunctionAction::execute(Document* d, const Event& event) const
    {
        PyLockObject locker;
        PyObject* e = PyEventFromEvent(&event, d);
        event.handled = true; // the user can call reject()

        PyObject* args = Py_BuildValue("(O)", e);
        PyObject* r = 0;

        try
        {
            r = PyObject_Call(m_func, args, 0);
        }
        catch (std::exception& exc)
        {
            cout << "ERROR: " << exc.what()
                 << " -- while executing action: " << docString() << endl;
        }
        catch (...)
        {
            cout << "ERROR: uncaught exception "
                 << " -- while executing action: " << docString() << endl;
        }

        Py_XDECREF(e);
        Py_XDECREF(args);

        if (PyErr_Occurred())
        {
            PyErr_Print();
            m_exception = true;
        }
    }

    bool PyFunctionAction::error() const { return m_exception; }

    Action* PyFunctionAction::copy() const
    {
        return new PyFunctionAction(m_func, docString());
    }

} // namespace TwkApp
