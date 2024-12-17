#ifndef __MuLang__TupleType__h__
#define __MuLang__TupleType__h__
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

    class TupleType : public Class
    {
    public:
        typedef STLVector<const Type*>::Type Types;

        TupleType(Context* context, const char* name, const Types& fieldTypes);
        ~TupleType();

        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();

        const Types& tupleFieldTypes() const { return _types; }

        static NODE_DECLARATION(defaultConstructor, Pointer);
        static NODE_DECLARATION(aggregateConstructor, Pointer);

    private:
        Types _types;
    };

} // namespace Mu

#endif // __MuLang__TupleType__h__
