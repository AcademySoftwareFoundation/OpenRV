//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Function.h>
#include <Mu/ReferenceType.h>
#include <iostream>
#include <Mu/MachineRep.h>
#include <assert.h>

namespace Mu
{

    using namespace std;

    ReferenceType::ReferenceType(Context* context, const char* typeName,
                                 Type* type)
        : Type(context, typeName, PointerRep::rep())
    {
        assert(type->_referenceType == 0);
        type->_referenceType = this;

        _isRefType = true;
        _type = type;
    }

    Object* ReferenceType::newObject() const { return 0; }

    void ReferenceType::load()
    {
        // nothing
    }

    Value ReferenceType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void ReferenceType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* pp = reinterpret_cast<Pointer*>(p);
        *pp = (*n->func()._PointerFunc)(*n, thread);
    }

    void ReferenceType::outputValue(ostream& o, const Value& value,
                                    bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, ValuePointer(&value._Pointer), state);
    }

    void ReferenceType::outputValueRecursive(ostream& o, const ValuePointer p,
                                             ValueOutputState& state) const
    {
        const ValuePointer dp = *reinterpret_cast<ValuePointer*>(p);
        dereferenceType()->outputValueRecursive(o, dp, state);
    }

    Type::MatchResult ReferenceType::match(const Type* t, Bindings&) const
    {
        return this == t ? Match : NoMatch;
    }

} // namespace Mu
