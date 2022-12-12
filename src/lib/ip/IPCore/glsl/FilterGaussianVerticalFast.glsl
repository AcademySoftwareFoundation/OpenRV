//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 FilterGaussianVerticalFast (const in inputImage in0,
                                 const in sampler2DRect RGB_sampler,
                                 const in float radius,
                                 const in outputImage win)
{
    vec2 size = win.size();
    vec2 st = win.xy;
    float tw = 0.0;
    vec3 c = vec3(0.0, 0.0, 0.0);
    // compute two sides
    float up = radius;
    if (st.y < radius) up = floor(st.y);
    float dow = radius;
    if (st.y + radius > size.y) dow = floor(size.y - st.y);
    up = floor(up / 2.0);
    dow = floor(dow / 2.0);
    float r;
    for (r = -up; r <= dow; r += 1.0)
    {
        vec4 fastd = texture(RGB_sampler, vec2(abs(r), 0.0));
        float w = fastd.y;
        float o = r >= 0.0 ? fastd.x : -fastd.x;
        c += w * in0(vec2(0.0, o)).rgb;
        tw += w;
    }
    c /= tw;
    return vec4(c, in0().a);
}

