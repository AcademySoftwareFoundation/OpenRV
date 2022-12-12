//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
require math;
require math_util;

//
//  Parse some obnoxious function type declarations
//

poop  := ((float; vector float[3]); (float; float), ((float; int); int));
poop2 := ((float[]; vector float[3]); (float; float), ((float; int); int));

(void; float) foo = print;
print(foo); print("\n");

\: sprint (string; string s) { s + " world"; }
\: foobar (void;) { print("foobar\n"); }

(string; string) x = sprint;
(float; float) s = math.sin;
(void;) v = foobar;
(float; float, float) r = math_util.random;


assert( x("hello") == "hello world" );
assert( s(math.pi / 2.0) == math.sin(math.pi / 2.0) );
v();
math_util.seed(10);
float rr = r(1.0, 2.0);
math_util.seed(10);
float rn = math_util.random(1.0, 2.0);
assert(rr == rn);

//
//  Try a little functional programming. Its in its own module in
//  order to protect it from future primitive functions "map" and
//  "fold"
//

module: test 
{
    \: add (int; int a, int b) { a + b; }
    \: times (int; int a, int b) { a * b; }
    \: sub (int; int a, int b) { a - b; }

    \: fold (int; (int;int,int) f, [int] a, int i)
    {
        let ac = i;
        for_each (e; a) { ac = f(e, ac); }
        ac;
    }

    let list = [1, 2, 3, 4];

    assert(fold(add, list, 0) == 10);
    assert(fold(times, list, 1) == 24);
    assert(fold(sub, list, 0) == 2);

    \: println (void; string s) { print(s + "\n"); }
    \: map (void; (void;string) f, [string] a) { for_each (e; a) f(e); }

    map(println, ["Hello", "World"]);

    //
    //  Ok let's get crazy
    //

    let flist = [add, times, sub];
    [int] ac;
    for_each (f; flist) ac = fold(f, list, 0) : ac;
    fold(add, ac, 0);

    //
    //  Lambda expressions
    //
    
    let F = \: (float; float a, float b) { (a + b) / (a - b); };
    assert( F(3, 7) == -2.5 );

    map( \: (void; string s) { print("--> %s <--\n" % s); },
         [ "Hello", "World", "using", "map" ] );
}

