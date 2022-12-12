//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Sieve imperative style
//

\: nsieve (void; int m) 
{
    int count = 0;
    bool[] isPrime;
    isPrime.resize(m);

    for (int i=2; i < m; i++) 
        isPrime[i] = true;

    for (int i = 2; i < m; i++)
    {
        if (isPrime[i]) 
        {
            count++;
            for (int j = i << 1; j < m; j += i) 
                isPrime[j] = false;
        }
    }

    print("Primes up to %8d %8d\n" % (m, count));
}

int m = 9;

for (int i = 0; i < 3; i++) 
{
    nsieve(10000 << (m-i));
}

