//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/VoidType.h>
#include <iostream>
#include <Mu/Value.h>
#include <Mu/Function.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/MachineRep.h>

namespace Mu
{
    using namespace std;

    VoidType::VoidType(Context* c)
        : PrimitiveType(c, "void", VoidRep::rep())
    {
    }

    VoidType::~VoidType() {}

    PrimitiveObject* VoidType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value VoidType::nodeEval(const Node* n, Thread& thread) const
    {
        (*n->func()._voidFunc)(*n, thread);
        return Value();
    }

    void VoidType::nodeEval(void*, const Node* n, Thread& thread) const
    {
        (*n->func()._voidFunc)(*n, thread);
    }

    void VoidType::constructInstance(Pointer) const
    {
        // nothing
    }

    void VoidType::outputValue(ostream& o, const Value& value, bool full) const
    {
        o << "void";
    }

    void VoidType::outputValueRecursive(std::ostream& o, const ValuePointer,
                                        ValueOutputState&) const
    {
        o << "void";
    }

    void VoidType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        s->addSymbols(new Function(c, "void", machineRep()->simpleBlockFunc(),
                                   Cast, Return, "void", Args, "?", End),
                      EndArguments);
    }

} // namespace Mu
