//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use runtime;

//
//  Generic quicksort
//

operator: <= (bool; string a, string b) { compare(a, b) <= 0; }
operator: >= (bool; string a, string b) { compare(a, b) >= 0; }

function: qsort (void; 'a a, int lo, int hi)
{
    if (lo < hi) 
    {
        let l = lo,
            h = hi,
            p = a[hi];
        
        do 
        {
            while (l < h && a[l] <= p) l++;
            while (h > l && a[h] >= p) h--;
            
            if (l < h) 
            {
                let t = a[l];
                    a[l] = a[h];
                    a[h] = t;
            }
            
        } while (l < h);
        
        let t = a[l];
        a[l] = a[hi];
        a[hi] = t;
        
        qsort(a, lo, l-1);
        qsort(a, l+1, hi);
    }
}

int[] a = { 10, 1, 2, 9, 4, 3, 6, 5, 8, 7 };
string[] b = {"hello", "a", "one", "two", "zebra"};
qsort(a, 0, a.size() - 1);
qsort(b, 0, b.size() - 1); 
a;
