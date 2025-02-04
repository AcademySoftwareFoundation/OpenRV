//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext__string_hash_table__h__
#define __stl_ext__string_hash_table__h__
#include <stl_ext/hash_table.h>
#include <stl_ext/string_algo.h>
#include <string>

namespace stl_ext
{

    //
    //  string_hash_table is a typedef which implements the hash_table<> class
    //  for std::string. See hash_table.h for information on that class.
    //

    struct string_traits
    {
        static bool equals(const std::string& a, const std::string& b)
        {
            return a == b;
        }

        static unsigned long hash(const std::string& s)
        {
            return stl_ext::hash(s);
        }
    };

    typedef stl_ext::hash_table<std::string, string_traits> string_hash_table;

} // namespace stl_ext

#endif // __stl_ext__string_hash_table__h__
