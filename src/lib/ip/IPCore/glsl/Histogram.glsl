//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 Histogram (const in inputImage in0,
                const in outputImage win)
{
    vec2 size = win.size();
    float y   = in0.st.y;
    vec3 h    = in0(vec2(0.0, -y)).rgb; // 0 - 1
    vec3 nh   = sqrt(h) * size.y * 2.0; // beautify, get to 0 - size.y

    vec4 c = vec4(0.0, 0.0, 0.0, 0.0);

    if (y < nh.r) { c.r = 1.0; c.a = 1.0; }
    if (y < nh.g) { c.g = 1.0; c.a = 1.0; }
    if (y < nh.b) { c.b = 1.0; c.a = 1.0; }

    return c == vec4(1,1,1,1) ? vec4(.6, .6, .6, 1) : c;
}

