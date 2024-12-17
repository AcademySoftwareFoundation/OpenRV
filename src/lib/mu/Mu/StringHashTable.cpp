//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/StringHashTable.h>

namespace Mu
{

    unsigned long StringTraits::hash(const Mu::String& s)
    {
        //
        //	Taken from the ELF format hash function
        //

        unsigned long h = 0, g;

        for (int i = 0, l = s.size(); i < l; i++)
        {
            h = (h << 4) + s[i];
            if (g = h & 0xf0000000)
                h ^= g >> 24;
            h &= ~g;
        }

        return h;
    }

} // namespace Mu
