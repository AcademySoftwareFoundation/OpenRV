#ifndef __mu_test__SequenceInterface__h__
#define __mu_test__SequenceInterface__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Interface.h>

namespace Mu
{

    class SequenceInterface : public Interface
    {
    public:
        SequenceInterface(Context* c);
        virtual ~SequenceInterface();

        virtual void load();
    };

} // namespace Mu

#endif // __mu_test__SequenceInterface__h__
