//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// from http://inferno.bell-labs.com/cm/cs/who/bwk/interps/pap.html
//
// The next program evaluates Ackermann's function. Computing ack(3,k) =
// 2^(k+3)-3 requires at least 4^(k+1) function calls, and reaches a
// recursive depth of 2^(k+3)-1, so this test gives the function call
// mechanism a thorough workout.
//
// This will fail for a large enough k that the C stack is exhausted. So if
// its not run in the main thread it will fail when the pthread stack limit
// is hit.
//
// The equivalent python program will throw at k == 7 because of recursion
// limits. The equivalent perl program is approx the same speed as python,
// but does not die. The original paper tested up to k == 8.
//

function: ack (int; int m, int n) 
{
    if m == 0 then (n+1) else (if n == 0 then ack(m-1, 1) else ack(m-1, ack(m, n-1)));
}

function: ack_long (int; int m, int n) 
{
    if (m == 0)
    {
        return n + 1;
    }
    else
    {
        if (n == 0)
        { 
            return ack_long(m-1, 1);
        }
        else
        {
            return ack_long(m-1, ack_long(m, n-1));
        }
    }
}


int k = 8; 
//assert(ack(3,k) == math.pow(2, (k+3)) - 3);
//assert(ack_long(3,k) == ack(3,k));
ack(3,k);

