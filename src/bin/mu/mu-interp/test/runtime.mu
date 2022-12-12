//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

let pname = runtime.lookup_name("+");
assert(pname == "+");

(int;int,int) plus = runtime.lookup_function(pname);
assert(plus(1,2) == 3);
