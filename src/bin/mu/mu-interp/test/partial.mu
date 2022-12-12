//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//


//module: suffix {
    //\: pi (void; float f) { math.pi * f; }
//}

// \: foo ()
// {
//     string suffix = "foo";
// }

\: add (int; int a, int b) { a + b; }

\: foo (int;)
{
    add(1,)(2);
}

