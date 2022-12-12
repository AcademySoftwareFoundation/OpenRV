//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Assemble RGBA from Y RY BY planes (assumed to be float)
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity

vec4 SourcePlanarYAC2Uncrop (const in sampler2DRect YA_sampler,
                             const in sampler2DRect RYBY_sampler,
                             const in vec2 ST,
                             const in vec2 ratio0,
                             const in vec3 Yweights,
                             const in vec2 offset,
                             const in vec2 YA_samplerSize,
                             const in vec2 uncropOrigin,
                             const in vec2 uncropDimension,
                             const in float orientation)
{
    //vec2 ratio = vec2((RYBY_coord + 1e-6) / (YA_coord + 1e-6));
    vec2 st = ST + offset;
    
    if (orientation == 1.0) // tl
    {
        st.y = YA_samplerSize.y - st.y;
    }
    else if (orientation == 2.0) // br
    {
        st.x = YA_samplerSize.x - st.x;
    }
    else if (orientation == 3.0) // tr
    {
        st.x = YA_samplerSize.x - st.x;
        st.y = YA_samplerSize.y - st.y;
    }
    
    if (uncropOrigin.x != 0.0 || uncropOrigin.y != 0.0 ||
        uncropDimension.x != YA_samplerSize.x || uncropDimension.y != YA_samplerSize.y)
    {
        float x = st.x / YA_samplerSize.x;
        float y = st.y / YA_samplerSize.y;
        float newx = uncropDimension.x * x - uncropOrigin.x; 
        float newy = uncropDimension.y * y - uncropOrigin.y;								  
        vec2 adjustedTexCoord = vec2(newx, newy);
        if (adjustedTexCoord.x < 0.0 || adjustedTexCoord.y < 0.0 || 
            adjustedTexCoord.x >= YA_samplerSize.x || adjustedTexCoord.y >= YA_samplerSize.y)
            return vec4(0.0); // black pixels with alpha 0

        vec2 YA = texture(YA_sampler, adjustedTexCoord).xw; 
        vec2 C2 = texture(RYBY_sampler, adjustedTexCoord * ratio0).xw;
        float Y = YA.x;
        float r = C2.x * Y + Y;
        float b = C2.y * Y + Y;
        float g = (Y - r * Yweights.r - b * Yweights.b) /Yweights.g;
        return vec4(r, g, b, YA.y);
    }

    vec2 YA = texture(YA_sampler, st).xw; 
    vec2 C2 = texture(RYBY_sampler, st * ratio0).xw;
    float Y = YA.x;
    float r = C2.x * Y + Y;
    float b = C2.y * Y + Y;
    float g = (Y - r * Yweights.r - b * Yweights.b) /Yweights.g;
    return vec4(r, g, b, YA.y);
}

