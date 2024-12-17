#ifndef __Mu__Namespace__h__
#define __Mu__Namespace__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>

namespace Mu
{

    //
    //  class Namespace
    //
    //  A symbol that provides a scope
    //

    class Namespace : public Symbol
    {
    public:
        Namespace(Context* context, const char* name);
        virtual ~Namespace();
    };

} // namespace Mu

#endif // __Mu__Namespace__h__
