//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/TypeModifier.h>
#include <iostream>

namespace Mu
{

    TypeModifier::TypeModifier(Context* context, const char* name)
        : Symbol(context, name)
    {
    }

    TypeModifier::~TypeModifier() {}

    void TypeModifier::outputNode(std::ostream& o, const Node*) const
    {
        output(o);
    }

    void TypeModifier::output(std::ostream& o) const { Symbol::output(o); }

    const Type* TypeModifier::transform(const Type* t, Context*) const
    {
        return t;
    }

} // namespace Mu
