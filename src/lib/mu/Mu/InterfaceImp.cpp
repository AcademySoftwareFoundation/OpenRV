//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Interface.h>
#include <Mu/InterfaceImp.h>

namespace Mu
{
    using namespace std;

    InterfaceImp::InterfaceImp(const Class* c, const Interface* i)
        : _class(c)
        , _interface(i)
        , _vtable(i->numFunctions())
    {
    }

    InterfaceImp::~InterfaceImp() {}

} // namespace Mu
