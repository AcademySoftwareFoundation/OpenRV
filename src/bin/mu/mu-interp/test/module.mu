//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
int x = 5;

module: foo 
{
    global int nine = 9;
    function: bar (float;) { 123.321; }
    function: something (void;) { print("what's up\n"); }
    int x = 10;
    assert(x == 10);
    assert(nine == 9);
}

assert(x == 5);
assert(foo.bar() == 123.321);
assert(foo.nine == 9);

module: foo 
{ 
    //
    //  Extend module foo
    //

    function: twoTimesBar (float;) { bar() * 2.0; }
    assert(nine == 9);
};

assert(foo.twoTimesBar() == foo.bar() * 2.0);

//
//  NOTE: if you try to extend module foo after the "use" statement,
//  mu will make a foo.foo module not extend __root.foo as you might
//  expect!
//

use foo;

assert(nine == 9);
assert(bar() == 123.321);
assert(foo.bar() == bar());

