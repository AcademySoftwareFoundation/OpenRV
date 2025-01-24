//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/SymbolType.h>
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

    SymbolType::SymbolType(Context* c, const char* name)
        : OpaqueType(c, name)
    {
    }

    SymbolType::~SymbolType() {}

    void SymbolType::outputValue(ostream& o, const Value& value,
                                 bool full) const
    {
        o << "<#" << fullyQualifiedName() << " ";

        if (value._Pointer)
        {
            const Symbol* s = reinterpret_cast<const Symbol*>(value._Pointer);
            o << s->fullyQualifiedName() << " " << s;
        }
        else
        {
            o << "nil";
        }

        o << ">";
    }

    void SymbolType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                          ValueOutputState& state) const
    {
        Pointer p = *reinterpret_cast<Pointer*>(vp);

        o << "<#" << fullyQualifiedName() << " ";

        if (p)
        {
            const Symbol* s = reinterpret_cast<const Symbol*>(p);
            o << s->fullyQualifiedName() << " " << s;
        }
        else
        {
            o << "nil";
        }

        o << ">";
    }

    void SymbolType::load()
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

    //----------------------------------------------------------------------

    TypeSymbolType::TypeSymbolType(Context* c, const char* name)
        : SymbolType(c, name)
    {
    }

    TypeSymbolType::~TypeSymbolType() {}

    //----------------------------------------------------------------------

    FunctionSymbolType::FunctionSymbolType(Context* c, const char* name)
        : SymbolType(c, name)
    {
    }

    FunctionSymbolType::~FunctionSymbolType() {}

    //----------------------------------------------------------------------

    VariableSymbolType::VariableSymbolType(Context* c, const char* name)
        : SymbolType(c, name)
    {
    }

    VariableSymbolType::~VariableSymbolType() {}

    //----------------------------------------------------------------------

    ParameterSymbolType::ParameterSymbolType(Context* c, const char* name)
        : SymbolType(c, name)
    {
    }

    ParameterSymbolType::~ParameterSymbolType() {}

} // namespace Mu
