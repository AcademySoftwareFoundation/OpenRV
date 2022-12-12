//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

F     := (float; float);
BinOp := (float;float,float);

\: ident (float a) { a; }
\: square (float a) { a * a; }

operator: - (F f) { \: (float a) { -f(a); }; }

\: op (F fa, F fb, BinOp o)
{
    \: (float a, float b) { o(fa(a),fb(b)); };
}

// core dumps if floats, not if ints
//operator: + (F fa, F fb) { op(fa, fb, (+)); }

operator: + (BinOp; F fa, F fb) { op(fa, fb, (+)); }

\: foo (F a, F b) { a + -b; }

//
//  Why is this not reducing completely?
//  there is still a dynamic thunk call in there.
//

-(-math.sin);//(1,2);
//foo(ident, square);
