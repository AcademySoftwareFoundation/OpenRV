//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

function: foo(int;)
{
    use math;
    max(1,2);
}

float f = 1.0;
math.abs(f);
use math;
abs(f);

function: bar(int;)
{
    max(1,2);
}

{
    use string; // using a type instead of module
    assert(split("foo bar", " ").back() == "bar");
}
