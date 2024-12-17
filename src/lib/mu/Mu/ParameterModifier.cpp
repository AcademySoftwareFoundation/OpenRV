//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ParameterModifier.h>
#include <iostream>

namespace Mu
{

    ParameterModifier::ParameterModifier(Context* context, const char* name)
        : Symbol(context, name)
    {
    }

    ParameterModifier::~ParameterModifier() {}

    void ParameterModifier::outputNode(std::ostream& o, const Node*) const
    {
        output(o);
    }

    void ParameterModifier::output(std::ostream& o) const { Symbol::output(o); }

} // namespace Mu
