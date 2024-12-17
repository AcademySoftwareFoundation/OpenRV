//
//  Copyright (c) 2016 Autodesk.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkUtil/FNV1a.h>

namespace TwkUtil
{
    using namespace std;

    //
    //  This could use some optimization if the compiler can't handle it
    //

#define fnv_prime (1099511628211u)
#define fnv_offset_basis (14695981039346656037u)

    size_t FNV1a64(const void* data, size_t size)
    {
        size_t hash = fnv_offset_basis;
        const char* beg = reinterpret_cast<const char*>(data);
        const char* end = beg + size;

        for (const char* p = beg; p != end; p++)
        {
            hash = (hash ^ *p) * fnv_prime;
        }

        return hash;
    };

} // namespace TwkUtil
