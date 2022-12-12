//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 FilterGaussianHorizontalFast (const in inputImage in0,
                                   const in sampler2DRect RGB_sampler,
                                   const in float radius,
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
    lef = floor(lef / 2.0);
    rig = floor(rig / 2.0);
    float r;
    for (r = -lef; r <= rig; r += 1.0)
    {
        vec4 fastd = texture(RGB_sampler, vec2(abs(r), 0.0));
        float w = fastd.y;
        float o = r >= 0.0 ? fastd.x : -fastd.x;
        c += w * in0(vec2(o, 0.0)).rgb;
        tw += w;
    }
    c /= tw;
    return vec4(c, in0().a);
}
