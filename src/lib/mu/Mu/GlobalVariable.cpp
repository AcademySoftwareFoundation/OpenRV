//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Type.h>
#include <Mu/MachineRep.h>
#include <Mu/GlobalVariable.h>
#include <Mu/ReferenceType.h>
#include <iostream>

namespace Mu
{

    using namespace std;

    GlobalVariable::GlobalVariable(Context* context, const char* name,
                                   const Type* storageClass, int offset,
                                   Variable::Attributes a, Node* initializer)

        : Variable(context, name, storageClass, offset, a)
        , _initializer(initializer)
    {
    }

    GlobalVariable::~GlobalVariable() {}

    const Type* GlobalVariable::nodeReturnType(const Node* n) const
    {
        const MachineRep* rep = storageClass()->machineRep();

        if (n->func() == rep->referenceGlobalFunc())
        {
            return storageClass()->referenceType();
        }
        else
        {
            return storageClass();
        }
    }

    void GlobalVariable::output(ostream& o) const
    {
        Variable::output(o);
        o << " (global)";
    }

    void GlobalVariable::outputNode(std::ostream& o, const Node* n) const
    {
        o << n->type()->name() << " global " << name();
    }

} // namespace Mu
