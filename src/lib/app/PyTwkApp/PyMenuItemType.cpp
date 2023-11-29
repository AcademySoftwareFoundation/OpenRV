//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//

#include <TwkPython/PyLockObject.h>
#include <Python.h>

#include <PyTwkApp/PyMenuItemType.h>
#include <PyTwkApp/structmember.h>
#include <MuPy/PyModule.h>

namespace TwkApp
{
  using namespace std;

  static PyObject* MenuItem_new( PyTypeObject* type, PyObject* args,
                                 PyObject* kwds )
  {
    PyLockObject locker;
    PyMenuItemObject* self;

    if( self = (PyMenuItemObject*)type->tp_alloc( type, 0 ) )
    {
      self->label = 0;
      self->actionHook = 0;
      self->key = 0;
      self->stateHook = 0;
      self->subMenu = 0;
    }

    return (PyObject*)self;
  }

  int MenuItem_init( PyObject* _self, PyObject* args, PyObject* kwds )
  {
    PyMenuItemObject* self = (PyMenuItemObject*) _self;

    PyLockObject locker;
    const char* label;
    PyObject* action;
    const char* key;
    PyObject* state;
    PyObject* subMenu;

    int ok = PyArg_ParseTuple( args, "sOsOO", &label, &action, &key, &state,
                               &subMenu );

    if( !ok ) return -1;

    Py_XINCREF( action );
    Py_XINCREF( state );
    Py_XINCREF( subMenu );

    self->label = PyBytes_FromString( label );
    self->actionHook = action;
    self->key = PyBytes_FromString( key );
    self->stateHook = state;
    self->subMenu = subMenu;
    return 0;
  }

  static void MenuItem_dealloc( PyObject* _self )
  {
    PyMenuItemObject* self = (PyMenuItemObject*) _self;

    PyLockObject locker;

    Py_XDECREF( self->label );
    Py_XDECREF( self->actionHook );
    Py_XDECREF( self->key );
    Py_XDECREF( self->stateHook );
    Py_XDECREF( self->subMenu );

    Py_TYPE(self)->tp_free( self );
  }

  static PyMethodDef methods[] = {
      { NULL } /* Sentinel */
  };

  static PyMemberDef members[] = {
      { "label", T_OBJECT_EX, offsetof( PyMenuItemObject, label ), 0,
        "User visible label" },
      { "actionHook", T_OBJECT_EX, offsetof( PyMenuItemObject, actionHook ), 0,
        "Activation callback" },
      { "key", T_OBJECT_EX, offsetof( PyMenuItemObject, actionHook ), 0,
        "Accelerator key" },
      { "stateHook", T_OBJECT_EX, offsetof( PyMenuItemObject, stateHook ), 0,
        "State callback" },
      { "subMenu", T_OBJECT_EX, offsetof( PyMenuItemObject, subMenu ), 0,
        "Sub-menu" },
      { NULL } /* Sentinel */
  };

  static PyTypeObject type = {
      PyVarObject_HEAD_INIT( NULL, 0 )
      "MenuItem",                   /*tp_name*/
      sizeof( PyMenuItemObject ),   /*tp_basicsize*/
      0,                            /*tp_itemsize*/
      MenuItem_dealloc,             /*tp_dealloc*/
      0,                            /*tp_print*/
      0,                            /*tp_getattr*/
      0,                            /*tp_setattr*/
      0,                            /*tp_compare*/
      0,                            /*tp_repr*/
      0,                            /*tp_as_number*/
      0,                            /*tp_as_sequence*/
      0,                            /*tp_as_mapping*/
      0,                            /*tp_hash */
      0,                            /*tp_call*/
      0,                            /*tp_str*/
      0,                            /*tp_getattro*/
      0,                            /*tp_setattro*/
      0,                            /*tp_as_buffer*/
      Py_TPFLAGS_DEFAULT,           /*tp_flags*/
      "Application Menu Item",      /* tp_doc */
      0,                            /* tp_traverse */
      0,                            /* tp_clear */
      0,                            /* tp_richcompare */
      0,                            /* tp_weaklistoffset */
      0,                            /* tp_iter */
      0,                            /* tp_iternext */
      methods,                      /* tp_methods */
      0,                            /* tp_members */
      0,                            /* tp_getset */
      0,                            /* tp_base */
      0,                            /* tp_dict */
      0,                            /* tp_descr_get */
      0,                            /* tp_descr_set */
      0,                            /* tp_dictoffset */
      MenuItem_init,                /* tp_init */
      0,                            /* tp_alloc */
      MenuItem_new,                 /* tp_new */
  };

  PyTypeObject* pyMenuItemType()
  {
    return &type;
  }

  void initPyMenuItemType()
  {
    //
  }

}  // namespace TwkApp
