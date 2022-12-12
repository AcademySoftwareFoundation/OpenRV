//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
let a = 1,
    b = a + 2;

global let c = b,
    d = c + a + b;

assert(b == 3);
assert(d == 7);
