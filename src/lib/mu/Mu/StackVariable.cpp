//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Type.h>
#include <Mu/StackVariable.h>
#include <Mu/MachineRep.h>
#include <iostream>
#include <Mu/ReferenceType.h>

namespace Mu
{

    using namespace std;

    StackVariable::StackVariable(Context* context, const char* name,
                                 const Type* storageClass, int stackPos,
                                 Attributes a)
        : Variable(context, name, storageClass, stackPos, a)
    {
    }

    StackVariable::StackVariable(Context* context, const char* name,
                                 const char* storageClass, int stackPos,
                                 Attributes a)
        : Variable(context, name, storageClass, stackPos, a)
    {
    }

    StackVariable::~StackVariable() {}

    String StackVariable::mangledName() const
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

    const Type* StackVariable::nodeReturnType(const Node* n) const
    {
        const Type* t = storageClass();

        if (t->isTypePattern())
        {
            return t->nodeReturnType(n);
        }
        else
        {
            const MachineRep* rep = t->machineRep();

            if (n->func() && n->func() == rep->referenceStackFunc())
            {
                return t->referenceType();
            }
            else
            {
                return t;
            }
        }
    }

    void StackVariable::output(ostream& o) const
    {
        Variable::output(o);
        o << " (on stack)";
    }

    void StackVariable::outputNode(std::ostream& o, const Node* n) const
    {
        o << n->type()->name() << " stack " << name();
    }

} // namespace Mu
