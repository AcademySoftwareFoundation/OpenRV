#ifndef __Mu__Construct__h__
#define __Mu__Construct__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Function.h>

namespace Mu
{

    //
    //  class Construct
    //
    //  A special type of function that evaluates some of its arguments
    //  conditionally and/or repeatedly.
    //

    class Construct : public Function
    {
    public:
        Construct(Context* context, const char* name, NodeFunc,
                  Attributes attributes, ...);
        virtual ~Construct();

        //
        //	Symbol API
        //

        virtual void output(std::ostream&) const;

    protected:
        Construct(Context*, const char* name);
    };

} // namespace Mu

#endif // __Mu__Construct__h__
