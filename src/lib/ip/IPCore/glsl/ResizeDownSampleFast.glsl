//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// This only works for scale being even, it will utilize gl bilinear interp
// to reduce number of samples
vec4 ResizeDownSampleFast (const in inputImage in0,
                           const in float scale,
                           const in outputImage win)
{
    vec4 res = vec4(0.0, 0.0, 0.0, 0.0);
    float i, j;
    for (i = 0.0; i < scale; i+=2.0)
        for (j = 0.0; j < scale; j+=2.0)
            res += in0(vec2(j + 0.5, i + 0.5));
    float w = scale * scale / 4.0;
    res /= w;
    return res;
}

