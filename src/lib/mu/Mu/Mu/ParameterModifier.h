#ifndef __Mu__ParameterModifier__h__
#define __Mu__ParameterModifier__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>

namespace Mu
{
    class Context;
    class ParameterVariable;

    //
    //  class ParameterModifier
    //
    //  Used as a keyword much like a TypeModifier is. Sets attributes of
    //  a ParameterVariable.
    //

    class ParameterModifier : public Symbol
    {
    public:
        ParameterModifier(Context* context, const char* name);
        virtual ~ParameterModifier();

        //
        //	Symbol API
        //

        virtual void outputNode(std::ostream&, const Node*) const;

        //
        //	Output the symbol
        //

        virtual void output(std::ostream&) const;

        //
        //  ParameterModifier APi
        //

        virtual void modify(ParameterVariable*) const = 0;
    };

} // namespace Mu

#endif // __Mu__TypeModifier__h__
