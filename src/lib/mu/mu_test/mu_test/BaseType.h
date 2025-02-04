#ifndef __MuLang__BaseType__h__
#define __MuLang__BaseType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/ClassInstance.h>
#include <Mu/Class.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/StringType.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class BaseType
    //
    //
    //

    class BaseType : public Class
    {
    public:
        BaseType(Context* c, Class* super = 0);
        ~BaseType();

        struct Layout
        {
            ClassInstance* next;
            const StringType::String* name;
        };

        virtual void load();

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(foo, Pointer);
    };

} // namespace Mu

#endif // __MuLang__BaseType__h__
