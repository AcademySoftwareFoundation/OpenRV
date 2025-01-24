//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Vector.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    ostream& operator<<(ostream& o, const Vector4f& v)
    {
        return o << "<" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3]
                 << ">";
    }

} // namespace Mu
