//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

use test;

//
//  Test interfaces
//

int[] x = {1, 2, 3};
sequence c = x;
c.clear();
c.push_back(3);
c.push_back(1);
c.push_back(2);
c.pop_back();
c.push_back(10);
assert(x == int[] {3, 1, 10});
c;
