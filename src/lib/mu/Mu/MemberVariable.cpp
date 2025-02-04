//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/MemberVariable.h>
#include <Mu/MachineRep.h>
#include <Mu/Type.h>
#include <Mu/ReferenceType.h>
#include <iostream>
#include <assert.h>

namespace Mu
{
    using namespace std;

    MemberVariable::MemberVariable(Context* context, const char* name,
                                   const Type* storageClass, int address,
                                   bool hidden, Variable::Attribute a)
        : Variable(context, name, storageClass, address, a)
        , _hidden(hidden)
    {
    }

    MemberVariable::MemberVariable(Context* context, const char* name,
                                   const char* storageClass, int address,
                                   bool hidden, Variable::Attribute a)
        : Variable(context, name, storageClass, address, a)
        , _hidden(hidden)
    {
    }

    String MemberVariable::mangledName() const
    {
        static const char* cppkeywords[] = {
            "this",    "switch", "default",   "auto",     "case",
            "return",  "throw",  "catch",     "template", "class",
            "private", "public", "protected", NULL};

        for (const char** k = cppkeywords; *k; k++)
        {
            if (name() == *k)
            {
                String o = "__";
                o += name().c_str();
                return o;
            }
        }

        return name();
    }

    const Type* MemberVariable::nodeReturnType(const Node* n) const
    {
        if (const Type* t = dynamic_cast<const Type*>(n->symbol()->scope()))
        {
            const MachineRep* rep = t->machineRep();

            if (n->func() == rep->referenceMemberFunc()
                || n->func() == rep->referenceClassMemberFunc())
            {
                return storageClass()->referenceType();
            }
            else
            {
                return storageClass();
            }
        }
        else
        {
            //
            //  How could this have happened? The MemberVariable is in the
            //  scope of something that is not a type. This is a program error.
            //

            assert(0);
        }

        return 0; // make xlc happy
    }

    void MemberVariable::output(ostream& o) const
    {
        Symbol::output(o);
        o << " (member)";
    }

    void MemberVariable::outputNode(std::ostream& o, const Node* n) const
    {
        o << n->type()->name() << " member " << name();
    }

    //----------------------------------------------------------------------

    InternalTypeMemberVariable::InternalTypeMemberVariable(Context* context,
                                                           const char* name,
                                                           const Class* value)
        : MemberVariable(context, name, "runtime.type_symbol", 0, true)
        , _value(value)
    {
    }

    //----------------------------------------------------------------------

    FunctionMemberVariable::FunctionMemberVariable(
        Context* context, const char* name, const char* storageClass,
        const Function* refFunction, const Function* extractFunction,
        int address, Attribute a)
        : MemberVariable(context, name, storageClass, address, a)
        , _refFunc(refFunction)
        , _extractFunc(extractFunction)
    {
    }

    FunctionMemberVariable::FunctionMemberVariable(
        Context* context, const char* name, const Type* storageClass,
        const Function* refFunction, const Function* extractFunction,
        int address, Attribute a)
        : MemberVariable(context, name, storageClass, address, a)
        , _refFunc(refFunction)
        , _extractFunc(extractFunction)
    {
    }

    const Function* FunctionMemberVariable::referenceFunction() const
    {
        return _refFunc;
    }

    const Function* FunctionMemberVariable::extractFunction() const
    {
        return _extractFunc;
    }

} // namespace Mu
