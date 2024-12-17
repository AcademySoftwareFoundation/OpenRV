#ifndef __Mu__Unresolved__h__
#define __Mu__Unresolved__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>
#include <Mu/Type.h>

namespace Mu
{

    class UnresolvedSymbol : public Symbol
    {
    public:
        UnresolvedSymbol(Context* context, const char*);
        virtual ~UnresolvedSymbol();
        virtual const Type* nodeReturnType(const Node*) const;
    };

    class UnresolvedType : public Type
    {
    public:
        UnresolvedType(Context* context);

        virtual Object* newObject() const;
        virtual Value nodeEval(const Node*, Thread& t) const;
        virtual void nodeEval(void*, const Node*, Thread& t) const;
        virtual void outputValue(std::ostream&, Value&) const;
    };

    class UnresolvedCall : public UnresolvedSymbol
    {
    public:
        UnresolvedCall(Context*);
        virtual ~UnresolvedCall();
    };

    class UnresolvedCast : public UnresolvedSymbol
    {
    public:
        UnresolvedCast(Context*);
        virtual ~UnresolvedCast();
    };

    class UnresolvedConstructor : public UnresolvedSymbol
    {
    public:
        UnresolvedConstructor(Context*);
        virtual ~UnresolvedConstructor();
    };

    class UnresolvedReference : public UnresolvedSymbol
    {
    public:
        UnresolvedReference(Context*);
        virtual ~UnresolvedReference();
    };

    class UnresolvedDereference : public UnresolvedSymbol
    {
    public:
        UnresolvedDereference(Context*);
        virtual ~UnresolvedDereference();
    };

    class UnresolvedMemberReference : public UnresolvedSymbol
    {
    public:
        UnresolvedMemberReference(Context*);
        virtual ~UnresolvedMemberReference();
    };

    class UnresolvedMemberCall : public UnresolvedSymbol
    {
    public:
        UnresolvedMemberCall(Context*);
        virtual ~UnresolvedMemberCall();
    };

    class UnresolvedStackReference : public UnresolvedSymbol
    {
    public:
        UnresolvedStackReference(Context*);
        virtual ~UnresolvedStackReference();
    };

    class UnresolvedStackDereference : public UnresolvedSymbol
    {
    public:
        UnresolvedStackDereference(Context*);
        virtual ~UnresolvedStackDereference();
    };

    class UnresolvedDeclaration : public UnresolvedSymbol
    {
    public:
        UnresolvedDeclaration(Context*);
        virtual ~UnresolvedDeclaration();
    };

    class UnresolvedStackDeclaration : public UnresolvedDeclaration
    {
    public:
        UnresolvedStackDeclaration(Context*);
        virtual ~UnresolvedStackDeclaration();
    };

    class UnresolvedAssignment : public UnresolvedSymbol
    {
    public:
        UnresolvedAssignment(Context*);
        virtual ~UnresolvedAssignment();
    };

} // namespace Mu

#endif // __Mu__Unresolved__h__
