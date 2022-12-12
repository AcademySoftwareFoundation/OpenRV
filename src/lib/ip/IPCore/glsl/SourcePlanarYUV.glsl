//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Assemble RGBA from Y U V planes
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity

vec4 SourcePlanarYUV (const in sampler2DRect Y_sampler,
                      const in sampler2DRect U_sampler,
                      const in sampler2DRect V_sampler,
                      const in vec2 ST,
                      const in vec2 ratio0,
                      const in vec2 ratio1,
                      const in mat4 YUVmatrix,
                      const in vec2 offset,
                      const in vec2 Y_samplerSize,
                      const in float orientation)
{
    //vec2 uratio = vec2((U_coord + 1e-6) / (Y_coord + 1e-6));
    //vec2 vratio = vec2((V_coord + 1e-6) / (Y_coord + 1e-6));
    vec2 st = ST + offset;
    if (st.x < 0.0 || st.y < 0.0 || st.x >= Y_samplerSize.x || st.y >= Y_samplerSize.y)
        return vec4(0.0); // black pixels with alpha 0

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
    
    return YUVmatrix * vec4( texture(Y_sampler, st).r,
                             texture(U_sampler, st * ratio0).r,
                             texture(V_sampler, st * ratio1).r,
                             1.0 );
}

