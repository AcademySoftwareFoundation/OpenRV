//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Class.h>
#include <Mu/Context.h>
#include <Mu/FunctionType.h>
#include <Mu/Interface.h>
#include <Mu/MachineRep.h>
#include <Mu/Module.h>
#include <Mu/NilType.h>
#include <Mu/ReferenceType.h>
#include <Mu/TypePattern.h>
#include <Mu/TypeVariable.h>
#include <Mu/Value.h>
#include <iostream>
#include <string.h>

namespace Mu
{
    using namespace std;

    TypeVariable::TypeVariable(Context* context, const char* name)
        : Type(context, name, VoidRep::rep())
    {
        _isTypeVariable = true;
    }

    TypeVariable::~TypeVariable() {}

    Type::MatchResult TypeVariable::match(const Type* type,
                                          Bindings& bindings) const
    {
        Bindings::iterator i = bindings.find(this);

        if (i != bindings.end())
        {
            return (*i).second == type ? Match : Conflict;
        }
        else
        {
            bindings[this] = type;
            return Match;
        }
    }

    Object* TypeVariable::newObject() const { return 0; }

    void TypeVariable::outputValue(std::ostream& o, Value& value) const
    {
        o << "\"" << name() << "\" is a type variable";
    }

    Value TypeVariable::nodeEval(const Node* n, Thread& t) const
    {
        //
        //  An exception *should* be thrown by whatever function is called
        //  here.
        //

        (*n->func()._voidFunc)(*n, t);
        return Value();
    }

    void TypeVariable::nodeEval(void*, const Node*, Thread& t) const { return; }

    const Type* TypeVariable::nodeReturnType(const Node*) const
    {
        if (globalModule())
        {
            return globalModule()->context()->unresolvedType();
        }
        else
        {
            return this;
        }
    }

} // Namespace  Mu
