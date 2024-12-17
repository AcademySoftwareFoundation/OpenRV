//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__$TType__h__
#define __MuQt__$TType__h__
#include <iostream>
#include <Mu/Class.h>

namespace Mu
{

    class QtColorTriangleType : public Class
    {
    public:
        //
        //  Constructors
        //

        QtColorTriangleType(Context* context, const char* name,
                            Class* superClass = 0);
        virtual ~QtColorTriangleType();

        //
        //  Class API
        //

        virtual void load();
    };

} // namespace Mu

#endif // __MuQt__QtColorTriangleType__h__
