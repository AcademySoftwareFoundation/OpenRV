//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Channel LUT lookup.  inScale and inOffset compensate for the fact
//  that GL textures are centered on the texel. So a scale and offset
//  are applied to move into the correct space. The outScale and
//  outOffset re-expand the output of the LUT to its normal range. The
//  out scaling is required because the input LUT may be normalized if
//  it was stored as an integral type.
//

vec4 ColorChannelLUT (const in vec4 P, 
                      const in sampler2DRect RGB_sampler,
                      const in vec3 widthMinusOne,
                      const in vec3 outScale,
                      const in vec3 outOffset)
{
    vec3 c = P.rgb * widthMinusOne + vec3(0.5);

    return vec4(vec3(texture(RGB_sampler, vec2(c.r, 0.0)).r,
                     texture(RGB_sampler, vec2(c.g, 0.0)).g,
                     texture(RGB_sampler, vec2(c.b, 0.0)).b) * outScale + outOffset,
                P.a);
}
        
        
