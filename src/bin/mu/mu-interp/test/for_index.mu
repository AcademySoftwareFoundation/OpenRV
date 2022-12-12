//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

string[] array = {"one", "two", "three", "four", "five"};
int[] iarray = {1,2,3,4,5};

for_index (i; array)
{
    print("%s = %d\n" % (array[i], iarray[i]));
}

//
//  This function should be marked pure
//  but its not.
//

\: transpose(float[4,4]; float[4,4] M)
{
    float[4,4] N;
    for_index (i,j; M) N[j,i] = M[i,j];
    N;
}


let M = transpose( float[4,4] {1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               1, 2, 3, 1} );

assert(M[1,3] == 2 && M[2,3] == 3);

int[2,2,2] X = {1,2,3,4,5,6,7,8};

for_index (i,j,k; X)
{
    print("i,j,k => %d,%d,%d = %d\n" % (i,j,k,X[i,j,k]));
    assert(X[i,j,k] == i * (2 * 2) + j * 2 + k + 1);
}
