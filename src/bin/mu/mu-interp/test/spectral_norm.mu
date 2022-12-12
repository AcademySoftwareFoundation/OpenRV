//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
/* -*- mode: c -*-
 *
 * The Great Computer Language Shootout
 * http://shootout.alioth.debian.org/
 *
 * Contributed by Sebastien Loisel
 */

use math;

\: eval_A (float; int i, int j) { 1.0/((i+j)*(i+j+1)/2+i+1); }

\: eval_A_times_u (void; float[] u, float[] Au)
{
    let N = u.size();

    for (int i=0; i<N; i++)
    {
        Au[i] = 0;

        for (int j=0; j<N; j++) 
            Au[i] += eval_A(i,j) * u[j];
    }
}

\: eval_At_times_u (void; float[] u, float[] Au)
{
    let N = u.size();

    for (int i=0; i<N; i++)
    {
        Au[i] = 0;

        for (int j=0; j<N; j++) 
            Au[i] += eval_A(j,i) * u[j];
    }
}

\: eval_AtA_times_u (void; float[] u, float[] AtAu)
{
    let N = u.size();
    float[] v; 
    v.resize(N);

    eval_A_times_u(u, v); 
    eval_At_times_u(v, AtAu); 
}

\: start (void; int N)
{
    float[] u, v;
    float vBv = 0, vv = 0;

    u.resize(N);
    v.resize(N);

    for (int i=0; i<N; i++) u[i] = 1;

    repeat (10)
    {
        eval_AtA_times_u(u,v);
        eval_AtA_times_u(v,u);
    }

    for (int i=0; i<N; i++) 
    { 
        vBv += u[i] * v[i];
        vv  += v[i] * v[i]; 
    }

    print("%0.9f\n" % sqrt(vBv/vv));
}

start(200);
