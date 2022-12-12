//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Test function definitions
//

function: logN (float; float x, float base) 
{
    math.log(x) / math.log(base);
};

function: logNCheck (bool;)
{
    logN(2.0, 3.0) == math.log(2.0) / math.log(3.0);
}

assert( logNCheck() );

function: identity (float; float a) { a; }

float q = 4.0;

assert(identity(q) == 4.0);

function: sum (vector float[4]; 
               vector float[4] a, 
               vector float[4] b, 
               vector float[4] c, 
               vector float[4] d)
{
    a + b + c + d;
}

vector float[4] v0 = vector float[4](1,0,0,0);
vector float[4] v1 = vector float[4](0,1,0,0);
vector float[4] v2 = vector float[4](0,0,1,0);
vector float[4] v3 = vector float[4](0,0,0,1);

assert( sum(v0, v1, v2, v3) == vector float[4](1,1,1,1) );

//
//  Recursion
//

function: factorial (int; int x)
{
    if x == 1 then 1 else x * factorial(x-1);
}

assert( factorial(10) == (10 * 9 * 8 * 7 * 6 * 5 * 4 * 3 * 2) );

//
//  function in the scope of another function
//

function: foo (float; float x, float y, float z)
{
    function: bar(float; float x, float y) { x / y; }
    bar(x, y) * bar(x, z);
}

assert( foo(11.0, 22.0, 44.0) == ((11.0 / 22.0) * (11.0 / 44.0)) );

//
//  Recursive function calling recursive function
//

function: foo (float; float x)
{
    if x == 2 then foo(1.0, 2.0, 3.0) + foo(x - 1.0) else factorial(5);
}

assert( int(foo(2.0)) == 120 );

//
//  Unspecified return type forms
//

function: unspecifiedReturn_logN (float x, float base) 
{
    math.log(x) / math.log(base);
};

function: unspecifiedReturn_logNCheck ()
{
    unspecifiedReturn_logN(2.0, 3.0) == math.log(2.0) / math.log(3.0);
}
