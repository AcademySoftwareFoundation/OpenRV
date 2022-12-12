//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

int[4] array;
array[0] = 1;
array[1] = 2;
array[2] = 3;
array[3] = 4;

int[4] copiedArray = int[4](array);
copiedArray[0] = 10;
assert(copiedArray[0] != array[0]);
assert(copiedArray[3] == 4);
assert(copiedArray[2] == 3);
assert(copiedArray[1] == 2);

for (int i=0; i < 4; i++) assert(array[i] == i+1);

float[4][] farray;
farray.push_back( float[4]() );
farray.back()[0] = 1;
farray.back()[1] = 2;
farray.back()[2] = 3;
farray.back()[3] = 4;

int[4][][10][100,50,1][][][][59,1] jesus;

float[4,4] M;
M[0,0] = 10.0;
assert(M[0,0] == 10.0);

float[4,4][1000] matrices;

for (int i=0; i < 1000; i++)
{
    matrices[i] = float[4,4]();

    for (int j=0; j<4; j++)
    {
	for (int k=0; k<4; k++)
	{
	    if (j == k)
	    {
		matrices[i][j,k] = 1;
	    }
	    else
	    {
		matrices[i][j,k] = 0;
	    }
	}
    }
}


