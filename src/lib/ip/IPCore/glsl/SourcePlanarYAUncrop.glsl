//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Look up Y and A values from rect samplers.
//  Two channel image lookup
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity

vec4 SourcePlanarYAUncrop (const in sampler2DRect Y_sampler, 
                           const in sampler2DRect A_sampler,
                           const in vec2 ST,
                           const in vec2 offset,
                           const in vec2 Y_samplerSize,
                           const in vec2 uncropOrigin,
                           const in vec2 uncropDimension,
                           const in float orientation)
{
    vec2 st = ST + offset;
    
    if (orientation == 1.0) // tl
    {
        st.y = Y_samplerSize.y - st.y;
    }
    else if (orientation == 2.0) // br
    {
        st.x = Y_samplerSize.x - st.x;
    }
    else if (orientation == 3.0) // tr
    {
        st.x = Y_samplerSize.x - st.x;
        st.y = Y_samplerSize.y - st.y;
    }
    
    if (uncropOrigin.x != 0.0 || uncropOrigin.y != 0.0 ||
        uncropDimension.x != Y_samplerSize.x || uncropDimension.y != Y_samplerSize.y)
    {
        float x = st.x / Y_samplerSize.x;
        float y = st.y / Y_samplerSize.y;
        float newx = uncropDimension.x * x - uncropOrigin.x; 
        float newy = uncropDimension.y * y - uncropOrigin.y;								  
        vec2 adjustedTexCoord = vec2(newx, newy);
        if (adjustedTexCoord.x < 0.0 || adjustedTexCoord.y < 0.0 || 
            adjustedTexCoord.x >= Y_samplerSize.x || adjustedTexCoord.y >= Y_samplerSize.y)
            return vec4(0.0); // black pixels with alpha 0

        return vec4(texture(Y_sampler, adjustedTexCoord).rrr, // NOTE: swizzle here
                    texture(A_sampler, adjustedTexCoord).r); 
    }
    return vec4(texture(Y_sampler, st).rrr, // NOTE: swizzle here
                texture(A_sampler, st).r); 
}

