//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

global float a = 10.0;
global string s = "hello world\n";

function: print_a (float;)
{
    a = 123.3;
    print(a + "\n");
    a;
}

function: print_s (string;)
{
    print(s + "\n");
    s;
}

assert( print_a() == a );
assert( print_s() == s );
