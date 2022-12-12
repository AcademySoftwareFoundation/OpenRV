//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

v4 := vector float[4];
v3 := vector float[3];

float a = 1;
float b = 2;
v4 v0 = (v4(1,2,3,4) + v4(1,1,1,1)) / v4(2,2,2,2);
v4 v1 = v4(a,b,a,b);
v4 v2 = (v0 * v1) + v0 / v1;

operator: % (v3; v3 a, v3 b) { cross(a,b); }
operator: ^ (float; v3 a, v3 b) { dot(a,b); }

v3 av = v3(1, 0, 0);
v3 bv = v3(0, 1, 0);
v3 cv = cross(av, bv);
assert(mag(cv) == 1.0);
assert((v3(0, 1, 0) ^ v3(1, .5, 0)) == .5);
