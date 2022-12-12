//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Case with predicate
//

case (re, regex.match)
{
    "([0-9]+-[0-9]+)-[0-9]+"    ->  { ... }
    ".*"                        ->  { print("matched anything\n"); }
}

//
// case_test
//     patten_test (test, body)
//     patten_test (body)
//     patten_test (test, body)
//     patten_test (test, body)
//     patten_test (body)
//
