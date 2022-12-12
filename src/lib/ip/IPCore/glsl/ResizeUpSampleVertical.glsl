//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//hermite, b-spline order 3
vec4 ResizeUpSampleVertical (const in inputImage in0,
                             const in outputImage win)
{
    vec4 res = vec4(0.0, 0.0, 0.0, 0.0);
    vec2 xy = win.xy;
    float ty = 0.5 * xy.y - 0.25;
    float rem = ty - float(int(ty));
    float rem1 = 1.0 - rem;
    float w = 2.0 * rem * rem * rem - 3.0 * rem * rem + 1.0;
    float w1 = 2.0 * rem1 * rem1 * rem1 - 3.0 * rem1 * rem1 + 1.0;
    res += w * in0(vec2(0.0, -rem)) + w1 * in0(vec2(0.0, rem1));
    res /= w +  w1;
    return res;
}

