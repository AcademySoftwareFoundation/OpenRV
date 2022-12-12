//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#include <stdio.h>

//
//  This file has to be backwards compatible with the original sophia
//  or mel
//

int main(int, char**)
{

int i,j;
float q=0;

for (j=0; j<100; j++)
for (i=0; i<1000000; i++)
{
    q += (float(i) * ((i % 2)==0 ? -1. : 1.))/1000000.0;
}

printf("%f\n",q);

 return 0;
}
