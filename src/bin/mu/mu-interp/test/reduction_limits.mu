//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  This is testing some difficult aspects of function and expression
//  reduction. Reduction here is refering to reduction during the
//  parse and/or compile phase not during "run" time. The goal is to
//  reduce as much as possible as early as possible. The earlier a
//  reduction occurs the faster the parse/compile phase will be
//  (because you minimize the number of times you need to scan an
//  expression -- at least that's the working theory).
//
//  For function reduction, an indirect call must be reduced to a
//  direct call. For constant reduction, some functions, namely g
//  below, are "maybe pure" and others like f are "pure". maybe pure
//  functions are pure only if any function arguments are pure or
//  maybe pure. maybe pure functions exist because they are applying
//  one of their function arguments. This application is what makes
//  the purity of the function unknown.
//
//  To see the results you need to run this in the interpreter with
//  -printFunc to dump the reduced function.
//

use math_util;
F := (int; int, int);

\: e (int a, int b) { seed(a); random(b); }             // not pure
\: f (int a, int b) { int(a + b + math.sin(a + b)); }   // pure
\: g (F u, int x)   { u(u(x, u(x,1)),1); }              // maybe pure

//
// This should reduce to the constant 14 because g is maybe pure, f is
// pure, and the rest in constant.
//

g(f, f(1,2) + f(2,1));  

//
//  This should never reduce because although e is constant it is not pure
//  and since g is maybe pure it cannot be reduced.
//

g(e, f(1,2) + f(2,1));  // not reduce because e is not pure

//
//  Same exersize, but with non-primitive type
//  

S := (string; string, string);

\: s (string a, string b) { b + a; }                    // pure
\: np (string a, string b) { print(""); a; }            // not pure
\: t (S u, string x) { u(u(x, u(x, "A")), "B"); }       // maybe pure

t(s, s("E", "F") + s("G", "H"));    // reduce to constant "BAFEHGFEHG" 
t(np, s("E", "F") + s("G", "H"));   // do not reduce (use -print to see if it does)

