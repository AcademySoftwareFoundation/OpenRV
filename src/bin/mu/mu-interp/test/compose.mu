//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Let's say I have x of type (float;float) and y of type
//  (float;float,float) and I want z == y(x(_1),_2)
//
//  NOTE: these examples purposefully avoid using closures to achieve
//  the same effect. See free.mu for the same examples using closures.
//

F1 := (float;float);         
F2 := (float;float,float); 

\: compose (F1 a, F2 b)
{
    (\: (float x, float y, F1 a, F2 b) { b(a(x), y); })(,,a,b);
}

//
//  F should be:
//
//      \: (float x, float y) { math.atan2(math.sin(x), y); }
//
//  after function reduction.
//

require math;
let F = compose(math.sin, math.atan2);
print(string(F) + "\n");

//
//  This statement *should* reduce to: assert(true) since F is
//  constant and additional arguments are also constant. Alternately,
//  if F is bound to the anonymous function as a function symbol, it
//  would be a straightforward call constant reduction.
//

assert(F(1.0, 2.0) == math.atan2(math.sin(1.0), 2.0));

