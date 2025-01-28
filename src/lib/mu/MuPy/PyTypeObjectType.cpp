//
// Copyright (c) 2011, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuPy/PyTypeObjectType.h>

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

    PyTypeObjectType::PyTypeObjectType(Context* c, const char* name)
        : OpaqueType(c, name)
    {
    }

    PyTypeObjectType::~PyTypeObjectType() {}

    void PyTypeObjectType::outputValue(ostream& o, const Value& value,
                                       bool full) const
    {
        o << "<#" << fullyQualifiedName() << " ";

        if (value._Pointer)
        {
            PyTypeObject* t = reinterpret_cast<PyTypeObject*>(value._Pointer);
            o << t->tp_name;
        }
        else
        {
            o << "nil";
        }

        o << ">";
    }

    void PyTypeObjectType::outputValueRecursive(ostream& o,
                                                const ValuePointer vp,
                                                ValueOutputState& state) const
    {
        Pointer p = *reinterpret_cast<Pointer*>(vp);

        o << "<#" << fullyQualifiedName() << " ";

        if (p)
        {
            PyTypeObject* t = reinterpret_cast<PyTypeObject*>(p);
            o << t->tp_name;
        }
        else
        {
            o << "nil";
        }

        o << ">";
    }

    void PyTypeObjectType::load()
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

        globalScope()->addSymbols(

            EndArguments);
    }

} // namespace Mu
