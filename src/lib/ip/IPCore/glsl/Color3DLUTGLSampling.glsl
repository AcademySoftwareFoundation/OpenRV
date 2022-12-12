//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  3D LUT lookup.  inScale and inOffset compensate for the fact that GL
//  textures are centered on the texel. So a scale and offset are applied
//  to move into the correct space. The outScale and outOffset re-expand
//  the output of the 3D LUT to its normal range. The out scaling is
//  required because the input 3D LUT may be normalized if it was stored as
//  an integral type.
//

vec4 Color3DLUTGLSampling(const in vec4 P,
                         const in sampler3D RGB_sampler,
                         const in vec3 inScale,
                         const in vec3 inOffset,
                         const in vec3 outScale,
                         const in vec3 outOffset)
{
    return vec4(texture(RGB_sampler, P.rgb * inScale + inOffset).rgb * outScale + outOffset,
                P.a);
}

