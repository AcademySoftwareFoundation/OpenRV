//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  This test tries to compose random mu expressions and then evaluate
//  them. The expressions are limited to single and double argument 
//

use math_util;
use math;

F1 := (float;float);
F2 := (float;float,float);

global F1[] f1_funcs = 
{
    math.sin, math.cos, math.tan, math.sqrt, 
    math.cbrt, math.asin, math.acos, math.atan,
    (-)
};

global F2[] f2_funcs = 
{
    math.pow, math.atan2, (-), (*), (/), (%)
};


\: random_F1 () { f1_funcs[random(f1_funcs.size())]; }
\: random_F2 () { f2_funcs[random(f2_funcs.size())]; }

\: compose_F1 (F1 f)
{
    \: (F1 b, float x) { f(b(x)); };
}

\: compose_F2 (F2 f)
{
    \: (F1 b, float x, float y) { f(b(x), y); };
}

\: random_reduce_F2 (F2 f)
{
    (\: (float x, float y) { f(x, y); })(random(1.0),);
}

\: random_reduce_F1 (F1 f)
{
    f(random(1.0));
}
