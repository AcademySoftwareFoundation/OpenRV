//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Look up Y and A value from rect sampler.
//  Two channel image lookup
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity

vec4 SourceYA (const in sampler2DRect YA_sampler,
               const in vec2 ST,
               const in vec2 offset,
               const in vec2 YA_samplerSize,
               const in float orientation)
{
    vec2 t = ST + offset;
    if (t.x < 0.0 || t.y < 0.0 || t.x >= YA_samplerSize.x || t.y >= YA_samplerSize.y)
        return vec4(0.0); // black pixels with alpha 0

    if (orientation == 1.0) // tl
    {
        t.y = YA_samplerSize.y - t.y;
    }
    else if (orientation == 2.0) // br
    {
        t.x = YA_samplerSize.x - t.x;
    }
    else if (orientation == 3.0) // tr
    {
        t.x = YA_samplerSize.x - t.x;
        t.y = YA_samplerSize.y - t.y;
    }

    return texture(YA_sampler, t).rrra; // NOTE: swizzle here
}
