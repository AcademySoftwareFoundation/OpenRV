//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

use math;
require runtime;

float[] array;

print(array);
print("\n");
array.push_back(1.0);
array.push_back(2.0);
assert(array.size() == 2);
assert(array[0] == 1.0 && array[1] == 2.0);


for (int i=0; i < 1000; i++) array.push_back(i);
assert(array.size() == 1002);

string[] stringArray;
for (int i=0; i < 10000; i++)
{
    stringArray.push_back("hello " + i);
}

print("starting copy\n");
string[] copyOfStringArray = string[](stringArray);
print("finished copy\n");

for (int i=0; i < stringArray.size(); i += 1000)
{
    print(stringArray[i] + "\n");
}

float[][] marray;
float[] a;
a.push_back(1);
marray.push_back( a );
marray.push_back( a );

(vector float[4])[] varray;
varray.push_back(vec4f(0,0,0,0));

varray.front() = vec4f(1,2,3,4);

for (int i=0; i < 1000; i++)
{
    varray.push_back( vec4f(i,i,i,i) );
}

varray.back() = vec4f(4,3,2,1); 

for (int i=0; i < varray.size(); i += 100)
{
    print(varray[i] + "\n");
}

for (int i=varray.size(); i > 0; i--)
{
    vec4f v = varray.pop_back();
}


float[1] qq = float[1]();
qq[0] = 99.0;
print(qq[0] + "\n");

// this one is not using the declaration syntax -- its using the
// aggregate_value syntax
float[2][] ca;
ca = (float[2][] { {1, 2}, {3, 4}, {1, 2}, });
assert(ca.size() == 3);
assert(ca[0][1] == 2.0);
assert(ca[1][0] == 3.0);

// using declaration aggregate_initializer
float[4,4] R = {1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1};


vec4f[4] axis = { {0, 0, 0, 1},
                  {1, 0, 0, 1},
                  {0, 1, 0, 1},
                  {0, 0, 1, 1} };

for (int i=0; i < axis.size(); i++)
{
    print("axis[" + i + "] = " + axis[i] + "\n");
}

string[][] names = {
    { "foo", "bar", "baz", "bing", "bang", "bong", "caaaa" },
    { "and", "eat", "this" },
    {"yo", "yo", "yo"} };

int[] nums = { math.pi, math.e, 10 + 20, sin(1.23), names.size() };
assert(nums.back() == names.size());


float[2,2][][2][] jesush = 
{
    {
	{ {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, },
	{ {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5} },
    },

    {
	{ {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, },
	{ {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5} },
    },

    {
	{ {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, },
	{ {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5}, {1, 2, 4, 5} },
    },
};


function: takes_an_array(void; int[] array)
{
    for (int i=0; i < array.size(); i++)
    {
	print(array[i] + "\n");
    }
}

takes_an_array( int[] {1, 2, 3, 4} );


module: foo 
{ 
    class: test { int x; } 
}

\: x ()
{
    let x = foo.test[2](foo.test(1), foo.test(2)),
        y = foo.test[2]();
    let a = foo.test[](),
        b = foo.test[]();

    x == y;
    a == b;
    a eq b;
    x eq y;
}


x();
