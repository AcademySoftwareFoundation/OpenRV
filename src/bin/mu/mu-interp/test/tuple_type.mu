//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

T := (int,string,float);            // tuple type created here

T foo = T.__allocate();             // literal allocator call
foo._0 = 1;                         // the fields of the tuple
foo._1 = "hello";                   //  are numbered
foo._2 = math.pi;

T bar = {11, "eleven", 11.11};      // default constructor
let baz = (12, "twelve", 12.12);    // tuple expression

let bigmess = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
               "one", "two", "three", "six", "ten",
               (vector float[3])(1, 2, 3),
               int[] {10, 8, -10},
               float[] {math.pi, math.e},
               T(1, "one", 1),
               float[4,4] {1, 0, 0, 0,
                           0, 1, 0, 0,
                           0, 0, 1, 0,
                           0, 0, 0, 1},
               \: (int n) { \: (int x) { x + n; }; },
               55);

assert(bigmess._0 == 1 && bigmess._21 == 55);

\: returns_tuple (float f, string s, int i) { (i, s, f); }

let as = (1, 2, 3),
    bf = 10,
    bm = returns_tuple(123.321, "one two three", 321);

assert(bf == 10);

