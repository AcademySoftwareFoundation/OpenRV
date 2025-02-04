#ifndef __Mu__MemberFunction__h__
#define __Mu__MemberFunction__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Function.h>

namespace Mu
{
    class Class;

    //
    //  class MemberFunction
    //
    //  A function which is a member of a class -- in C++ parlance, a
    //  "virtual function".
    //

    class MemberFunction : public Function
    {
    public:
        enum MFHints
        {
            Destructor = 1 << 11, // continuation of Function hints
        };

        typedef STLVector<const MemberFunction*>::Type MemberFunctionVector;

        //
        //  Constructors are used in the same way that Function
        //  constructors are used. Indeed, they call the same init
        //  function in Function.
        //

        MemberFunction(Context* context, const char* name, NodeFunc,
                       Attributes attributes, ...);

        MemberFunction(Context* context, const char* name,
                       const Type* returnType, int nparams, ParameterVariable**,
                       Node*, Attributes attributes);

        MemberFunction(Context* context, const char* name,
                       const Type* returnType, int nparams, ParameterVariable**,
                       NodeFunc, Attributes attributes);

        virtual ~MemberFunction();

        //
        //	Symbol API
        //

        virtual void output(std::ostream&) const;

        //
        //	Returns the class of which this is a memeber
        //

        Class* memberClass();
        const Class* memberClass() const;

        //
        //  Find all functions which implement this function
        //

        void findOverridingFunctions(MemberFunctionVector&) const;

        //
        //	Offset into vtable
        //

        size_t offset() const { return _offset; }

        //
        //	matches() returns true if the name, return value, and
        //	arguments except the "self" argument match.
        //

        virtual bool matches(const Function*) const;

        //
        //

        bool isConstructor() const;

    protected:
        MemberFunction(Context*, const char*);

    private:
        //
        //	The member function has an offset into the vitual function
        //	table. This offset is given to the MemberFunction by the Class
        //	of which it is a member.
        //

        size_t _offset;
        bool _destructor : 1;
        friend class Class;
    };

} // namespace Mu

#endif // __Mu__MemberFunction__h__
