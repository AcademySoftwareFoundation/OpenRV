#ifndef __MuLinear__MathLinearModule__h__
#define __MuLinear__MathLinearModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/Module.h>

namespace Mu
{

    class MathLinearModule : public Module
    {
    public:
        MathLinearModule(Context* c, const char* name);
        virtual ~MathLinearModule();

        virtual void load();
    };

} // namespace Mu

#endif // __MuLinear__MathLinearModule__h__
