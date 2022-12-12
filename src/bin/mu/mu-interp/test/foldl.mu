//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
A := int;
B := int;
Op := (A;A,B);

\: foldl (A; Op f, A seed, [B] l)
{
    if (l eq nil)
    {
        return seed;
    }
    else
    {
        let h:t = l;
        return foldl(f, f(h, seed), t);
    }
}

\: filter ([A]; (bool;A) p, [A] l)
{
    if (l eq nil)
    {
        return nil;
    }
    else
    {
        let h:t = l;

        if (p(h))
        {
            return h : filter(p, t);
        }
        else
        {
            return filter(p,t);
        }
    }
}

\: even (bool; int a) { return (a & 1) == 0; }

foldl((+), 0, [1,2,3,4]);

filter(even, [1,2,3,4]);
