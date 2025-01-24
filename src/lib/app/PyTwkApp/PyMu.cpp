//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Signature.h>
#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <Mu/TypeModifier.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantType.h>
#include <MuLang/MuLangContext.h>
#include <MuPy/PyModule.h>
#include <MuTwkApp/MuInterface.h>
#include <TwkPython/PyLockObject.h>
#include <Python.h>
#include <sstream>

namespace TwkApp
{
    using namespace std;

    static Mu::Value pyToMu(PyObject* obj) { return Mu::Value(); }

    /* 'self' is not used */
    static PyObject* mu_eval(PyObject* self, PyObject* args)
    {
        PyLockObject locker;
        const char* s;
        if (!PyArg_ParseTuple(args, "s", &s))
            return NULL;
        Mu::Context::ModuleList modules;
        string r = muEval(muContext(), muProcess(), modules, s, "muEval");
        return Py_BuildValue("s", r.c_str());
    }

    static PyMethodDef pymu_methods[] = {
        {"muEval", mu_eval, METH_VARARGS,
         "evaluate mu code -- to be used a fallback only"},
        {NULL, NULL} /* sentinel */
    };

    static struct PyModuleDef moduledef = {
        {
            1,    /* ob_refcnt */
            NULL, /* ob_type */
            NULL, /* m_init */
            0,    /* m_index */
            NULL, /* m_copy */
        },
        "pymu",       /* m_name */
        NULL,         /* m_doc */
        -1,           /* m_size */
        pymu_methods, /* m_methods */
        NULL,         /* m_reload */
        NULL,         /* m_traverse */
        NULL,         /* m_clear */
        NULL,         /* m_free */
    };

    PyObject* initPyMu()
    {
        PyLockObject locker;
        PyObject* pymu = PyImport_AddModule("pymu");

        PyModule_Create(&moduledef);

        return pymu;
    }

} // namespace TwkApp
