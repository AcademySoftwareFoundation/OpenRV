#ifndef __Mu__PyModule__
#define __Mu__PyModule__
//
// Copyright (c) 2011, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Module.h>
#include <Mu/Node.h>
#include <Mu/Value.h>

#include <Python.h>

namespace Mu
{

    class MuLangContext;
    class Type;
    class ClassInstance;

    class PyModule : public Module
    {
    public:
        PyModule(Context* c, const char* name);
        virtual ~PyModule();

        virtual void load();

        typedef void* (*MuToPyObjectConverter)(const Type*, Mu::ClassInstance*);
        typedef ClassInstance* (*PyObjectToMuConverter)(const Type*, void*);
        typedef void* (*MuOpaqueToPyObjectConverter)(const Type*, void*);
        typedef Pointer (*PyObjectToMuOpaqueConverter)(const Type*, void*);

        //
        //  NOTE: any of these can be 0 if no function is needed
        //

        static void addConverterFunctions(MuToPyObjectConverter,
                                          PyObjectToMuConverter,
                                          MuOpaqueToPyObjectConverter,
                                          PyObjectToMuOpaqueConverter);

        static Value py2mu(MuLangContext*, Process*, const Type*, PyObject*);
        static PyObject* mu2py(MuLangContext*, Process*, const Type*,
                               const Value&);

        static NODE_DECLARATION(nPy_DECREF, void);
        static NODE_DECLARATION(nPy_INCREF, void);
        static NODE_DECLARATION(nPyErr_Print, void);
        static NODE_DECLARATION(nPyObject_GetAttr, Pointer);
        static NODE_DECLARATION(nPyObject_CallObject, Pointer);
        static NODE_DECLARATION(nPyObject_CallObject2, Pointer);
        static NODE_DECLARATION(nPyTuple_New, Pointer);
        static NODE_DECLARATION(nPyTuple_Size, int);
        static NODE_DECLARATION(nPyTuple_SetItem, int);
        static NODE_DECLARATION(nPyTuple_GetItem, Pointer);
        static NODE_DECLARATION(nPyString_Check, bool);
        static NODE_DECLARATION(nPyFunction_Check, bool);
        static NODE_DECLARATION(nPyImport_Import, Pointer);
        static NODE_DECLARATION(nPy_TYPE, Pointer);
        static NODE_DECLARATION(nPyModule_GetName, Pointer);
        static NODE_DECLARATION(nPyModule_GetFilename, Pointer);
        static NODE_DECLARATION(is_nil, bool);
        static NODE_DECLARATION(type_name, Pointer);

        static NODE_DECLARATION(classInstanceToPyObject, Pointer);
        static NODE_DECLARATION(PyObjectToString, Pointer);
        static NODE_DECLARATION(PyObjectToBool, bool);
        static NODE_DECLARATION(PyObjectToInt, int);
        static NODE_DECLARATION(PyObjectToFloat, float);
    };

} // namespace Mu

#endif
