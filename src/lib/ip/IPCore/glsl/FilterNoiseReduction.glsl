//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// i0 is the original
// i1 is the gaussian blurred
vec4 FilterNoiseReduction(const in vec4 i0,
                          const in vec4 i1,
                          const in float amount,
                          const in float threshold)
{
    vec4 c = i0 - i1;
    vec4 cabs = abs(c);
    vec3 res;
    if (threshold < cabs.r) res.r = c.r * amount + i1.r;
    else res.r = i0.r;
    if (threshold < cabs.g) res.g = c.g * amount + i1.g;
    else res.g = i0.g;
    if (threshold < cabs.b) res.b = c.b * amount + i1.b;
    else res.b = i0.b;
    return vec4(res, i0.a);

 }
