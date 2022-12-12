//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: suffix { \: h (half; float f) { half(f); } }

let v = 1.0h + 2.0h;

assert(v == 3.0h);
print("value is %s\n" % string((v, "h")));
print("value is %.1f\n" % v);

half[] array;
half[4,4] M = {1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1 };

for (int i=0; i< 100; i++) array.push_back(half(i));
assert(array[50] == 50.0h);
for_each (h; array) print(" %g" % h);
print("\n");

let n = half(1.0 + 1.1 + 1.02  + 1.003 + 1.0004);

// some half specific functions are in the scope of half
n.round(3);
n.bits();
let q = 123h;
assert(q.bits() == 30075);
half.epsilon;
half.infinity;
