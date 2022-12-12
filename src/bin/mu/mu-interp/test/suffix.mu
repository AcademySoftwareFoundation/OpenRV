//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
// Constant suffixes are all declared in the root level suffix module.
// You can add any suffix you want to this module.
//

module: suffix
{
    \: f (float; int i) { i; }
    \: f (float; float f) { f; }
    \: f (float; string s) { float(s); }

    \: pi (float; float f) { math.pi * f; }
    \: pi (float; int f) { math.pi * f; }

    \: v3 (vector float[3]; float f) { (vector float[3])(f,f,f); }
    \: v4 (vector float[4]; float f) { (vector float[4])(f,f,f,f); }
}

let f = 10f,
    v = 10.0v4;

print("%f\n" % f);
print("%s\n" % v);

"123.321"f; // weird but legit because of definition for strings
4 * 2 pi;    // another weird suffix usage
