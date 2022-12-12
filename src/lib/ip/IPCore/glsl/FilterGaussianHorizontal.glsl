//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 FilterGaussianHorizontal (const in inputImage in0,
                               const in float radius,
                               const in float sigma,
                               const in outputImage win)
{
    vec2 size = win.size();
    vec2 st = win.xy;
    float tw = 0.0;
    vec3 c = vec3(0.0, 0.0, 0.0);
    // compute two sides
    float lef = radius;
    if (st.x < radius) lef = floor(st.x);
    float rig = radius;
    if (st.x + radius > size.x) rig = floor(size.x - st.x);
    float r;
    for (r = -lef; r <= rig; r += 1.0)
    {
        float w = exp(-r * r * sigma); // note sigma is 1 / 2 * sigma^2 in gaussian equation
        c += w * in0(vec2(r, 0.0)).rgb;
        tw += w;
    }
    c /= tw;
    return vec4(c, in0().a);
}
