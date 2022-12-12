//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

float i = 1.23;

if (true) 
{ 
    int q = math.max(1,2); 
    i *= 100;

    if (true)
    {
	int i = 10;
	assert(i == 10);
    }
    else
    {
	assert(false);
	int c = 1;
    }

    assert(i == 123.0);
    q += 2;
}
