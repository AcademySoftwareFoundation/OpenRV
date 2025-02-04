#ifndef __MuLang__CClassType__h__
#define __MuLang__CClassType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <mu_test/BaseType.h>
#include <Mu/Node.h>
#include <Mu/ClassInstance.h>
#include <Mu/Class.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/DynamicArray.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class CClassType
    //
    //
    //

    class CClassType : public Class
    {
    public:
        CClassType(Context* c, Class* super);
        ~CClassType();

        struct Layout : public BaseType::Layout
        {
            ClassInstance* parent;
            DynamicArray* children;
        };

        virtual void load();

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(baz, Pointer);
        static NODE_DECLARATION(bar, Pointer);
    };

} // namespace Mu

#endif // __MuLang__CClassType__h__
