//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkContainer/DefaultValues.h>

namespace TwkContainer
{
    using namespace std;

    ostream& operator<<(ostream& o, const StringPair& v)
    {
        return o << "(" << v.first << ", " << v.second << ")";
    }

} // namespace TwkContainer
