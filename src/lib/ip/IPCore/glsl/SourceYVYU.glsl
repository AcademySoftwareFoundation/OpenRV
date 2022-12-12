//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert packed 8 bit YUYV to RGB
//
//  NOTE: this doesn't really work because the Y's are being
//  interpolated incorreectly. If you use 2 channels you get the
//  opposite problem of the color components interpolating between U
//  and V but the Y is interpolated correctly.
//

vec4 SourceYVYU (const in sampler2DRect YVYU_sampler,
                 const in vec2 ST,
                 const in mat4 YUVmatrix,
                 const in vec2 offset,
                 const in vec2 YVYU_samplerSize,
                 const in float orientation)
{
    vec2 st = ST + offset;
    if (st.x < 0.0 || st.y < 0.0 || st.x >= YVYU_samplerSize.x || st.y >= YVYU_samplerSize.y)
        return vec4(0.0);

    if (orientation == 1.0) // tl
    {
        st.y = YVYU_samplerSize.y - st.y;
    }
    else if (orientation == 2.0) // br
    {
        st.x = YVYU_samplerSize.x - st.x;
    }
    else if (orientation == 3.0) // tr
    {
        st.x = YVYU_samplerSize.x - st.x;
        st.y = YVYU_samplerSize.y - st.y;
    }
    
    vec4 c = texture(YVYU_sampler, st);
    vec4 yuv = vec4(c.y, c.x, c.z, 1.0);
    return YUVmatrix * yuv;
}

