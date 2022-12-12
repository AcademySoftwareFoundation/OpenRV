//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Look up RGBA value from rect sampler
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity
vec4 SourceRGBA (const in sampler2DRect RGBA_sampler,
                 const in vec2 ST,
                 const in vec2 offset,
                 const in vec2 RGBA_samplerSize,
                 const in float orientation)
{
    vec2 t = ST + offset;
    if (t.x < 0.0 || t.y < 0.0 || t.x >= RGBA_samplerSize.x || t.y >= RGBA_samplerSize.y)
        return vec4(0.0); // black pixels with alpha 0

    if (orientation == 1.0) // tl
    {
        t.y = RGBA_samplerSize.y - t.y;
    }
    else if (orientation == 2.0) // br
    {
        t.x = RGBA_samplerSize.x - t.x;
    }
    else if (orientation == 3.0) // tr
    {
        t.x = RGBA_samplerSize.x - t.x;
        t.y = RGBA_samplerSize.y - t.y;
    }
    
    return texture(RGBA_sampler, t);
}
