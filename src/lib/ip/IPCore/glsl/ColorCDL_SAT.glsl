//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Academy Color Decision List 1.2
//

vec4 ColorCDL_SAT (const in vec4 P, 
                   const in vec3 slope,
                   const in vec3 offset,
                   const in vec3 power,
                   const in mat4 S)
{
    vec3 c = pow(clamp(P.rgb * slope + offset, 0.0, 1.0), power);
    return vec4(clamp(S * vec4(c, 1.0), 0.0, 1.0).rgb, P.a);
}
