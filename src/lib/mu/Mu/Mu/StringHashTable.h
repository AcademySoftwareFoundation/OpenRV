#ifndef __Mu__StringHashTable__h__
#define __Mu__StringHashTable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/HashTable.h>

namespace Mu
{

    //
    //  StringHashTable is a typedef which implements the HashTable<>
    //  class for String.
    //

    struct StringTraits
    {
        static int compare(const String& a, const String& b)
        {
            return a.compare(b);
        }

        static bool equals(const String& a, const String& b) { return a == b; }

        static unsigned long hash(const String&);
    };

    typedef Mu::HashTable<String, StringTraits> StringHashTable;

} // namespace Mu

#endif // __Mu__StringHashTable__h__
