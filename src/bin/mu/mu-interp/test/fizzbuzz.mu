//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

\: fizzbuzz (void; int start, int end)
{
    for (int i=start; i <=end; i++)
    {
        let m3 = i % 3 == 0,
            m5 = i % 5 == 0;

        if (m3) print("Fizz");
        if (m5) print("Buzz");
        if (!m3 && !m5) print("%d" % i);
        print("\n");
    }
}

fizzbuzz(1,100);
