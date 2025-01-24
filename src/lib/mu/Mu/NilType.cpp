//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/NilType.h>
#include <iostream>
#include <Mu/Value.h>
#include <Mu/Function.h>
#include <Mu/Node.h>
#include <Mu/MachineRep.h>

namespace Mu
{
    using namespace std;

    NilType::NilType(Context* context)
        : Class(context, "nil")
    {
        //
        //  NilType is a nebulous class because it can be cast to any
        //  other type. (So far, its the only nebulous class).
        //

        _nebulousAncestry = true;
    }

    NilType::~NilType() {}

    bool NilType::nebulousIsA(const Class*) const { return true; }

    Object* NilType::newObject() const { return 0; }

    Value NilType::nodeEval(const Node* n, Thread& thread) const
    {
        (*n->func()._PointerFunc)(*n, thread);
        return Value(Pointer(0));
    }

    void NilType::nodeEval(void*, const Node* n, Thread& thread) const
    {
        (*n->func()._PointerFunc)(*n, thread);
    }

    void NilType::outputValue(ostream& o, const Value& value, bool) const
    {
        o << "nil";
    }

    void NilType::outputValueRecursive(std::ostream& o, const ValuePointer,
                                       ValueOutputState&) const
    {
        o << "nil";
    }

    void NilType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        scope()->addSymbols(new Function(context(), "nil",
                                         machineRep()->constantFunc(), Mapped,
                                         Return, "nil", End),

                            EndArguments);
    }

} // namespace Mu
