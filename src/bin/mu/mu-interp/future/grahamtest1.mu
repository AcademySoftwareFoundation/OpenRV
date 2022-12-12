//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

// The problem: Write a function foo that takes a number n and returns a 
// function that takes a number i, and returns n incremented by i.

\: foo ('a n)
{
    \: ('a i) { n + i; };
}

foo(1);
