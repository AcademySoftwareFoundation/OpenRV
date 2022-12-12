//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Compute a gaussian kernel
//

int n = 5;
float[5,5] kernel;
float accum = 0.0;

\: g (float x) { math.exp(-(x * x)); }

for (int i=0; i < n; i++)
{
    for (int j=0; j < n; j++)
    {
        let xd = float(i) / (n - 1.0) - 0.5,
            yd = float(j) / (n - 1.0) - 0.5,
            d  = math.sqrt(xd * xd + yd * yd),
            ga = g(d * 2.5);

        kernel[i,j] = ga;
        accum += ga;
    }
}

for (int i=0; i < n; i++)
{
    for (int j=0; j < n; j++)
    {
        kernel[i,j] /= accum;
    }
}

kernel;
