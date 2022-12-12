//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//mitchell, b-spline order 3, B = C = 1/3
vec4 ResizeUpSampleHorizontalMitchell (const in inputImage in0,
                                       const in outputImage win)
{
    vec4 res = vec4(0.0, 0.0, 0.0, 0.0);
    vec2 xy = win.xy;
    float tx = 0.5 * xy.x - 0.25;
    float rem = tx - float(int(tx));
    float rem1 = 1.0 - rem;
    float rem2 = rem + 1.0;
    float rem3 = rem1 + 1.0;
    float w = 7.0 * rem * rem * rem - 12.0 * rem * rem + 6.0 - 2.0 / 3.0;
    float w1 = 7.0 * rem1 * rem1 * rem1 - 12.0 * rem1 * rem1 + 6.0 - 2.0 / 3.0;
    float w2 = -7.0 * rem2 * rem2 * rem2 / 3.0 + 12.0 * rem2 * rem2 - 20.0 * abs(rem2) + 32.0 / 3.0;
    float w3 = -7.0 * rem3 * rem3 * rem3 / 3.0 + 12.0 * rem3 * rem3 - 20.0 * abs(rem3) + 32.0 / 3.0;
    res += w * in0(vec2(-rem, 0.0)) + w1 * in0(vec2(rem1, 0.0))
           + w2 * in0(vec2(-rem2, 0.0)) + w3 * in0(vec2(rem3, 0.0));
    res /= w +  w1 + w2 + w3;
    return res;
}

