//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  RGBA from YUVA
//

vec4 SourceAYUV (const in sampler2DRect YUVA_sampler,
                 const in vec2 ST,
                 const in mat4 YUVmatrix,
                 const in vec2 offset,
                 const in vec2 YUVA_samplerSize,
                 const in float orientation)
{
    vec2 st = ST + offset;
    if (st.x < 0.0 || st.y < 0.0 || st.x >= YUVA_samplerSize.x || st.y >= YUVA_samplerSize.y)
        return vec4(0.0); // black pixels with alpha 0
    
    if (orientation == 1.0) // tl
    {
        st.y = YUVA_samplerSize.y - 1.0 - st.y;
    }
    else if (orientation == 2.0) // br
    {
        st.x = YUVA_samplerSize.x - 1.0 - st.x;
    }
    else if (orientation == 3.0) // tr
    {
        st.x = YUVA_samplerSize.x - 1.0 - st.x;
        st.y = YUVA_samplerSize.y - 1.0 - st.y;
    }
    
    vec4 yuva = texture(YUVA_sampler, st).yzwx; // swizzle
    return vec4((YUVmatrix * vec4(yuva.xyz, 1.0)).rgb, yuva.a);
}

