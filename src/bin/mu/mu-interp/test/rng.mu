//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

IM := 139968;
IA := 3877;
IC := 29573;
global int LAST = 42;

\: gen_random (float; float x)
{
    LAST = (LAST * IA + IC) % IM;
    (x * float(LAST)) / float(IM);
}

\: doit (float; int N)
{
    if (N < 1) N = 1;
    repeat (N-1) gen_random(100.0);
    gen_random(100.0);
}

doit(1000000);
