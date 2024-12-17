#ifndef __MuLang__AClassType__h__
#define __MuLang__AClassType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <mu_test/BaseType.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class AClassType
    //
    //
    //

    class AClassType : public Class
    {
    public:
        AClassType(Context* c, Class* super);
        ~AClassType();

        struct Layout : public BaseType::Layout
        {
            int value;
        };

        virtual void load();

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(foo, Pointer);
        static NODE_DECLARATION(bar, Pointer);
    };

} // namespace Mu

#endif // __MuLang__AClassType__h__
