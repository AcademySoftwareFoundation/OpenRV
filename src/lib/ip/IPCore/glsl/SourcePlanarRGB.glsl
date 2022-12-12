//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// Assemble RGBA from R G B planes.
//
// G is assumed to have the "master" coordinates. I.e. G will always
// have the highest resolution (but may be the same as R and B)
//
// TODO make it work with other resolution R and B
//

//  NOTE if we are in gl 3.0+ we can do textureSize on the sampler and
//  will not need to pass it in. this is just for compatiblity

vec4 SourcePlanarRGB (const in sampler2DRect R_sampler,
                      const in sampler2DRect G_sampler,
                      const in sampler2DRect B_sampler,
                      const in vec2 ST,
                      const in vec2 offset, 
		              const in vec2 R_samplerSize,
                      const in float orientation)
{
    vec2 st = ST + offset;

    if (st.x < 0.0 || st.y < 0.0 || st.x >= R_samplerSize.x || st.y >= R_samplerSize.y)
        return vec4(0.0); // black pixels with alpha 0

    if (orientation == 1.0) // tl
    {
        st.y = R_samplerSize.y - st.y;
    }
    else if (orientation == 2.0) // br
    {
        st.x = R_samplerSize.x - st.x;
    }
    else if (orientation == 3.0) // tr
    {
        st.x = R_samplerSize.x - st.x;
        st.y = R_samplerSize.y - st.y;
    }
    
    return vec4( texture(R_sampler, st).r,
                 texture(G_sampler, st).r,
                 texture(B_sampler, st).r,
                 1.0 );
}
