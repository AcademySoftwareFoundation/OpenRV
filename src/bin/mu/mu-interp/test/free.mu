//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  This is a test of closure generation
//
//  Note: these function definitions are purposefully omitting the
//  return type in order to work the inference mechanism.
//

int n = 10;

//
//  Free variable "n" will be bound to top scope "n" if called from the top
//  scope.
//
//  addn signature is: (int;int)

\: addn (int x) { x + n; }



//
//  Free variable n in internal lambda expression will be bound to the
//  parameter passed into adder().
//
//  adder signature is: ((int;int);int)

\: adder (int n) { \: (int x) { x + n; }; }

//
//  The internal function is bound and is returned.  The return expression
//  is a lambda expression: so the free variable is captured at that time.
//
//  adder_literal signature is: ((int;int);int)

\: adder_literal (int n) { \: F (int x) { x + n; }; F; }

//
//  I'm not clear as to whether this breaks the definition of closure
//  in scheme.  But it makes sense in the context of Mu's
//  implementation of a "closure" (free variables are hoisted to named
//  parameters which automatically match symbols in the context of a
//  call or function object creation).
//
//  adder_literal2 signature is: ((int;int);int)

\: adder_literal2 (int n) { addn; } // addn will bind the local "n"

assert(addn(2) == 12);
assert(adder(11)(9) == 20);
assert(adder(10)(9) == 19);
assert(adder_literal(3)(10) == adder(3)(10));
assert(adder_literal2(4)(3) == adder_literal(4)(3));

//
//  Something a bit more freaky. In this case, n is doubled inside the
//  function locally. Since the addn lambda expression occurs *after* the
//  doubling, the addn function binds 2 * n instead of n.
//
//  adder_double signature is: ((int;int);int)

\: adder_double (int n) { n *= 2; addn; }

assert(adder_double(2)(2) == 6);

//
//  Sanity check. addn will use the global value 10 in this case
//

// \: adder_10 (int q) { addn; }
// adder_10(123);


//
//  Here's function composition (as in compose.mu) using closures
//
//  F1 is a type of "takes a float returns a float"
//  F2 is a type of "takes two floats returns a float"
//
//  compose signatures: ((float;float);(float;float))
//                      ((float;float,float);(float;float),(float;float,float))

F1 := (float;float);         
F2 := (float;float,float); 

\: compose (F1 a) { \: (float x) { a(x); }; }
\: compose (F1 a, F2 b) { \: (float x, float y) { b(a(x), y); }; }

assert(compose(math.sin)(.123) == math.sin(.123));
assert(compose(math.sin, math.atan2)(.123, .321) == 
       math.atan2(math.sin(.123), .321));

compose(math.sin, math.atan2);

//
//  Why is this interesting?
//
//  quadruple actually contains a free variable (double)
//  which is bound to the outside double when quadruple(2)
//  is called! So it actually computes the correct value
//  (as if this were ML).
//

let doubleIt    = \: (int x) { x + x; };
let quadrupleIt = \: (int x) { doubleIt(x); };
quadrupleIt(2);

print(string(quadrupleIt) + "\n");
