//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolTable.h>
#include <Mu/VariantInstance.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantType.h>
#include <algorithm>
#include <typeinfo>

namespace Mu
{
    using namespace std;

    VariantType::VariantType(Context* context, const char* name)
        : Type(context, name, PointerRep::rep())
        , _numTags(0)
    {
        _isPrimitive = false;
    }

    VariantType::~VariantType() {}

    Type::MatchResult VariantType::match(const Type* type, Bindings& b) const
    {
        if (this == type
            || (dynamic_cast<const VariantTagType*>(type)
                && type->scope() == this))
        {
            return Match;
        }

        return Type::match(type, b);
    }

    Object* VariantType::newObject() const { return 0; }

    size_t VariantType::objectSize() const { return 0; }

    void VariantType::constructInstance(Pointer) const
    {
        // what to do here?
    }

    void VariantType::copyInstance(Pointer src, Pointer dst) const
    {
        reinterpret_cast<VariantInstance*>(src)->tagType()->copyInstance(src,
                                                                         dst);
    }

    Value VariantType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void VariantType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* pp = reinterpret_cast<Pointer*>(p);
        *pp = (*n->func()._PointerFunc)(*n, thread);
    }

    void VariantType::outputValue(ostream& o, const Value& value,
                                  bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, ValuePointer(&value._Pointer), state);
    }

    void VariantType::outputValueRecursive(ostream& o, const ValuePointer p,
                                           ValueOutputState& state) const
    {
        if (p)
        {
            const VariantInstance* i =
                *reinterpret_cast<const VariantInstance**>(p);
            if (i)
                i->tagType()->outputValueRecursive(o, p, state);
            else
                o << "nil";
        }
        else
        {
            o << "nil";
        }
    }

    void VariantType::addSymbol(Symbol* s)
    {
        if (VariantTagType* tt = dynamic_cast<VariantTagType*>(s))
        {
            tt->_index = _numTags++;
        }

        Type::addSymbol(s);
    }

    void VariantType::load() { Type::load(); }

} // namespace Mu
