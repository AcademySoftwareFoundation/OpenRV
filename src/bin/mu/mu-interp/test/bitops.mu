//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

/// should print 1

int a = 1;
int b = 2;
int c = 3;

assert((a & b) == 0);
assert((a | b) == c);
assert((a & ~b) == a);
assert((a ^ c) == b);

