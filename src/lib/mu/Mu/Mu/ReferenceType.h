#ifndef __Mu__ReferenceType__h__
#define __Mu__ReferenceType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Node.h>
#include <Mu/Type.h>

namespace Mu
{

    class Context;
    union Value;
    class Node;

    //
    //  class ReferenceType
    //
    //  This is a type which represents a reference (pointer) to another
    //  type. Its a derived type. Normally you automatically generate
    //  ReferenceType objects for each Type you have.
    //
    //  Typeically, a language will sub-class from this and create a
    //  reference type which implements load() so that it automatically
    //  creates cast functions from the reference type to the
    //  dereferencetype.
    //
    //  You will need to implement a dereference NodeFunc foreach each
    //  type if using MU_FUNCTION_UNION.
    //

    class ReferenceType : public Type
    {
    public:
        ReferenceType(Context* context, const char* name,
                      Type* dereferenceType);

        //
        //	Type API
        //

        virtual Object* newObject() const;
        virtual Value nodeEval(const Node*, Thread& t) const;
        virtual void nodeEval(void*, const Node*, Thread& t) const;

        //
        //	Symbol API
        //

        virtual void load();

        //
        //	The type for which this type is a reference
        //

        Type* dereferenceType() { return _type; }

        const Type* dereferenceType() const { return _type; }

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	Match returns true only if there is an exact match.
        //

        MatchResult match(const Type*, Bindings&) const;

    protected:
        Type* _type;
    };

} // namespace Mu

#endif // __Mu__ReferenceType__h__
