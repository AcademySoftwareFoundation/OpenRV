//
// Copyright (c) 2011, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuPy/PyObjectType.h>

#include <Python.h>

#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Value.h>
#include <Mu/config.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    PyObjectType::PyObjectType(Context* c, const char* name)
        : OpaqueType(c, name)
    {
    }

    PyObjectType::~PyObjectType() {}

    void PyObjectType::outputValue(ostream& o, const Value& value,
                                   bool full) const
    {
        o << "<#" << fullyQualifiedName() << " ";

        if (value._Pointer)
        {
            PyObject* pyobj = reinterpret_cast<PyObject*>(value._Pointer);

            PyTypeObject* t = Py_TYPE(pyobj);
            o << t->tp_name;
        }
        else
        {
            o << "nil";
        }

        o << " " << value._Pointer;
        o << ">";
    }

    void PyObjectType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                            ValueOutputState& state) const
    {
        Pointer p = *reinterpret_cast<Pointer*>(vp);

        o << "<#" << fullyQualifiedName() << " ";

        if (p)
        {
            PyObject* pyobj = reinterpret_cast<PyObject*>(p);

            PyTypeObject* t = Py_TYPE(pyobj);
            o << t->tp_name;
        }
        else
        {
            o << "nil";
        }

        o << " " << *(Pointer*)vp;
        o << ">";
    }

    void PyObjectType::load()
    {
        OpaqueType::load();

        USING_MU_FUNCTION_SYMBOLS;
        Symbol* s = scope();

        string rname = name();
        rname += "&";
        const char* n = fullyQualifiedName().c_str();
        string nref = n;
        nref += "&";
        const char* nr = nref.c_str();

        Context* c = context();

#if 0
    addSymbol( new Function(c, "()", callPyObject, Function::None,
                            Return, "python.PyObject",
                            Args, "pythion.PyObject"
                            End) );
#endif
    }

} // namespace Mu
