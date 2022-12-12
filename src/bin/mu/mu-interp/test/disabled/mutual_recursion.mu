//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  This test is mainly useful for determining if unresolved calls are
//  handled properly. In this case, mutual recursion needs to be
//  resolved. This example prints out a random mu expression (idea
//  from something I saw on the web when looking up mutual recursion).
//

use io;
use math_util;
use math;

global string[] ops   = {" + ", " - ", " * ", " / ", " % "};
global string[] funcs = { "math.sin", "math.cos", "math.tan", "math.sqrt", "math.pow",
                          "math.asin", "math.acos", "math.atan", "math.cbrt" };

\: infixExpression (void; ostream out, int depth)
{
    print(out, "(");
    randomExpression(out, depth);
    print(out, ops[random(ops.size())]);
    randomExpression(out, depth);
    print(out, ")");
}

\: callExpression (void; ostream out, int depth)
{
    print(out, funcs[random(funcs.size())] + "(");
    randomExpression(out, depth);
    print(out, ")");
}

\: randomExpression (void; ostream out, int depth)
{
    let r = random(10);

    if (r < depth)
    {
        if (r % 4 == 0) callExpression(out, depth-1);
        else infixExpression(out, depth-1);
    }
    else
    {
        print(out, string(random(1.0)));
    }
}

\: doit (void; int level) 
{
    randomExpression(out(), level); 
    print(out(), "\n");
}

for (int i=0; i < 25; i++)
{
    seed(2239 + i);
    doit(7);
}
