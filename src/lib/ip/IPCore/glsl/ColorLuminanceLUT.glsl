//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Luminance LUT lookup.  
//

vec4 ColorLuminanceLUT (const in vec4 P, 
                        const in sampler2DRect RGB_sampler,
                        const in vec2 widthMinusOne)
{
    float l = dot(P.rgb, vec3(0.2126, 0.7152, 0.0722)); // 709 weights
    return vec4( texture(RGB_sampler, widthMinusOne * vec2(l)).rgb,
                 P.a );
}
