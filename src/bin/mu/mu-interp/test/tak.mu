//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
/*  takeuchi benchmark in ici
 *  see <url:http://www.lib.uchicago.edu/keith/crisis/benchmarks/tak/>
 *  Tod Olson <ta-olson@uchicago.edu>
 *
 *  This is the fp version of tak
 */

function: tak (float; float x, float y, float z)
{
    if y >= x then z else tak(tak(x - 1.0, y, z),
                              tak(y - 1.0, z, x),
                              tak(z - 1.0, x, y));
}

float n = 7.0;
assert(tak(n * 3.0, n * 2.0, n * 1.0) == 14.0);
//assert(tak(28, 12, 6) == 7);
