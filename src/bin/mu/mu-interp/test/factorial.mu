//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  An attempt at getting tail recursion working
//

\: factorial_recursive (int; int x)
{
    if x == 1 then 1 else x * factorial_recursive(x-1);
}

\: factorial_tail_recursive (int; int n)
{
    \: iterate (int; int n, int acc)
    {
        if n <= 1 then acc else iterate(n-1, acc*n);
    }

    return iterate(n, 1);
}

int n = 15;
print(factorial_tail_recursive(n) + "\n");
assert(factorial_recursive(n) == factorial_tail_recursive(n));

(int; int) factorial_type_function = nil;
factorial_type_function = factorial_recursive;
factorial_type_function = factorial_tail_recursive;
(int; int) other_func = factorial_type_function;
