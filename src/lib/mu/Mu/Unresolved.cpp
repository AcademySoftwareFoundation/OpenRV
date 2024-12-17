//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Unresolved.h>
#include <Mu/Module.h>
#include <Mu/Context.h>
#include <Mu/MachineRep.h>

namespace Mu
{
    using namespace std;

    UnresolvedSymbol::UnresolvedSymbol(Context* context, const char* n)
        : Symbol(context, n)
    {
        _datanode = true;
    }

    UnresolvedSymbol::~UnresolvedSymbol() {}

    const Type* UnresolvedSymbol::nodeReturnType(const Node*) const
    {
        const Context* c = globalModule()->context();
        return c->unresolvedType();
    }

    UnresolvedType::UnresolvedType(Context* context)
        : Type(context, "type*", VoidRep::rep())
    {
        _isUnresolvedType = 1;
    }

    Object* UnresolvedType::newObject() const { return 0; }

    void UnresolvedType::outputValue(std::ostream& o, Value& value) const
    {
        o << "unresolved value?";
    }

    Value UnresolvedType::nodeEval(const Node* n, Thread& t) const
    {
        //
        //  An exception *should* be thrown by whatever function is called
        //  here.
        //

        (*n->func()._voidFunc)(*n, t);
        return Value();
    }

    void UnresolvedType::nodeEval(void*, const Node*, Thread& t) const
    {
        return;
    }

    UnresolvedCall::UnresolvedCall(Context* context)
        : UnresolvedSymbol(context, "call*")
    {
    }

    UnresolvedCall::~UnresolvedCall() {}

    UnresolvedCast::UnresolvedCast(Context* context)
        : UnresolvedSymbol(context, "cast*")
    {
    }

    UnresolvedCast::~UnresolvedCast() {}

    UnresolvedConstructor::UnresolvedConstructor(Context* context)
        : UnresolvedSymbol(context, "constructor*")
    {
    }

    UnresolvedConstructor::~UnresolvedConstructor() {}

    UnresolvedReference::UnresolvedReference(Context* context)
        : UnresolvedSymbol(context, "ref*")
    {
    }

    UnresolvedReference::~UnresolvedReference() {}

    UnresolvedDereference::UnresolvedDereference(Context* context)
        : UnresolvedSymbol(context, "deref*")
    {
    }

    UnresolvedDereference::~UnresolvedDereference() {}

    UnresolvedMemberReference::UnresolvedMemberReference(Context* context)
        : UnresolvedSymbol(context, "member_ref*")
    {
    }

    UnresolvedMemberReference::~UnresolvedMemberReference() {}

    UnresolvedMemberCall::UnresolvedMemberCall(Context* context)
        : UnresolvedSymbol(context, "member_call*")
    {
    }

    UnresolvedMemberCall::~UnresolvedMemberCall() {}

    UnresolvedStackReference::UnresolvedStackReference(Context* context)
        : UnresolvedSymbol(context, "stack_ref*")
    {
    }

    UnresolvedStackReference::~UnresolvedStackReference() {}

    UnresolvedStackDereference::UnresolvedStackDereference(Context* context)
        : UnresolvedSymbol(context, "stack_deref*")
    {
    }

    UnresolvedStackDereference::~UnresolvedStackDereference() {}

    UnresolvedDeclaration::UnresolvedDeclaration(Context* context)
        : UnresolvedSymbol(context, "declaration*")
    {
    }

    UnresolvedDeclaration::~UnresolvedDeclaration() {}

    UnresolvedAssignment::UnresolvedAssignment(Context* context)
        : UnresolvedSymbol(context, "assign*")
    {
    }

    UnresolvedAssignment::~UnresolvedAssignment() {}

} // namespace Mu
