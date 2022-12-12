//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

assert("image.%04d.jpg" % 4 == "image.0004.jpg");
assert("foo%d" % 1 == "foo1");
assert("%dfoo" % 1 == "1foo");
assert("math.pi is %g and math.e is %g" % (math.pi, math.e) ==
       "math.pi is 3.14159 and math.e is 2.71828");


string x = ""; 

for (int i = 0; i < 4097; ++i) 
{
    assert(x.size() == i);
    x = "%s." % x;
} 

