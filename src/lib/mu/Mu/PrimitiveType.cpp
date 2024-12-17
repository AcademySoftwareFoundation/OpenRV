//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/PrimitiveType.h>
#include <Mu/MachineRep.h>
#include <iostream>

namespace Mu
{

    using namespace std;

    PrimitiveType::PrimitiveType(Context* context, const char* tname,
                                 const MachineRep* rep)
        : Type(context, tname, rep)
    {
        _isPrimitive = true;
        _isGCAtomic = true;
    }

    PrimitiveType::~PrimitiveType() {}

    size_t PrimitiveType::objectSize() const { return machineRep()->size(); }

    void PrimitiveType::constructInstance(Pointer p) const
    {
        memset(p, 0, machineRep()->size());
    }

    void PrimitiveType::copyInstance(Pointer a, Pointer b) const
    {
        memcpy(b, a, machineRep()->size());
    }

} // namespace Mu
