//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
float_array := float[];
float_array foo;
foo.push_back(0);
foo.push_back(1);
foo.push_back(2);
foo.push_back(3);

a := float;	    // a = float
b := a[];	    // b = float[]
c := b[];	    // c = float[][]
d := c;		    // d = float[][]
e := b;		    // e = float[]

d array;
array.push_back( e() );
array.back().push_back(69.0);

m := math;	    // module alias
sine := m.sin;	    // function alias

assert(sine(array.back().back()) == math.sin(69.0));

potato := float_array.push_back;	// member function alias (exact)
assert(foo.size() == 4);

potato(foo, 123.321);		// aliased call to a member function

assert(foo.size() == 5);
assert(foo[4] == 123.321);

int crap = 10;
wad := crap;	    // wad is an alias for crap
assert(wad == 10);
wad = 20;
assert(crap == 20);

function: add (float; float x, float y) { x + y; }

sum := add;
assert( sum(1.0, 2.0) == 3.0 );

vec := vector float[4];

