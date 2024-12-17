//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Object.h>
#include <Mu/Type.h>
#include <Mu/MuProcess.h>
#include <Mu/GarbageCollector.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    Object::ExternalHeap Object::_externalHeap;

    Object::Object(const Type* type) { _type = type; }

    Object::Object() { _type = 0; }

    Object::~Object() { _type = (const Type*)MU_DELETED; }

    void Object::typeDelete() { type()->deleteObject(this); }

    void Object::retainExternal() { _externalHeap[this]++; }

    void Object::releaseExternal()
    {
        _externalHeap[this]--;
        if (_externalHeap[this] <= 0)
            _externalHeap.erase(this);
    }

} // namespace Mu
