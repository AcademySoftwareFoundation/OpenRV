//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
\: foo (void;)
{
    let list = [1,2,3];
    \: bar (void;) { for_each (l; list) print(l); }
    \: baz (void;) { bar(); }

    baz();
}

// should print "123"
foo();
