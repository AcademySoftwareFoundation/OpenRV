//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

SortFunc := (int; int, int);

function: sort (int[]; int[] array, SortFunc comp)
{
    function: qsort (void; int[] a, int lo, int hi, SortFunc comp)
    {
        if (lo < hi) 
        {
            let l = lo,
                h = hi,
                p = a[hi];

            do 
            {
                while (l < h && comp(a[l],p) <= 0) l++;
                while (h > l && comp(a[h],p) >= 0) h--;

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

            qsort(a, lo, l-1, comp);
            qsort(a, l+1, hi, comp);
        }
    }

    qsort(array, 0, array.size() - 1, comp);
    array;
}

\: nbits (int; int a)
{
    int count = 0;
    for (int i=0; i < 32; i++) if (((a >> i) & 1) == 1) count++;
    count;
}

int[] ebits;
global int[] blookup;

for (int i=0; i < 100 ; i++) 
{
    ebits.push_back(i);
    blookup.push_back(nbits(i));
}

sort( int[] {2, 3, 7, 4, 1}, \: (int; int a, int b) { a - b; } );
sort( ebits, \: (int; int a, int b) { blookup[b] - blookup[a]; } );

