//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Construct.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    Construct::Construct(Context* context, const char* name)
        : Function(context, name)
    {
    }

    Construct::Construct(Context* context, const char* name, NodeFunc func,
                         Attributes attributes, ...)
        : Function(context, name)
    {
        va_list ap;
        va_start(ap, attributes);
        init(func, attributes, ap);
        va_end(ap);
    }

    Construct::~Construct() {}

    void Construct::output(std::ostream& o) const
    {
        o << "construct ";
        Function::output(o);
    }

} // namespace Mu
