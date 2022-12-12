//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Test the partial application and partial evaluation as well as
//  function object reference/creation.
//


//
//  Use of a function object expression with partial apply and operator()
//

\: f1 (float; float a, float b, float c) 
{ 
    float i = b * c;
    a + i;
}

assert( f1(1,2,)(3) == f1(1,2,3) );     // direct
assert( (f1)(1,2,)(3) == f1(1,2,3) );   // indirect (always dynamic)

//
//  Use of variable containing function object w/ partial apply
//

\: f2 (float; float a, float b, float c) 
{ 
    float i = b * c;
    return a + i;
}

let f2v = f2;
assert( f2v(1,2,)(3) == f2(1,2,3) );

//
//  Same as above, but with local variables in the function
//

\: f3 (float; float a, float b, float c) 
{ 
    {
        float i = b;
        c *= i;
    }

    return a + c;
}

let f3v = f3;
assert( f3v(1,2,)(3) == f3(1,2,3) );

f3v(1,2,);

//
//  This should evaluate via const reduction to:
//  \: (int; int c) (+ 3 c)
//
\: add (int; int a, int b, int c) { a + b + c; }
let add3 = add(1,,)(2,);
assert(add3(4) == 7);

//
//  Reference type argumets with constant folding. Assert the actual
//  contents of the partially applied function.
//

\: sf (string; string a, string b, string c)
{
    a + "-" + b + "-" + c;
}

assert(sf(,"a",)("oop", "doo") == sf("oop", "a", "doo"));   // direct
assert((sf)(,"a",)("oop", "doo") == sf("oop", "a", "doo")); // indirect


//
//  Harder -- a function which takes a function -- inserting a
//  constant function object.
//

\: func_a (string; int x, (string;int) f) { f(x); }
let qq = func_a(,\: (string;int x) { "X => " + x; });
assert(qq(123) == "X => 123");
