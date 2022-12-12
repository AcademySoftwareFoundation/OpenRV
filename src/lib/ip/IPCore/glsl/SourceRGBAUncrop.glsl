//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Look up RGBA value from rect sampler
//

vec4 SourceRGBAUncrop (const in sampler2DRect RGBA_sampler,
                       const in vec2 ST,
                       const in vec2 offset,
                       const in vec2 samplerSize,
                       const in vec2 uncropOrigin,
                       const in vec2 uncropDimension,
                       const in float orientation)
{
    vec2 texCoord = ST + offset;

    if (orientation == 1.0) // tl
    {
        texCoord.y = samplerSize.y - texCoord.y;
    }
    else if (orientation == 2.0) // br
    {
        texCoord.x = samplerSize.x - texCoord.x;
    }
    else if (orientation == 3.0) // tr
    {
        texCoord.x = samplerSize.x - texCoord.x;
        texCoord.y = samplerSize.y - texCoord.y;
    }

    // apply transformation on the texCoord to obtain the proper texCoord if necessary
    if (uncropOrigin.x != 0.0 || 
        uncropOrigin.y != 0.0 ||
        uncropDimension.x != samplerSize.x || 
        uncropDimension.y != samplerSize.y)
    {
        float x = texCoord.x / samplerSize.x;
        float y = texCoord.y / samplerSize.y;
        float newx = uncropDimension.x * x - uncropOrigin.x; 
        float newy = uncropDimension.y * y - uncropOrigin.y;								  
        vec2 adjustedTexCoord = vec2(newx, newy);
        if (adjustedTexCoord.x < 0.0 || adjustedTexCoord.y < 0.0 || 
            adjustedTexCoord.x >= samplerSize.x || adjustedTexCoord.y >= samplerSize.y)
            return vec4(0.0);
        return texture(RGBA_sampler, adjustedTexCoord);
    }
    else
    {
        return texture(RGBA_sampler, texCoord);		
    }
}
