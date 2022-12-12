//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  This file has to be backwards compatible with the original sophia
//  or mel
//

// old:		    170u 1.3s	2:58
// new:		    124u 0.99s	2:10 
// new_no_setjmp:   100u 0.6s	1:45
// safe_no_setjmp:  173u 1.1s	3:00

// not safe, setjmp 122.730u 1.030s 2:10.65 94.7%   0+0k 0+5io 0pf+0w

// latest opt: 117.350u 0.190s 1:59.51 98.3%   0+0k 0+0io 0pf+0w

int i,j;
float q=0;

for (j=0; j<100; j++)
for (i=0; i<1000000; i++)
{
    if ((i % 2)==0)
    {
        q += (float(i) * ( -1. ))/1000000.0;
    }
    else
    {
        q += (float(i) * ( 1.))/1000000.0;
    }
}

print(q);
