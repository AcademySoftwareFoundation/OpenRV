#ifndef __runtime__TestModule__h__
#define __runtime__TestModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Module.h>
#include <Mu/Node.h>

namespace Mu
{

    class TestModule : public Module
    {
    public:
        TestModule(Context* c, const char* name);
        virtual ~TestModule();

        virtual void load();
    };

} // namespace Mu

#endif // __runtime__TestModule__h__
