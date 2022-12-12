//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

function: puke (float; 
		int which_one,
		float vomit = math.cos(math.pi) * -1.0,
		float yak = 33.8,
		float heave = 44 - 7)
{
    if (which_one == 1) return vomit;
    if (which_one == 2) return yak;
    if (which_one == 3) return heave;

    throw exception("ok");
}


assert( puke(1) == math.cos(math.pi) * -1.0 );
assert( puke(2) == 33.8 );
assert( puke(3) == 44 - 7 );
assert( puke(1, 10) == 10 );
assert( puke(2, 10, 20) == 20 );
assert( puke(3, 10, 20, 30) == 30 );

try
{
    puke(0);
}
catch (exception exc)
{
    print(exc);
    print("\n");
}
