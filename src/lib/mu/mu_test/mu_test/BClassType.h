#ifndef __MuLang__BClassType__h__
#define __MuLang__BClassType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <mu_test/BaseType.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class BClassType
    //
    //
    //

    class BClassType : public Class
    {
    public:
        BClassType(Context* c, Class* super);
        ~BClassType();

        struct Layout : public BaseType::Layout
        {
            float value;
            char ch;
        };

        virtual void load();

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(foo, Pointer);
        static NODE_DECLARATION(bar, Pointer);
    };

} // namespace Mu

#endif // __MuLang__BClassType__h__
