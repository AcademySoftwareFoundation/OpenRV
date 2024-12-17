//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

namespace Mu
{

    //
    // Note: assumes long is at least 32 bits.
    // Ripped off of SGI's STL with some additional low-end primes.
    //
    static const int num_primes = 31;
    static const unsigned long prime_list[num_primes] = {
        7,           13,        23,        53,        97,         193,
        389,         769,       1543,      3079,      6151,       12289,
        24593,       49157,     98317,     196613,    393241,     786433,
        1572869,     3145739,   6291469,   12582917,  25165843,   50331653,
        100663319,   201326611, 402653189, 805306457, 1610612741, 3221225473ul,
        4294967291ul};

    unsigned long nextPrime(unsigned long last)
    {
        for (int i = 0; i < num_primes; i++)
            if (prime_list[i] > last)
                return prime_list[i];
        return 0;
    }

} // namespace Mu
