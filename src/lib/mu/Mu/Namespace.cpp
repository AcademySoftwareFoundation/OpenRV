//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Namespace.h>

namespace Mu
{
    using namespace std;

    Namespace::Namespace(Context* context, const char* name)
        : Symbol(context, name)
    {
        _searchable = true;
    }

    Namespace::~Namespace() {}

} // namespace Mu
