#ifndef __Mu__FunctionType__h__
#define __Mu__FunctionType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/Class.h>
#include <Mu/Signature.h>

namespace Mu
{

    class FunctionType : public Class
    {
    public:
        FunctionType(Context* context, const char* name);
        FunctionType(Context* context, const char* name, const Signature*);
        virtual ~FunctionType();

        const Signature* signature() const { return _signature; }

        //
        //  Type API
        //

        virtual void serialize(std::ostream&, Archive::Writer&,
                               const ValuePointer) const;

        virtual void deserialize(std::istream&, Archive::Reader&,
                                 ValuePointer) const;

        virtual void reconstitute(Archive::Reader&, Object*) const;

        virtual Object* newObject() const;
        virtual Value nodeEval(const Node*, Thread& t) const;
        virtual void nodeEval(void*, const Node*, Thread& t) const;
        virtual void deleteObject(Object*) const;

        virtual void outputNode(std::ostream&, const Node*) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        virtual void load();

        //
        //  Match. If a function type A is compatible with B, it means
        //  that a function object of type A can be held in a variable of
        //  type B.
        //
        //  This can be the case if one or more of the arguments of A are
        //  base classes of the arguments in B. And/Or if A's return type
        //  is a sub-class of B's return type.
        //
        //  NOTE: Read the above again! It can be very confusing.
        //

        MatchResult match(const Type*, Bindings&) const;

    private:
        static NODE_DECLARATION(dereference, Pointer);
        static NODE_DECLARATION(disambiguate, Pointer);
        static NODE_DECLARATION(ambiguate, Pointer);
        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(eq, bool);
        static NODE_DECLARATION(print, void);

        static NODE_DECLARATION(call, void);

    private:
        const Signature* _signature;
    };

} // namespace Mu

#endif // __Mu__FunctionType__h__
