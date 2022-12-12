//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Assemble RGBA from Y and UV planes
//

vec4 SourcePlanar2YUV (const in sampler2DRect Y_sampler,
                       const in sampler2DRect UV_sampler,
                       const in vec2 ST,
                       const in vec2 ratio0,
                       const in mat4 YUVmatrix,
                       const in vec2 offset,
                       const in vec2 Y_samplerSize,
                       const in float orientation)
{
    vec2 st = ST + offset;
    if (st.x < 0.0 || st.y < 0.0 || st.x >= Y_samplerSize.x || st.y >= Y_samplerSize.y)
        return vec4(0.0); // black pixels with alpha 0

    if (orientation == 1.0) // tl
    {
        st.y = Y_samplerSize.y - 1.0 - st.y;
    }
    else if (orientation == 2.0) // br
    {
        st.x = Y_samplerSize.x - 1.0 - st.x;
    }
    else if (orientation == 3.0) // tr
    {
        st.x = Y_samplerSize.x - 1.0 - st.x;
        st.y = Y_samplerSize.y - 1.0 - st.y;
    }

    vec2 uv = texture(UV_sampler, st * ratio0).ra;
    float y = texture(Y_sampler, st).r;
    return YUVmatrix * vec4(y, uv.x, uv.y, 1.0 );
}

