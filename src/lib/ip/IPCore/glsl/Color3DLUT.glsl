//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  3D LUT lookup.  The outScale and outOffset re-expand the output of
//  the 3D LUT to its normal range. The out scaling is required
//  because the input 3D LUT may be normalized if it was stored as an
//  integral type.
//
//  Texel values are at *centers* so we need to account for that when
//  looking up value.
//
//  we do trilinear ourselves here, and not use gl's 3d sampling
//

vec4 Color3DLUT(const in vec4 P,
                const in sampler3D RGB_sampler,
                const in vec3 sizeMinusOne,
                const in vec3 inScale,
                const in vec3 inOffset,
                const in vec3 outScale,
                const in vec3 outOffset)
{
    
    vec3 iP = clamp(P.rgb, vec3(0.0), vec3(1.0)) * sizeMinusOne;  //  Index in lattice of color
    vec3 p0 = floor(iP);                //  Floor index lattice location
    vec3 p1 = p0 + vec3(1.0, 1.0, 1.0); //  Far (diag) lattice point
    vec3 u  = iP - p0;                  //  Local cell parameter

    vec3 n0 = p0 * inScale + inOffset;  //  Normalized texture coords
    vec3 n1 = p1 * inScale + inOffset;  //  NOTE: transform to texel center

    //
    // 1st face
    //

    vec4 c0 = mix(texture(RGB_sampler, n0),
                  texture(RGB_sampler, vec3(n1.x, n0.y, n0.z)),
                  u.x);

    vec4 c1 = mix(texture(RGB_sampler, vec3(n0.x, n1.y, n0.z)),
                  texture(RGB_sampler, vec3(n1.x, n1.y, n0.z)),
                  u.x);

    vec4 ca = mix(c0, c1, u.y);

    //
    // 2nd face
    //

    vec4 c2 = mix(texture(RGB_sampler, vec3(n0.x, n0.y, n1.z)),
                  texture(RGB_sampler, vec3(n1.x, n0.y, n1.z)),
                  u.x);

    vec4 c3 = mix(texture(RGB_sampler, vec3(n0.x, n1.y, n1.z)),
                  texture(RGB_sampler, n1),
                  u.x);

    vec4 cb = mix(c2, c3, u.y);

    //
    //  Interp face colors
    //

    vec4 c = mix(ca, cb, u.z);
    
    //
    //  Apply output transform and return
    //

    return vec4(c.rgb * outScale + outOffset, P.a);
}

