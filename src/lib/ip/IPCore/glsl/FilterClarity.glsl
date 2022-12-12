//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// i0 is the original
// i1 is the gaussian blurred
// mid tone is considered as 0.25-0.75
vec4 FilterClarity(const in vec4 i0,
                   const in vec4 i1,
                   const in float amount)
{
    float l = 0.299 * i0.r + 0.587 * i0.g + 0.114 * i0.b;
    float newAmount = amount;
    if (l > 0.75)
    {
        float w = 1.0 - min(0.25, l - 0.75) / 0.25;
        newAmount = amount * w;
    }
    if (l < 0.25)
    {
        float w = 1.0 - min(0.25, 0.25 - l) / 0.25;
        newAmount = amount * w;
    }
    vec4 c = i0 - i1;
    vec3 res;
    res.r = c.r * newAmount + i0.r;
    res.g = c.g * newAmount + i0.g;
    res.b = c.b * newAmount + i0.b;
    return vec4(res, i0.a);
}
