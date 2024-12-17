#ifndef __Mu__InteractiveSession__h__
#define __Mu__InteractiveSession__h__
//
// Copyright (c) 2013, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <iostream>

namespace Mu
{
    class Context;
    class Process;
    class Thread;

    class InteractiveSession
    {
    public:
        InteractiveSession();
        void run(Context*, Process*, Thread*);
    };

} // namespace Mu

#endif // __Mu__InteractiveSession__h__
