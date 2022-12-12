//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vector float[4] v = vector float[4](1,2,3,4);
print("v = %s\n" % v);
print("v.z = %s\n" % v.z);
float f = (v.x + v.y + v.z + v.w) / float(4.0);
v.x = f;
v.w = (vector float[4](0,0,0,123) + vector float[4](0,0,0,200)).w;
print("f = %s\n" % f);
assert(f == 2.5);
assert(v == vector float[4](2.5, 2, 3, 323));
