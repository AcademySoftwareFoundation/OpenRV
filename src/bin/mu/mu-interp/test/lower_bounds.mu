//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
\: lower_bounds (int; int[] array, int n)
{
    \: f (int; int[] array, int n, int i, int i0, int i1)
    {
        if (array[i] <= n)
        {
            if (i+1 == array.size() || array[i+1] > n)
            {
                return i;
            }
            else
            {
                return f(array, n, (i + i1) / 2, i, i1);
            }
        }

        if i == 0 then -1 else f(array, n, (i + i0) / 2, i0, i);
    }

    f(array, n, array.size() / 2, 0, array.size());
}

int[] array = int[] {10, 20, 30, 40, 50, 60, 70, 80, 90};
let index = lower_bounds(array, 81);
assert(index == 7);
