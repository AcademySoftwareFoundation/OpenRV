#ifndef __mu_test__BarInterface__h__
#define __mu_test__BarInterface__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Interface.h>

namespace Mu
{

    class BarInterface : public Interface
    {
    public:
        BarInterface(Context*);
        virtual ~BarInterface();

        virtual void load();

        static NODE_DECLARATION(bar, Pointer);
        static NODE_DECLARATION(foo, Pointer);
    };

} // namespace Mu

#endif // __mu_test__BarInterface__h__
