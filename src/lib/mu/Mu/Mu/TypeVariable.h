#ifndef __Mu__TypeVariable__h__
#define __Mu__TypeVariable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Type.h>

namespace Mu
{

    //
    //  TypeVariable
    //
    //  This class represents a place holder or free type. When type
    //  matching is occuring, any free TypeVariables are bound up. If
    //  there are no conflicts during binding, then the a match can
    //  occur. The variables only hold values long enough to match.
    //

    class TypeVariable : public Type
    {
    public:
        //
        //  Types
        //

        typedef STLMap<const TypeVariable*, const Type*>::Type Bindings;

        //
        //  Constructors
        //

        TypeVariable(Context* context, const char* name);
        virtual ~TypeVariable();

        //
        //  Type API
        //

        virtual Object* newObject() const;
        virtual Value nodeEval(const Node*, Thread& t) const;
        virtual void nodeEval(void*, const Node*, Thread& t) const;
        virtual const Type* nodeReturnType(const Node*) const;
        virtual void outputValue(std::ostream&, Value&) const;
        virtual MatchResult match(const Type*, Bindings&) const;
    };

} // namespace Mu

#endif // __Mu__TypeVariable__h__
