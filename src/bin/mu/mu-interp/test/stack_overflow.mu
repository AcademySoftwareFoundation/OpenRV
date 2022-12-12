//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

function: overflow (int; int a, int b, int c, int d, int e, int f)
{
    print("frame " + a + "\n");
    a++;
    overflow(a,b,c,d,e,f);
}

try
{
    //overflow(1,2,3,4,5,6);
    print("stack limit removed\n");
}
catch (exception e)
{
    print("caught: ");
    print(e);
    print("\n");
}

