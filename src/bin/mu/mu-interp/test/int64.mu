//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

for_each (m; runtime.machine_types()) 
{
    if (m._0 == "int64")
        print("name=(%s %s) size=%d width=%d salign=%d nalign=%d\n" % m);
}

module: suffix 
{ 
    \: L (int64; int a) { a; } 
    \: L (int64; int64 a) { a; } 
    \: I (int; int a) { a; } 
}

let x = 123L,
    y = int(x),
    z = int64(x),
    m = -9223372036854775807;   // must be 64

"foo " + int64.min;
                
// too big for 32bits, must be 64
print("9223372036854775807 = %d\n" % 9223372036854775807);
// format conversion should be automatic based on arg types
print("int64.max padded 20 = \"%20x\"\n" % int64.max);
print("int64.min padded 20 = \"%20x\"\n" % int64.min);
print("int64.max = %d\n" % int64.max);
print("float(int64.max) = %f\n" % float(int64.max));
print("sum = %d\n" % (int64.min + int64.max));

int64[4,4] M = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
let l = [1L, 2L, 3L];
let a = int64[] {10, 20, int64.max, int64.min};
