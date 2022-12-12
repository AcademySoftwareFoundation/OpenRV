//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Academy Color Decision List 1.2  
//
//  This version does _not_ clamp, so (A) violates the spec and (B) is
//  suitable for application in a linear HDR space.
//

vec4 ColorCDL_SAT_noClamp (const in vec4 P, 
                   const in vec3 slope,
                   const in vec3 offset,
                   const in vec3 power,
                   const in mat4 S)
{
    vec3 c = pow(P.rgb * slope + offset, power);
    return vec4((S * vec4(c, 1.0)).rgb, P.a);
}
