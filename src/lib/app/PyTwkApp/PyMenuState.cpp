//
// Copyright (c) 2011 Tweak Inc.
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//

#include <PyTwkApp/PyMenuState.h>
#include <PyTwkApp/PyInterface.h>

#include <MuPy/PyModule.h>

#include <TwkPython/PyLockObject.h>
#include <Python.h>

#include <iostream>

namespace TwkApp
{
  using namespace std;

  PyStateFunc::PyStateFunc( PyObject* obj )
      : m_func( obj ), m_exception( false )
  {
    PyLockObject locker;
    if( obj ) Py_XINCREF( obj );
  }

  PyStateFunc::~PyStateFunc()
  {
    PyLockObject locker;
    if( m_func ) Py_XDECREF( m_func );
  }

  int PyStateFunc::state()
  {
    PyLockObject locker;
    int val = 1;

    if( m_func )
    {
      PyObject* args = Py_BuildValue( "()" );
      PyObject* r = PyObject_Call( m_func, args, 0 );
      Py_XDECREF( args );
      if( PyErr_Occurred() )
      {
        PyErr_Print();
        m_exception = true;
      }
      else
        val = PyLong_AsLong( r );
    }

    return val;
  }

  bool PyStateFunc::error() const
  {
    return m_exception;
  }

  Menu::StateFunc* PyStateFunc::copy() const
  {
    return new PyStateFunc( m_func );
  }

}  // namespace TwkApp
