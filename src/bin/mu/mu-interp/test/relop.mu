//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
float a = 1.0;
float b = 2.0;

if (a == b)
{
    assert(false);
}
else
{
    if (a != b)
    {
	if (a < b)
	{
	    if (a > b)
	    {
		print(0);
	    }
	    else
	    {
		if (a <= b && !(a >= b))
		{
		    print(1);
		}
		else
		{
		    assert(false);
		}
	    }
	}
	else
	{
	    assert(false);
	}
    }
    else
    {
	assert(false);
    }
}
