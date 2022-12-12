//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Basic functional mapping operations
//

Predicate := (bool;'a);

\: map ('a[]; ('a;'b) f, 'b[] array) 
{ 
    'a[] result;
    for_each (x; array) result.push_back(f(x)); 
    result;
}

\: map (void; (void;'b) f, 'b[] array) 
{ 
    for_each (x; array) f(x); 
}

\: reduce ('a; ('a;'a,'a) op, 'a[] array, 'a seed)
{
    let acc = seed;
    for_each (x; array) acc = op(acc, x);
    acc;
}

\: filter ('a[]; Predicate pred, 'a[] array)
{
    'a[] result;
    for_each (x; array) if (pred(x)) result.push_back(x);
    result;
}

operator: ! (Predicate; Predicate f)
{
    \: (bool; 'a x) { !f(x); };
}

operator: || (Predicate; Predicate f1, Predicate f2)
{
    \: (bool; 'a x) { f1(x) || f2(x); };
}

operator: && (Predicate; Predicate f1, Predicate f2)
{
    \: (bool; 'a x) { f1(x) && f2(x); };
}


map(print, string[] {"one\n", "two\n", "three\n"});
