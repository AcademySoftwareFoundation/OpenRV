#ifndef __Mu__MemberVariable__h__
#define __Mu__MemberVariable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Variable.h>

namespace Mu
{
    class Class;

    //
    //  class MemberVariable
    //	-and-
    //  class FunctionMemberVariable
    //
    //  This could be either a simple type member or a class member. If
    //  you want to use functions for access instead of the basic
    //  MemberVariable you should use FunctionMemberVariable. (see below)
    //

    class MemberVariable : public Variable
    {
    public:
        MemberVariable(Context* context, const char* name,
                       const Type* storageClass, int address = 0,
                       bool hidden = false, Attribute a = ReadWrite);

        MemberVariable(Context* context, const char* name,
                       const char* storageClass, int address = 0,
                       bool hidden = false, Attribute a = ReadWrite);

        bool isHidden() const { return _hidden; }

        //
        //	For this symbol, this can return either storageClass() or
        //	storageClass()->referenceType() depending on what the _node
        //	func is.
        //

        virtual const Type* nodeReturnType(const Node* n) const;

        //
        //	More Symbol API
        //

        virtual void output(std::ostream& o) const;
        virtual void outputNode(std::ostream&, const Node*) const;
        virtual String mangledName() const;

        //
        //	Offset
        //

        size_t instanceOffset() const { return _instanceOffset; }

    private:
        size_t _instanceOffset;
        bool _hidden;
        friend class Class;
    };

    //
    //  class InternalTypeMemberVariable
    //
    //  This variable is always of type runtime.type_symbol. Its hidden by
    //  default. InternalTypeMemberVariables never change value (they hold
    //  runtime type information).
    //

    class InternalTypeMemberVariable : public MemberVariable
    {
    public:
        InternalTypeMemberVariable(Context* c, const char* name,
                                   const Class* value);

        const Class* value() const { return _value; }

    private:
        const Class* _value;
    };

    //----------------------------------------------------------------------

    //
    //  class FunctionMemberVariable
    //
    //  This class creates something that appears to be a member variable,
    //  but is in fact a family of functions which access some unknown
    //  value from elsewhere.
    //

    class FunctionMemberVariable : public MemberVariable
    {
    public:
        FunctionMemberVariable(Context* context, const char* name,
                               const char* storageClass,
                               const Function* refFunction,
                               const Function* extractFunction = 0,
                               int address = 0, Attribute a = ReadWrite);

        FunctionMemberVariable(Context* context, const char* name,
                               const Type* storageClass,
                               const Function* refFunction,
                               const Function* extractFunction = 0,
                               int address = 0, Attribute a = ReadWrite);

        //
        //	Variable API
        //

        virtual const Function* referenceFunction() const;
        virtual const Function* extractFunction() const;

    private:
        const Function* _refFunc;
        const Function* _extractFunc;
    };

} // namespace Mu

#endif // __Mu__MemberVariable__h__
