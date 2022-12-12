//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

function: negabs(int; int x)
{
    if (x > 0) return -x;
    return x;
}

assert( negabs(1) == -1 && negabs(-1) == -1 );

function: foobar(string;)
{
    if (true)
    {
	return "foobar did its thing\n";
    }
    else
    {
	return "problem\n";
    }
}

print( foobar() );


function: lotsOluck(string;)
{
    for (int i=0; i < 10; i++)
    {
	if (i == 3)
	{
	    return "returned ok";
	}
    }

    assert(false);
    return "not so good";
}

print( lotsOluck() + "\n" );

function: breakAndReturn(int;)
{
    for (int i=0; i < 10; i++)
    {
	if (i == 2)
	{
	    for (int q=0; q < 10; q++)
	    {
		if (q == 5) return q;
		continue;
	    }
	}
    }

    return -1;
}

assert( breakAndReturn() == 5 );


