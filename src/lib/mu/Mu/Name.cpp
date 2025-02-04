//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Name.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    ostream& operator<<(ostream& o, Name n)
    {
        // o << " (item=" << hex << n._item << dec << ") ";
        return o << static_cast<const String&>(n);
    }

    bool Name::includes(const char* s, size_t start) const
    {
        String t(*this);
        return t.find(s, start) == start;
    }

    bool Name::PointerSortKey(Name a, Name b) { return a._item < b._item; }

} // namespace Mu
