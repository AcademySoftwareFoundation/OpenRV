//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Look up Y value from rect sampler. 
//  Single channel image lookup
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity

vec4 SourceYUncrop (const in sampler2DRect Y_sampler,
                    const in vec2 ST,
                    const in vec2 offset,
                    const in vec2 Y_samplerSize,
                    const in vec2 uncropOrigin,
                    const in vec2 uncropDimension,
                    const in float orientation)
{
    vec2 t = ST + offset;

    if (orientation == 1.0) // tl
    {
        t.y = Y_samplerSize.y - t.y;
    }
    else if (orientation == 2.0) // br
    {
        t.x = Y_samplerSize.x - t.x;
    }
    else if (orientation == 3.0) // tr
    {
        t.x = Y_samplerSize.x - t.x;
        t.y = Y_samplerSize.y - t.y;
    }

    if (uncropOrigin.x != 0.0 ||
        uncropOrigin.y != 0.0 ||
        uncropDimension.x != Y_samplerSize.x || 
        uncropDimension.y != Y_samplerSize.y)
    {
        float x    = t.x / Y_samplerSize.x;
        float y    = t.y / Y_samplerSize.y;
        float newx = uncropDimension.x * x - uncropOrigin.x; 
        float newy = uncropDimension.y * y - uncropOrigin.y;								  
        vec2  nt   = vec2(newx, newy);

        if (nt.x < 0.0 || 
            nt.y < 0.0 || 
            nt.x >= Y_samplerSize.x || 
            nt.y >= Y_samplerSize.y)
        {
            return vec4(0.0); // black pixels with alpha 0
        }
        else
        {
            return texture(Y_sampler, nt).rrra;
        }
    }

    return texture(Y_sampler, t).rrra; // NOTE: swizzle here
}

