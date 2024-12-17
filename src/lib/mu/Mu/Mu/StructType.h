#ifndef __MuLang__StructType__h__
#define __MuLang__StructType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <vector>
#include <Mu/Type.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/MachineRep.h>

namespace Mu
{
    class Thread;

    //
    //  StructType
    //
    //  This is a simple record type. Fields named and ordered. No additional
    //  functions can be declared inside a struct. Its just the data.
    //

    class StructType : public Class
    {
    public:
        typedef std::pair<std::string, const Type*> NameValuePair;
        typedef STLVector<NameValuePair>::Type NameValuePairs;

        StructType(Context* context, const char* name,
                   const NameValuePairs& fields);
        ~StructType();

        virtual void load();

        static NODE_DECLARATION(defaultConstructor, Pointer);
        static NODE_DECLARATION(aggregateConstructor, Pointer);

        const NameValuePairs& fields() const { return _fields; }

    private:
        NameValuePairs _fields;
    };

} // namespace Mu

#endif // __MuLang__StructType__h__
