//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
// The first program uses arrays as if they were indexed. It sets n
// elements of an array to integer values, then copies the array to
// another array beginning at index n-1. 
//

int n = 200000;

// int[] x, y;
// x.resize(n);
// y.resize(n);

int[200000] x, y;

for (int i = 0; i < n; i++)
{
    x[i] = i;
}

for (int j = n-1; j >= 0; j--)
{
    y[j] = x[j];
}
