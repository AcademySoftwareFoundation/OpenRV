//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  This is a module (imath) defined purely as source
//  no .muc or .so file.
//
//  The module will fail to load if a module of the same
//  name is not defined in here.
//

module: imath
{
    use math;
    \: add (int; int a, int b) { a + b; }
    \: subtract (int; int a, int b) { a - b; }
    \: multiply (int; int a, int b) { a * b; }
    \: divide (int; int a, int b) { a / b; }
    \: modulo (int; int a, int b) { a % b; }
    \: power (int; int a, int b) { math.pow(a, b); }

    global int answer = 42;
}

