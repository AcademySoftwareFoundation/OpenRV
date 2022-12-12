//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
function: equivalent (bool; string[] a, string[] b)
{
    if (a.size() != b.size()) return false;
    for (int i=0; i < a.size(); i++)
        if (a[i] != b[i]) return false;
    true;
}

require math;

let x = 10.0;           // x => float
let s = math.sin(x);    // s => float
let y = 10;             // y => int


string foo = "foo|bar";
let tokens = string.split(foo, "|"); 
assert(equivalent(tokens, string[] {"foo", "bar"}));

//
// comma seperated let declares each variable using its own rhs
//

let a = 10 % 6,             // a => int
    b = math.cos(math.pi),  // b => float
    c = "chicken";          // c => string

assert(c == "chicken");
assert(a == 4);
assert(b == math.cos(math.pi));


//
//  Unresolved implicit typing
//  The types of local variables may require patching
//

function: foo (int; int a)
{
    let x = bar(a, 10),
        y = float(bar(a, 11));

    x + y;
}

function: bar (int; int a, int b) 
{ 
    a + b; 
}

assert(foo(11) == 43);

//
//  Pattern matching
//


{
    class: Foo { float a; (float,float) b; }

    let pi = float(math.pi);

    let (a, {b, (c0, c1)}, (d, e, f)) = (1, Foo(2.0, (3.0, pi)), (9, 8, 7));

    assert(c1 == pi);
}

