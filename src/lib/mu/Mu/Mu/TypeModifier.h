#ifndef __Mu__TypeModifier__h__
#define __Mu__TypeModifier__h__
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
    class Context;

    //
    //  class TypeModifier
    //
    //  Usually transforms one type into another type. Used for things
    //  like "varying" or "const".
    //

    class TypeModifier : public Symbol
    {
    public:
        TypeModifier(Context* context, const char* name);
        virtual ~TypeModifier();

        //
        //	Symbol API
        //

        virtual void outputNode(std::ostream&, const Node*) const;

        //
        //	Output the symbol
        //

        virtual void output(std::ostream&) const;

        //
        //	Typemodifier API
        //

        virtual const Type* transform(const Type*, Context*) const;
    };

} // namespace Mu

#endif // __Mu__TypeModifier__h__
