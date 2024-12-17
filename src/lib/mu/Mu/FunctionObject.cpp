//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/Function.h>

namespace Mu
{
    using namespace std;
    using namespace stl_ext;

    FunctionObject::FunctionObject()
        : ClassInstance()
        , _function(0)
    {
    }

    FunctionObject::FunctionObject(const FunctionType* type)
        : ClassInstance(type)
        , _function(0)
        , _dependent(0)
    {
    }

    FunctionObject::FunctionObject(const Function* f)
        : ClassInstance(f->type())
        , _function(f)
        , _dependent(0)
    {
    }

    FunctionObject::FunctionObject(Thread& t, const char* className)
        : ClassInstance(t, className)
        , _function(0)
        , _dependent(0)
    {
    }

    FunctionObject::~FunctionObject()
    {
        // if (_function->isLambda())
        // {
        //     delete _function;
        // }

        _function = (Function*)MU_DELETED;
        _dependent = (FunctionObject*)MU_DELETED;
    }

} // namespace Mu
